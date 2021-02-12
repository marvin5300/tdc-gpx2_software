#include <stdint.h>
#include <string>

class GPX2 {
public:
	GPX2();
	~GPX2();

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