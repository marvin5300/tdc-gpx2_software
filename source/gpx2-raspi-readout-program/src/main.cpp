#include "spidevices/gpx2/gpx2.h"
#include "gpio.h"
#include <string>
#include <iostream>
#include <iomanip>

constexpr double max_interval { 10e-8 }; // maximum interval between two stop bits to count as hit
constexpr unsigned wait_timeout { 10000 }; // if no signal from detector for wait_timeout amount of time before restarting the waiting...
constexpr unsigned interrupt_pin { 20 }; // interrupt pin for the falling edge signal coming from GPX2 chip

void print_hex(const std::string& str) {
	for (auto byte : str) {
		std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned int>(byte) << " ";
	}
	std::cout << std::endl;
}

auto main()->int {
	SPI::GPX2_TDC::GPX2 gpx2{};
	SPI::GPX2_TDC::Config conf{};
	conf.loadDefaultConfig();
	conf.BLOCKWISE_FIFO_READ = 0U;

	//conf.PIN_ENA_STOP3 = 0;
	//conf.PIN_ENA_STOP4 = 0;
	//conf.HIT_ENA_STOP3 = 0;
	//conf.HIT_ENA_STOP4 = 0;

	gpx2.init();

	int verbosity = 0;

	bool status = gpx2.write_config(conf);
	std::string config = gpx2.read_config();
	if (status && (config == conf.str())) {
		if (verbosity > 1) {
			std::cout << "config written..." << std::endl;
		}
	}
	else {
		std::cout << "tried to write:" << std::endl;
		print_hex(conf.str());
		std::cout << "read back:" << std::endl;
		print_hex(config);
		std::cout << "failed to write config!" << std::endl;
		exit(-1);
	}

	gpio handler{};
	gpio::setting pin_setting{};
	pin_setting.gpio_pins = std::vector<unsigned int>{ interrupt_pin };

	auto callback = handler.list_callback(pin_setting);

	bool data_available = false;

	handler.start();

	gpx2.init_reset();
	auto event = callback->wait_async(std::chrono::milliseconds {wait_timeout});
	while (true) {
		if (event.wait_for(std::chrono::microseconds(1)) == std::future_status::ready) {
			/*if (verbosity > 1) {
				auto now = std::chrono::system_clock::now();
                auto time = std::chrono::system_clock::to_time_t(now);
				std::cerr << "event was not detected for " << static_cast<double>(wait_timeout)*0.001 << " seconds (time: " << time << ")"<< std::endl;
			}
			continue;
		}*/
			if (event.get().type == gpio::event::Rising){
				//std::cout << "gpio " << event.pin << " rising edge" << std::endl;
				data_available = false;
			}
			if (event.get().type == gpio::event::Falling) {
				data_available = true;
			}
			event = callback->wait_async(std::chrono::milliseconds {wait_timeout});
		}
		if (data_available){
			/*
			auto diffs = gpx2.get_filtered_intervals(max_interval);
			for (auto val : diffs) {
				std::cout << val << std::endl;
			}
			*/
			auto measurements = gpx2.read_results();
			for (auto meas : measurements){
				if (meas.status == SPI::GPX2_TDC::Meas::Valid){
					std::cout << static_cast<int>(meas.stop_channel) << " " << meas.ref_index << " " << meas.stop_result << std::endl;
				}
			}
		}
	}

	handler.stop();
	handler.join();
}
