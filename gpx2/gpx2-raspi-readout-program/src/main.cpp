#include "gpx2.h"
#include <string>
#include <iostream>

int main() {
	GPX2_TDC::GPX2 gpx2;
	std::string config = gpx2.read_config();

	for (auto byte : config) {
		std::cout << std::hex << static_cast<unsigned int>(byte) << " ";
	}
	std::cout << std::endl;
	//GPX2_TDC::Config config;
}
