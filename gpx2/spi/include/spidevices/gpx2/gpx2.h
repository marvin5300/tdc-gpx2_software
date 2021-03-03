#ifndef GPX2_H
#define GPX2_H

#include "config.h"
#include "../spidevice.h"
#include <stdint.h>
#include <string>

namespace SPI {
	namespace GPX2_TDC {
		static const std::uint8_t spiopc_power = 0x30; // Power on reset and stop measurement
		static const std::uint8_t spiopc_init = 0x18; // Initializes Chip and starts measurement
		static const std::uint8_t spiopc_write_config = 0x80; // Write to configuration register X=0..17
		static const std::uint8_t spiopc_read_results = 0x60; // Read opcode for result and status register X=8..31
		static const std::uint8_t spiopc_read_config = 0x40; // Readout of configuration register X=0..17

		class GPX2 : public spiDevice {
		public:
			using spiDevice::spiDevice;

			Config config;

			void init();
			void power_on_reset();
			void init_reset();
			auto write_config()->bool;
			auto write_config(const Config& data)->bool;
			auto write_config(const std::string& data)->bool;
			auto write_config(const std::uint8_t reg_addr, const std::uint8_t data)->bool;
			auto read_config()->std::string;
			auto read_config(const std::uint8_t reg_addr)->std::uint8_t;
			auto read_results()->std::string;
		};
	}
}
#endif // !GPX2_H
