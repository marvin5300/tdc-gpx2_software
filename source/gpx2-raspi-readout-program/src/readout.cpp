#include "readout.h"
#include "gpio.h"
#include <future>
#include <string>
#include <iostream>
#include <iomanip>
#include <queue>
#include <cmath>

Readout::~Readout()
{
	if (m_result.valid()) {
		stop();
		m_result.wait();
	}
}

void Readout::start(double max_interval, unsigned interrupt_pin) {
	m_max_interval = max_interval;
	m_interrupt_pin = interrupt_pin;

	if (m_result.valid()) {
		std::cout << "gpio manager already started\n";
		return;
	}

	process_result = std::async(std::launch::async, &Readout::process_loop, this);

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

		process_result.wait();

		return result + process_result.get() + shutdown();
	});

}

auto Readout::process_loop()->int {
	while (m_run) {
		process_queue();
		std::this_thread::sleep_for(process_loop_timeout);
	}
	return 0;
}

void Readout::stop()
{
	m_run = false;
}

void Readout::join()
{
	if (!m_result.valid())
	{
		return;
	}
	m_result.wait();
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
			std::cout << "config written..." << std::endl;
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
	auto measurements = gpx2->read_results();

	if (measurements.size() > 4) {
		std::cerr << "read more than 4 channel. This can't be, closing..." << std::endl;
		return -1;
	}

	size_t first{};
	size_t second{};

	// do it for stop0 channel
	first = 0;
	second = 2;
	if (SPI::GPX2_TDC::diff(measurements[0], measurements[2]) > 0) {
		first = 2;
		second = 0;
	}
	if (measurements[first].status == SPI::GPX2_TDC::Meas::Valid) {
		auto back_opt = stop0.back();
		if (!back_opt.has_value() || diff(back_opt.value(), measurements[first]) != 0.) {
			stop0.push(measurements[first]);
		}
	}
	if (measurements[second].status == SPI::GPX2_TDC::Meas::Valid) {
		auto back_opt = stop0.back();
		if (!back_opt.has_value() || diff(back_opt.value(), measurements[second]) != 0.) {
			stop0.push(measurements[second]);
		}
	}

	// do it for stop1 channel
	first = 1;
	second = 3;
	if (SPI::GPX2_TDC::diff(measurements[0], measurements[2]) > 0) {
		first = 3;
		second = 1;
	}
	if (measurements[first].status == SPI::GPX2_TDC::Meas::Valid) {
		auto back_opt = stop1.back();
		if (!back_opt.has_value() || diff(back_opt.value(), measurements[first]) != 0.) {
			stop1.push(measurements[first]);
		}
	}
	if (measurements[second].status == SPI::GPX2_TDC::Meas::Valid) {
		auto back_opt = stop1.back();
		if (!back_opt.has_value() || diff(back_opt.value(), measurements[second]) != 0.) {
			stop1.push(measurements[second]);
		}
	}

	/*
	for (auto meas : measurements){
		if (meas.status == SPI::GPX2_TDC::Meas::Valid){
			//std::cout << static_cast<int>(meas.stop_channel) << " " << meas.ref_index << " " << meas.stop_result << std::endl;
		}
	}
	*/
	return 0;
}

void Readout::process_queue() {
	// checks interval between ordered stop0 and stop1 channels.
	// Compares a few values from the queues, removes not matching timestamps and prints matching ones
	//if (stop0.size() > 200 || stop1.size() > 200) {
	//	auto q0 = stop0.dump();
	//	auto q1 = stop1.dump();
	//	std::cout << "\nstop0:" << std::endl;
	//	while(!q0.empty()){
	//		std::cout << std::setw(8) << std::setfill('0') << q0.front().ref_index << "." << std::setw(6) << std::setfill('0') << q0.front().stop_result << std::endl;
	//		q0.pop();
	//	}
	//	std::cout << "\nstop1:" << std::endl;
	//	while (!q1.empty()) {
	//		std::cout << std::setw(8) << std::setfill('0') << q1.front().ref_index << "." << std::setw(8) << std::setfill('0') << q1.front().stop_result << std::endl;
	//		q1.pop();
	//	}
	//	std::cout << std::endl;
	//	m_run = false;
	//}
	//return;
	auto front_opt_0{ stop0.front() };
	auto front_opt_1{ stop1.front() };
	if (!front_opt_0.has_value() || !front_opt_1.has_value()) {
		return;
	}
	auto interval = SPI::GPX2_TDC::diff(front_opt_0.value(), front_opt_1.value());
	while (!std::isnormal(interval)) {
		//std::cerr << "interval is nan, values: " << front_opt_0.value().ref_index << " " << front_opt_1.value().ref_index << std::endl;
		stop0.pop();
		stop1.pop();
		front_opt_0 = stop0.front();
		front_opt_1 = stop1.front();
		if (!front_opt_0.has_value() || !front_opt_1.has_value()) {
			return;
		}
		interval = SPI::GPX2_TDC::diff(front_opt_0.value(), front_opt_1.value());
	}
	size_t size0{ stop0.size() };
	size_t size1{ stop1.size() };
	if (size0 > 1000 || size1 > 1000) {
		std::cerr << "stop0 size: " << size0 << " stop1 size: " << size1 << " interval: " << interval;
		std::cerr << " stop0.front().ref_index " << front_opt_0.value().ref_index;
		std::cerr << " stop1.front().ref_index " << front_opt_1.value().ref_index;
		std::cerr << std::endl;
	}
	while (interval > m_max_interval) {
		front_opt_0 = stop0.pop();
		if (!front_opt_0.has_value()) {
			return;
		}
		interval = SPI::GPX2_TDC::diff(front_opt_0.value(), front_opt_1.value());
	}
	while (interval < -m_max_interval) {
		front_opt_1 = stop1.pop();
		if (!front_opt_1.has_value()) {
			return;
		}
		interval = SPI::GPX2_TDC::diff(front_opt_0.value(), front_opt_1.value());
	}
	if (fabs(interval) < m_max_interval) {
		std::cout << interval;
		std::cout << " " << front_opt_0.value().ref_index << " " << front_opt_0.value().stop_result;
		std::cout << " " << front_opt_1.value().ref_index << " " << front_opt_1.value().stop_result;
		std::cout << std::endl;
	}
	return;
}
