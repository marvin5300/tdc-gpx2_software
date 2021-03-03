#include "spidevices/gpx2/config.h"
#include <stdint.h>
#include <string>

using namespace SPI::GPX2_TDC;

// default config defined by the author of this program
Config::Config()
	: PIN_ENA_RSTIDX{ 0 }
	, PIN_ENA_DISABLE{ 0 }
	, PIN_ENA_LVDS_OUT{ 0 }
	, PIN_ENA_REFCLK{ 0 }
	, PIN_ENA_STOP4{ 0 }
	, PIN_ENA_STOP3{ 0 }
	, PIN_ENA_STOP2{ 0 }
	, PIN_ENA_STOP1{ 0 } // reg_addr 0
	, HIGH_RESOLUTION{ 0 }
	, CHANNEL_COMBINE{ 0 }
	, HIT_ENA_STOP4{ 0 }
	, HIT_ENA_STOP3{ 0 }
	, HIT_ENA_STOP2{ 0 }
	, HIT_ENA_STOP1{ 0 } // reg_addr 1
	, BLOCKWISE_FIFO_READ{ 0 }
	, COMMON_FIFO_READ{ 0 }
	, LVS_DOUBLE_DATA_RATE{ 0 }
	, STOP_DATA_BITWIDTH{ 0 }
	, REF_INDEX_BITWIDTH{ 0 } // reg_addr 2
	, REFCLK_DIVISIONS_LOWER{ 0 } // reg_addr 3
	, REFCLK_DIVISIONS_MIDDLE{ 0 } // reg_addr 4
	, REFCLK_DIVISIONS_UPPER{ 0 } // reg_addr 5
	, LVDS_TEST_PATTERN{ 0 } // reg_addr 6
	, REFCLK_BY_XOSC{ 0 }
	, LVDS_DATA_VALID_ADJUST{ 0 } // reg_addr 7
	, CMOS_INPUT{ 0 } // reg_addr 16
{}

void Config::loadDefaultConfig() {
	// reg_addr 0
	PIN_ENA_DISABLE = 0;
	PIN_ENA_RSTIDX = 0;
	PIN_ENA_LVDS_OUT = 0;
	PIN_ENA_REFCLK = 0;
	PIN_ENA_STOP4 = 1;
	PIN_ENA_STOP3 = 1;
	PIN_ENA_STOP2 = 1;
	PIN_ENA_STOP1 = 1;

	// reg_addr 1
	HIGH_RESOLUTION = 2;
	CHANNEL_COMBINE = 0;
	HIT_ENA_STOP4 = 1;
	HIT_ENA_STOP3 = 1;
	HIT_ENA_STOP2 = 1;
	HIT_ENA_STOP1 = 1;

	// reg_addr 2
	BLOCKWISE_FIFO_READ = 1;
	COMMON_FIFO_READ = 1;
	LVS_DOUBLE_DATA_RATE = 0;
	STOP_DATA_BITWIDTH = 0b11;
	REF_INDEX_BITWIDTH = 0b010;

	// reg_addr 3
	REFCLK_DIVISIONS_LOWER = 0x40;	// 5MHz clk => T = 200ns = 200,000ps

	// reg_addr 4
	REFCLK_DIVISIONS_MIDDLE = 0x0d;	// set REFCLK_DIVISIONS to 200,000 for LSB to be 1ps

	// reg_addr 5
	REFCLK_DIVISIONS_UPPER = 0x03; // 200,000 = 0x030d40

	// reg_addr 6
	LVDS_TEST_PATTERN = 0;

	// reg_addr 7
	REFCLK_BY_XOSC = 1; // use external quartz, ignore external clock => 1
	LVDS_DATA_VALID_ADJUST = 1;

	// reg_addr 16
	CMOS_INPUT = 1;
}

// pack bits in the correct order and return a string.
std::string Config::str() const {
	std::string data;
	uint8_t byte;

	byte = PIN_ENA_RSTIDX << 7;
	byte |= PIN_ENA_DISABLE << 6;
	byte |= PIN_ENA_LVDS_OUT << 5;
	byte |= PIN_ENA_REFCLK << 4;
	byte |= PIN_ENA_STOP4 << 3;
	byte |= PIN_ENA_STOP3 << 2;
	byte |= PIN_ENA_STOP2 << 1;
	byte |= PIN_ENA_STOP1;

	data += static_cast<char>(byte);

	byte = HIGH_RESOLUTION << 6;
	byte |= CHANNEL_COMBINE << 4;
	byte |= HIT_ENA_STOP4 << 3;
	byte |= HIT_ENA_STOP3 << 2;
	byte |= HIT_ENA_STOP2 << 1;
	byte |= HIT_ENA_STOP1;

	data += static_cast<char>(byte);

	byte = BLOCKWISE_FIFO_READ << 7;
	byte |= COMMON_FIFO_READ << 6;
	byte |= LVS_DOUBLE_DATA_RATE << 5;
	byte |= STOP_DATA_BITWIDTH << 3;
	byte |= REF_INDEX_BITWIDTH;

	data += static_cast<char>(byte);

	data += REFCLK_DIVISIONS_LOWER;
	data += REFCLK_DIVISIONS_MIDDLE;
	data += REFCLK_DIVISIONS_UPPER;

	byte = (0b110 << 5) | (LVDS_TEST_PATTERN << 4);
	data += static_cast<char>(byte);

	byte = (REFCLK_BY_XOSC << 8) | (0b1 << 7) | (LVDS_DATA_VALID_ADJUST << 4) | 0b0011;
	data += static_cast<char>(byte);

	data += static_cast<char>(0b10100001);
	data += static_cast<char>(0b00010011);
	data += static_cast<char>(0b00000000);
	data += static_cast<char>(0b00001010);
	data += static_cast<char>(0b11001100);
	data += static_cast<char>(0b11001100);
	data += static_cast<char>(0b11110001);
	data += static_cast<char>(0b01111101);

	byte = (CMOS_INPUT << 2);
	data += static_cast<char>(byte);
	return data;
}