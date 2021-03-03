#include "gpx2.h"
#include "config.h"
#include <iostream>
#include <iomanip>

extern "C" {
#include <pigpiod_if2.h>
}

using namespace GPX2_TDC;

GPX2::GPX2() {
	pi = pigpio_start((char*)"127.0.0.1", (char*)"8888");
	if (pi < 0) {
		std::cerr << "Could not connect to pigpio daemon. Is pigpiod running? Start with sudo pigpiod\n";
		return;
	}
	if (!spiInitialise()) {
		std::cerr << "Could not initialise spi connection.\n";
		exit(-1);
		return;
	}
	power_on_reset();
}

GPX2::~GPX2() {
	if (spiHandle >= 0) {
		bool status = spi_close(pi, spiHandle);
		if (status != 0) {
			std::cerr << "pi bad handle";
		}
	}
	if (pi >= 0) {
		pigpio_stop(pi);
	}
}

void GPX2::power_on_reset() {
	writeSpi(spiopc_power, "");
}

void GPX2::init_reset() {
	writeSpi(spiopc_init, "");
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
	return writeSpi(spiopc_write_config, data);
}

bool GPX2::write_config(const std::uint8_t reg_addr, const std::uint8_t data) {
	if (reg_addr > 16) {
		std::cerr << "write config is only possible on register addr. 0...16\n";
		return false;
	}
	std::string conf_str = std::string({static_cast<char>(data)});
	return writeSpi(spiopc_write_config | reg_addr, conf_str);
}

std::string GPX2::read_config() {
	return readSpi(spiopc_read_config, 17);
}

std::uint8_t GPX2::read_config(const std::uint8_t reg_addr) {
	if (reg_addr > 16) {
		std::cerr << "read config is only possible on register addr. 0...16\n";
		return 0;
	}
	std::string data = readSpi(spiopc_read_config | reg_addr, 1);
	if (data.empty()) {
		return 0;
	}
	return data[0];
}

std::string GPX2::read_results() {
	return readSpi(spiopc_read_results | 0x08, 26);
}

bool GPX2::writeSpi(const std::uint8_t command, const std::string& data) {
	if (!spiInitialised) {
		if (!spiInitialise()) {
			return false;
		}
	}
	const std::size_t n = data.size() + 1;

	auto txBuf{ std::make_unique<char[]>(n) };
	auto rxBuf{ std::make_unique<char[]>(n) };
	txBuf[0] = static_cast<char>(command);
	for (std::size_t i = 1; i < n; i++) {
		txBuf[i] = data[i - 1];
	}

	auto status = spi_xfer(pi, spiHandle, txBuf.get(), rxBuf.get(), n);
	if (status != static_cast<decltype(status)>(n)) {
		if (status == PI_BAD_HANDLE) {
			std::cerr << "writeSpi(...) : PI_BAD_HANDLE\n";
		}
		else if (status == PI_SPI_XFER_FAILED) {
			std::cerr << "writeSpi(...) : PI_SPI_XFER_FAILED\n";
		}
		else if (status == PI_BAD_SPI_COUNT) {
			std::cerr << "writeSpi(...): PI_BAD_SPI_COUNT\n";
		}
		else {
			std::cerr << "writeSpi(...) : wrong number of bytes transfered\n";
		}
		return false;
	}
	return status;
}

std::string GPX2::readSpi(const std::uint8_t command, const std::size_t bytesToRead) {
	if (!spiInitialised) {
		if (!spiInitialise()) {
			return "";
		}
	}
	const std::size_t n = bytesToRead + 1;

	auto txBuf{ std::make_unique<char[]>(n) };
	auto rxBuf{ std::make_unique<char[]>(n) };
	txBuf[0] = static_cast<char>(command);

	auto status = spi_xfer(pi, spiHandle, txBuf.get(), rxBuf.get(), n);
	if (status != static_cast<decltype(status)>(n)) {
		if (status == PI_BAD_HANDLE) {
			std::cerr << "readSpi(...) : PI_BAD_HANDLE\n";
		}
		else if (status == PI_SPI_XFER_FAILED) {
			std::cerr << "readSpi(...) : PI_SPI_XFER_FAILED\n";
		}
		else if (status == PI_BAD_SPI_COUNT) {
			std::cerr << "readSpi(...) : PI_BAD_SPI_COUNT\n";
		}
		else {
			std::cerr << "readSpi(...) : wrong number of bytes transfered\n";
		}
		return "";
	}

	std::string data;
	for (std::size_t i = 1; i < n; i++) {
		data += rxBuf[i];
	}
	return data;
}

bool GPX2::isSpiInitialised() {
	return spiInitialised;
}

bool GPX2::spiInitialise() {
	if (spiInitialised) {
		return true;
	}
	spiHandle = spi_open(pi, 0, spi_freq, spi_flags);
	if (spiHandle < 0) {
		std::string errstr = "";
		switch (spiHandle) {
		case PI_BAD_CHANNEL:
			errstr = "DMA channel not 0-15";
			break;
		case PI_BAD_SPI_SPEED:
			errstr = "bad SPI speed";
			break;
		case PI_BAD_FLAGS:
			errstr = "bad spi open flags";
			break;
		case PI_NO_AUX_SPI:
			errstr = "no auxiliary SPI on Pi A or B";
			break;
		case PI_SPI_OPEN_FAILED:
			errstr = "can't open SPI device";
			break;
		default:
			break;
		}
		std::cerr << "Could not initialise SPI bus:" << errstr << "\n";
		return false;
	}
	spiInitialised = true;
	return true;
}
