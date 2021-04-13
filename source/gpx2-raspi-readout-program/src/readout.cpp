#include "readout.h"
#include "gpio.h"
#include <future>
#include <string>
#include <iostream>
#include <iomanip>
#include <queue>
#include <cmath>
#include <omp.h>
#include <csignal>

std::unique_ptr<Readout> Readout::instance{};

Readout::~Readout()
{
	if (m_result.valid()) {
		stop();
		m_result.wait();
		process_result.wait();
	}
}

void Readout::start(double max_ref_diff, unsigned interrupt_pin) {
	if (m_result.valid()) {
		std::cerr << "readout already started\n";
		return;
	}

	instance.reset(this);

	m_max_interval = max_ref_diff;
	m_interrupt_pin = interrupt_pin;

	m_result = std::async(std::launch::async, [&] {
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


		return result + shutdown();
	});

	process_result = std::async(std::launch::async, [&] {
		start_time = std::chrono::high_resolution_clock::now();
		while (m_run) {
			std::this_thread::sleep_for(process_loop_timeout);
			process_queue();
		}
		while (stop1.size() > 0 && stop0.size() > 0) {
			process_queue(true);
		}
		end_time = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
		std::cout << evt_count << " events, ";
		std::cout << duration << " ms, ";
		auto rate = static_cast<double>(evt_count) / static_cast<double>(duration) * 1e6;
		std::cout << "rate is " << rate << "/s, " << rate * 60 << "/min, " << rate * 60 * 24 << "/d, " << rate * 60 * 24 * 7 << "/w" << std::endl;
		std::cout << "non processed events in queues: " << stop0.size() << " " << stop1.size() << " (should not be greate than " << max_queue_size << ")" << std::endl;
		return 0;
	});
}

void Readout::stop()
{
	m_run = false;
}

void Readout::join()
{
	if (!m_result.valid()||!process_result.valid())
	{
		std::cerr << "m_result valid? " << m_result.valid() << " process_result valid? " << process_result.valid() << std::endl;
		return;
	}
	m_result.wait();
	process_result.wait();
}

auto Readout::setup()->int {
	if (gpx2) {
		std::cerr << "tried to create new gpx2 device but it is already created." << std::endl;
		return -1;
	}
	gpx2 = std::make_unique<SPI::GPX2_TDC::GPX2>();
	SPI::GPX2_TDC::Config conf{};
	conf.loadDefaultConfig();
	conf.BLOCKWISE_FIFO_READ = 1U;
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

auto Readout::shutdown() -> int
{
	// put here all stuff that has to be done before closing
	if (handler) {
		handler->stop();
		handler->join();
	}
	return 0;
}

auto Readout::read_tdc()->int {
	// checks if data is available on the gpx2 (by checking if interrupt pin is low)
	// then reads out data from the gpx2-tdc and puts it in the queue
	while (callback->read(m_interrupt_pin) != 0) {
		// while interrupt pin is high, do not readout (since there is no data available)
		std::this_thread::sleep_for(readout_loop_timeout);
	}
	for (unsigned i = 0; i < 4; i++) {
		auto measurements = gpx2->read_results();
		for (unsigned j = 0; j < 4; j++) {
			if (measurements[j]) {
				if (j % 2 == 0) {
					stop0.push(std::move(measurements[j]));
				}
				else {
					stop1.push(std::move(measurements[j]));
				}
			}
		}
	}
	return 0;
}

void Readout::process_queue(bool ignore_max_queue) {
	// checks ref_diff between ordered stop0 and stop1 channels.
	// Compares a few values from the queues, removes not matching timestamps and prints matching ones

	if (stop0.size() < max_queue_size && stop1.size() < max_queue_size && !ignore_max_queue) {
		return;
	}
	auto q0 = stop0.dump();
	auto q1 = stop1.dump();
#pragma omp parallel sections num_threads(2)
	{
#pragma omp section 
		{
			std::sort(q0.begin(), q0.end());
			std::unique(q0.begin(), q0.end());
		}
#pragma omp section
		{
			std::sort(q1.begin(), q1.end());
			std::unique(q1.begin(), q1.end());
		}
	}
	
	while (!q0.empty() && !q1.empty()) {
		auto ref_diff = static_cast<int32_t>(q1.front().ref_index) - static_cast<int32_t>(q0.front().ref_index);
		while (ref_diff > 0) {
			//std::cout << "ref_diff " << ref_diff << std::endl;
			q0.pop_front();
			//evt_count += 1U;
			if (q0.empty()) {
				return;
			}
			ref_diff = static_cast<int32_t>(q1.front().ref_index) - static_cast<int32_t>(q0.front().ref_index);
		}
		while (ref_diff < 0) {
			//std::cout << "ref_diff " << ref_diff << std::endl;
			q1.pop_front();
			//evt_count += 1U;
			if (q1.empty()) {
				return;
			}
			ref_diff = static_cast<int32_t>(q1.front().ref_index) - static_cast<int32_t>(q0.front().ref_index);
		}
		if (ref_diff == 0) {
			auto diff = SPI::GPX2_TDC::diff(q0.front(), q1.front());
			if (fabs(diff) < m_max_interval) {
				evt_count += 1;
				std::cout << diff;
				//std::cout << " " << q0.front().ref_index << " " << q0.front().stop_result;
				//std::cout << " " << q1.front().ref_index << " " << q1.front().stop_result;
				std::cout << std::endl;
			}
			q0.pop_front();
			q1.pop_front();
		}
		/*
		std::cout << "\nstop0, " << q0.size() << ":" << std::endl;
		while (!q0.empty()) {
			std::cout << std::setw(8) << std::setfill('0') << q0.front().ref_index << "." << std::setw(6) << std::setfill('0') << q0.front().stop_result << std::endl;
			q0.pop_front();
		}
		std::cout << "\nstop1, " << q1.size() << ":" << std::endl;
		while (!q1.empty()) {
			std::cout << std::setw(8) << std::setfill('0') << q1.front().ref_index << "." << std::setw(8) << std::setfill('0') << q1.front().stop_result << std::endl;
			q1.pop_front();
		}
		std::cout << std::endl;
		*/
	}
}