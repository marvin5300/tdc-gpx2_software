#ifndef GPX2_H
#define GPX2_H

#include "config.h"
#include "../spidevice.h"
#include <stdint.h>
#include <limits>
#include <string>
#include <vector>
#include <queue>


namespace SPI {
	namespace GPX2_TDC {
		static constexpr std::uint8_t spiopc_power = 0x30; // Power on reset and stop measurement
		static constexpr std::uint8_t spiopc_init = 0x18; // Initializes Chip and starts measurement
		static constexpr std::uint8_t spiopc_write_config = 0x80; // Write to configuration register X=0..17
		static constexpr std::uint8_t spiopc_read_results = 0x60; // Read opcode for result and status register X=8..31
		static constexpr std::uint8_t spiopc_read_config = 0x40; // Readout of configuration register X=0..17

		enum class StopChannel{
			channel_0,
			channel_1,
			channel_2,
			channel_3
		};

		struct Meas{
			enum Status{
				Invalid,
				Valid
			}status{Invalid};
			StopChannel stop_channel{};
			uint32_t ref_index{};
			uint32_t stop_result{};
			double lsb_ps{1.};
			double refclk_freq{std::numeric_limits<double>::quiet_NaN()};
			inline operator bool()
			{
				return status != Invalid;
			}
			//friend double operator-(const Meas& first, const Meas& second);
		};
		/*inline double operator-(const Meas& first, const Meas& second){
			if (!first || !second || first.refclk_freq != second.refclk_freq || first.refclk_freq==0.){
				std::cout << "measurement not valid" << std::endl;
			}
			double refclk_period = 1/first.refclk_freq;
			double stop_first = first.lsb_ps*first.stop_result;
			double stop_second = second.lsb_ps*second.stop_result;
			double result{};
			result = refclk_period*static_cast<double>(second.ref_index - first.ref_index);
			result += (stop_second - stop_first)*pico_second;
			return result;
		}*/

		class GPX2 : public spiDevice {
		public:
			using spiDevice::spiDevice;

			void init(std::string busAddress = "/dev/spidev0.0", std::uint32_t speed = 61035, Mode mode = SPI::Mode::spi_mode_1, std::uint8_t bits = 8);
			void power_on_reset();
			void init_reset();
			[[nodiscard]] auto write_config()->bool;
			[[nodiscard]] auto write_config(const Config& data)->bool;
			[[nodiscard]] auto read_config()->std::string;
			[[nodiscard]] auto get_filtered_intervals(double max_interval)->std::vector<double>;
			[[nodiscard]] auto read_results()->std::vector<Meas>;
		private:
			[[nodiscard]] auto write_config(const std::string& data)->bool;
			[[nodiscard]] auto write_config(const std::uint8_t reg_addr, const std::uint8_t data)->bool;
			[[nodiscard]] auto read_config(const std::uint8_t reg_addr)->std::uint8_t;
			std::queue<Meas> readout_buffer;
			Config config;
		};
	}
}
#endif // !GPX2_H
