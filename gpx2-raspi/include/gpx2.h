#ifndef GPX2_H
#define GPX2_H

#include "config.h"
#include <stdint.h>
#include <string>

static const uint8_t spiopc_power = 0x30; // Power on reset and stop measurement
static const uint8_t spiopc_init = 0x18; // Initializes Chip and starts measurement
static const uint8_t spiopc_write_config = 0x80; // Write to configuration register X=0..17
static const uint8_t spiopc_read_results = 0x60; // Read opcode for result and status register X=8..31
static const uint8_t spiopc_read_config = 0x40; // Readout of configuration register X=0..17

class GPX2 {
public:
	GPX2();
	~GPX2();

	Config config;

	void power_on_reset();
	void init_reset();
	bool write_config();
	bool write_config(Config data);
	bool write_config(std::string data);
	bool write_config(uint8_t reg_addr, uint8_t data);
	std::string read_all_config();
	uint8_t read_config(uint8_t reg_addr);

	bool writeSpi(uint8_t command, std::string data);
	std::string readSpi(uint8_t command, unsigned int bytesToRead);
	bool isSpiInitialised();
private:
	int pi = -1;
	int spiHandle = -1;
	unsigned int spi_freq = 61035;
	uint32_t spi_flags = 0;
	bool spiInitialised = false;

	bool spiInitialise();
};

#endif // !GPX2_H