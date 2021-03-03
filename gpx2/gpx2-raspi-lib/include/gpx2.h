#ifndef GPX2_H
#define GPX2_H

#include "config.h"
#include <stdint.h>
#include <string>

namespace GPX2_TDC {

static const std::uint8_t spiopc_power = 0x30; // Power on reset and stop measurement
static const std::uint8_t spiopc_init = 0x18; // Initializes Chip and starts measurement
static const std::uint8_t spiopc_write_config = 0x80; // Write to configuration register X=0..17
static const std::uint8_t spiopc_read_results = 0x60; // Read opcode for result and status register X=8..31
static const std::uint8_t spiopc_read_config = 0x40; // Readout of configuration register X=0..17

	class GPX2 {
	public:
		GPX2();
		~GPX2();

		Config config;

		void power_on_reset();
		void init_reset();
		bool write_config();
		bool write_config(const Config& data);
		bool write_config(const std::string& data);
		bool write_config(const std::uint8_t reg_addr, const std::uint8_t data);
		std::string read_config();
		std::uint8_t read_config(const std::uint8_t reg_addr);
		std::string read_results();

		bool writeSpi(const std::uint8_t command, const std::string& data);
		std::string readSpi(const std::uint8_t command, const std::size_t bytesToRead);
		bool isSpiInitialised();
	private:
		int pi = -1;
		int spiHandle = -1;
		unsigned int spi_freq = 61035;
		uint32_t spi_flags = 0b10;
		bool spiInitialised = false;

		bool spiInitialise();
	};
}
#endif // !GPX2_H
