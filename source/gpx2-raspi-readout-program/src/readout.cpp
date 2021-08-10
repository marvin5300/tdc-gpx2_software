#include "readout.h"
#include "gpio.h"
#include <future>
#include <string>
#include <iostream>
#include <iomanip>
#include <queue>
#include <cmath>
#include <omp.h>
#include <chrono>
#include <csignal>
#include <algorithm>

Readout::Readout(double max_ref_diff, unsigned interrupt_pin)
	: m_max_interval{max_ref_diff}
	, m_interrupt_pin{interrupt_pin}
	, gpio_thread{
			[&] {
			auto result{ setup() };
			if (result != 0) {
				return result;
			}

			while (m_run) {
				result = read_tdc();
				if (result != 0) {
					break;
				}
			}
			return result;
		}
	}
	,analysis_thread{
			[&] {
			start_time = std::chrono::high_resolution_clock::now();
			while (m_run) {
				std::this_thread::sleep_for(process_loop_timeout);
				process_queue();
			}
			if (!tdc_stop[1].empty() && !tdc_stop[0].empty()) {
				process_queue(true);
			}
			end_time = std::chrono::high_resolution_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
			std::cerr << evt_count << " events, ";
			std::cerr << duration << " ms, ";
			auto rate = static_cast<double>(evt_count) / static_cast<double>(duration) * 1e6;
			std::cerr << "rate is " << rate << "/s, " << rate * 60 << "/min, " << rate * 60 * 60 << "/h, " << rate * 60 * 60 * 24 << "/d, " << rate * 60 * 60 * 24 * 7 << "/w" << std::endl;
			std::cerr << "non processed events in queues: " << tdc_stop[0].size() << " " << tdc_stop[1].size() << " (should not be greate than " << max_queue_size << ")" << std::endl;
			return 0;
		}
	}{}

Readout::~Readout() {
	stop();
	gpio_thread.join();
	analysis_thread.join();
}

void Readout::stop()
{
	m_run = false;
}

auto Readout::setup()->int {
	if (gpx2) {
		std::cerr << "tried to create new gpx2 device but it is already created." << std::endl;
		return -1;
	}
	gpx2 = std::make_unique<SPI::GPX2_TDC::GPX2>();
	SPI::GPX2_TDC::Config conf{};
	conf.loadDefaultConfig();
	conf.BLOCKWISE_FIFO_READ = 0U;
	conf.COMMON_FIFO_READ = 0U;

	//conf.PIN_ENA_STOP3 = 0;
	//conf.PIN_ENA_STOP4 = 0;
	//conf.HIT_ENA_STOP3 = 0;
	//conf.HIT_ENA_STOP4 = 0;

	gpx2->init();

	int verbosity = 0;

	bool status = gpx2->write_config(conf);
	std::string config = gpx2->read_config();
	if (status && (config == conf.str())) {
		if (verbosity > 1) {
			std::cerr << "config written..." << std::endl;
		}
	}
	else {
		std::cerr << "tried to write:" << std::endl;
		print_hex(conf.str());
		std::cerr << "read back:" << std::endl;
		print_hex(config);
		std::cerr << "failed to write config!" << std::endl;
		return -1;
	}

	handler = std::make_unique<gpio>();
	gpio::setting pin_setting{};

	callback = handler->list_callback(pin_setting);

	handler->start();

	gpx2->init_reset();
	return 0;
}

auto Readout::read_tdc()->int {
	// checks if data is available on the gpx2 (by checking if interrupt pin is low)
	// then reads out data from the gpx2-tdc and puts it in the queue
	while (callback->read(m_interrupt_pin) != 0) {
		// while interrupt pin is high, do not readout (since there is no data available)
		// std::this_thread::sleep_for(std::chrono::microseconds(1));
	}
	//for (unsigned i = 0; i < 4; i++) {
		auto now = std::chrono::system_clock::now();
		auto measurements = gpx2->read_results();
		for (unsigned j = 0; j < 4; j++) {
			if (measurements[j]) {
				measurements[j].ts = now;
				tdc_stop[j%2].emplace(std::move(measurements[j]));
			}
		}
	//}
	queue_condition.notify_all();
	return 0;
}

void Readout::process_queue(bool ignore_max_queue) {
	// checks ref_diff between ordered tdc_stop[0] and tdc_stop[1] channels.
	// Compares a few values from the queues, removes not matching timestamps and prints matching ones

	std::mutex mutex;
	std::unique_lock<std::mutex> lock {mutex};
	if (queue_condition.wait_for(lock,std::chrono::seconds(1))==std::cv_status::timeout || (tdc_stop[0].size() < max_queue_size && tdc_stop[1].size() < max_queue_size && !ignore_max_queue)) {
		return;
	}
	std::array<std::vector<SPI::GPX2_TDC::Meas>,2> data{};
	for (std::size_t i{0}; i<2; i++){
		while(tdc_stop[i].size()>min_queue_size || (ignore_max_queue && !tdc_stop[i].empty())){
			data[i].emplace_back(tdc_stop[i].front());
			tdc_stop[i].pop();
		}
	}
	// at this point one queue is full and we can check the content for 
	std::sort(data[0].begin(), data[0].end());
	std::unique(data[0].begin(), data[0].end());
	std::sort(data[1].begin(), data[1].end());
	std::unique(data[1].begin(), data[1].end());
	
	auto reset_ref = [&](){return static_cast<int32_t>(data[1].front().ref_index) - static_cast<int32_t>(data[0].front().ref_index);};
	while (!data[0].empty() && !data[1].empty()) {
		auto ref_diff = reset_ref();
		while (ref_diff > 0) {
			//std::cout << "ref_diff " << ref_diff << std::endl;
			data[0].erase(data[0].begin());
			//evt_count += 1U;
			if (data[0].empty()) {
				return;
			}
			ref_diff = reset_ref();
		}
		while (ref_diff < 0) {
			//std::cout << "ref_diff " << ref_diff << std::endl;
			data[1].erase(data[1].begin());
			//evt_count += 1U;
			if (data[1].empty()) {
				return;
			}
			ref_diff = reset_ref();
		}
		if (ref_diff == 0) {
			auto diff = SPI::GPX2_TDC::diff(data[0].front(), data[1].front());
			if (fabs(diff) < m_max_interval) {
				evt_count += 1;
				std::cout << std::chrono::duration_cast<std::chrono::nanoseconds>(data[0].front().ts.time_since_epoch()).count();
				std::cout << " ";
				std::cout << diff;
				//std::cout << " " << q0.front().ref_index << " " << q0.front().stop_result;
				//std::cout << " " << q1.front().ref_index << " " << q1.front().stop_result;
				std::cout << "\n";
			}
			data[0].erase(data[0].begin());
			data[1].erase(data[1].begin());
		}
	}
}
