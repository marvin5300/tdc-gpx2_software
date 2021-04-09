#include "spidevices/gpx2/gpx2.h"
#include "gpio.h"
#include <string>
#include <iostream>
#include <iomanip>

constexpr double max_interval { 10e-8 }; // maximum interval between two stop bits to count as hit
constexpr unsigned wait_timeout { 1000000 }; // if no signal from detector for wait_timeout amount of time before restarting the waiting...
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

	handler.start();

	gpx2.init_reset();
	while (true) {
		auto event = callback->wait(std::chrono::milliseconds {wait_timeout});
		if (!event) {
			if (verbosity > 1) {
				std::cout << "event was neither rising nor falling. This should not happen. Stopping." << std::endl;
			}
			break;
		}
		/*if (event.type == gpio::event::Rising){
			std::cout << "gpio " << event.pin << " rising edge" << std::endl;
		}*/
		if (event.type == gpio::event::Falling) {
			auto diffs = gpx2.get_filtered_intervals(max_interval);
			for (auto val : diffs) {
				std::cout << val << std::endl;
			}
		}
	}

	handler.stop();
	handler.join();
}
