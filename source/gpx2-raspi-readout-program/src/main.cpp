#include "spidevices/gpx2/gpx2.h"
#include "gpio.h"
#include <string>
#include <iostream>
#include <iomanip>

void print_hex(const std::string& str) {
	for (auto byte : str) {
		std::cout << std::hex << std::setw(2) << std::setfill('0') <<  static_cast<unsigned int>(byte) << " ";
	}
	std::cout << std::endl;
}

void gpio_handle() {
	gpio handler{};
	gpio::setting conf{};
	conf.gpio_pins = std::vector<unsigned int>{18};

	auto callback = handler.list_callback(conf);


	handler.start();

	while (true) {

		auto event = callback->wait(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::seconds {gpio::standard_timeout}));

		std::cout << "gpio " << event.pin << std::endl;

		if (!event) {
			break;
		}
	}


	handler.stop();
	handler.join();
}


auto main()->int{
	gpio_handle();
	/*
	SPI::GPX2_TDC::GPX2 gpx2{};
	SPI::GPX2_TDC::Config conf{};
	conf.loadDefaultConfig();

	gpx2.init();

	std::cout << "trying to write config..." << std::endl;
	print_hex(conf.str());
	bool status = gpx2.write_config(conf);
	if (status) {
		std::cout << "config written..." << std::endl;
	}
	else {
		std::cout << "failed to write config!" << std::endl;
		exit(-1);
	}

	std::string config = gpx2.read_config();
	print_hex(config);*/
}
