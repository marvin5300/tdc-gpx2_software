#include "spidevices/gpx2/gpx2.h"
#include <string>
#include <iostream>
#include <iomanip>

void print_hex(const std::string& str) {
	for (auto byte : str) {
		std::cout << std::hex << std::setw(2) << std::setfill('0') <<  static_cast<unsigned int>(byte) << " ";
	}
	std::cout << std::endl;
}

int main() {
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
	print_hex(config);
}
