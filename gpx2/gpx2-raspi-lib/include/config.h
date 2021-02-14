#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include <string>

namespace GPX2_TDC {
	struct Config {
		// reg_addr 0
		uint8_t PIN_ENA_RSTIDX : 1;
		uint8_t PIN_ENA_DISABLE : 1;
		uint8_t PIN_ENA_LVDS_OUT : 1;
		uint8_t PIN_ENA_REFCLK : 1;
		uint8_t PIN_ENA_STOP4 : 1;
		uint8_t PIN_ENA_STOP3 : 1;
		uint8_t PIN_ENA_STOP2 : 1;
		uint8_t PIN_ENA_STOP1 : 1;

		// reg_addr 1
		uint8_t HIGH_RESOLUTION : 2;
		uint8_t CHANNEL_COMBINE : 2;
		uint8_t HIT_ENA_STOP4 : 1;
		uint8_t HIT_ENA_STOP3 : 1;
		uint8_t HIT_ENA_STOP2 : 1;
		uint8_t HIT_ENA_STOP1 : 1;

		// reg_addr 2
		uint8_t BLOCKWISE_FIFO_READ : 1;
		uint8_t COMMON_FIFO_READ : 1;
		uint8_t LVS_DOUBLE_DATA_RATE : 1;
		uint8_t STOP_DATA_BITWIDTH : 2;
		uint8_t REF_INDEX_BITWIDTH : 3;

		// reg_addr 3
		uint8_t REFCLK_DIVISIONS_LOWER; // (Lower byte)

		// reg_addr 4
		uint8_t REFCLK_DIVISIONS_MIDDLE; // (Middle byte)

		// reg_addr 5
		uint8_t REFCLK_DIVISIONS_UPPER : 4; // (Upper bits)

		// reg_addr 6
		uint8_t LVDS_TEST_PATTERN : 1;

		// reg_addr 7
		uint8_t REFCLK_BY_XOSC : 1;
		uint8_t LVDS_DATA_VALID_ADJUST : 2;

		// reg_addr 16
		uint8_t CMOS_INPUT : 1;

		Config();
		std::string str();
	};
}
#endif // !CONFIG_H
