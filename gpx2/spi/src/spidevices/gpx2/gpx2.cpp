#include "spidevices/gpx2/gpx2.h"
#include "spidevices/gpx2/config.h"
#include <iostream>
#include <iomanip>

using namespace SPI::GPX2_TDC;

void GPX2::init() {
	spiDevice::init();
	power_on_reset();
}

void GPX2::power_on_reset() {
	write(spiopc_power, "");
}

void GPX2::init_reset() {
	write(spiopc_init, "");
}

bool GPX2::write_config() {
	return write_config(config);
}

bool GPX2::write_config(const Config& data) {
	return write_config(data.str());
}

bool GPX2::write_config(const std::string& data) {
	if (data.size() != 17) {
		std::cerr << "write all of the config only possible if 17 bytes of data provided\n";
		return false;
	}
	return write(spiopc_write_config, data);
}

bool GPX2::write_config(const std::uint8_t reg_addr, const std::uint8_t data) {
	if (reg_addr > 16) {
		std::cerr << "write config is only possible on register addr. 0...16\n";
		return false;
	}
	std::string conf_str = std::string({static_cast<char>(data)});
	return write(spiopc_write_config | reg_addr, conf_str);
}

std::string GPX2::read_config() {
	return read(spiopc_read_config, 17);
}

std::uint8_t GPX2::read_config(const std::uint8_t reg_addr) {
	if (reg_addr > 16) {
		std::cerr << "read config is only possible on register addr. 0...16\n";
		return 0;
	}
	std::string data = read(spiopc_read_config | reg_addr, 1);
	if (data.empty()) {
		return 0;
	}
	return data[0];
}

std::string GPX2::read_results() {
	return read(spiopc_read_results | 0x08, 26);
}
