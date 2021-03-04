#ifndef GPIODHANDLER_H
#define GPIODHANDLER_H
#include <cstdint>
#include <vector>
#include <memory>
extern "C" {
#include <gpiod.h>
}
enum class PinBias : std::uint8_t {
	OpenDrain = 0x01,
	OpenSource = 0x02,
	ActiveLow = 0x04,
	PullDown = 0x10,
	PullUp = 0x20
};
class GpiodHandler {
public:
	GpiodHandler() = default;
	~GpiodHandler();
	auto init(const std::vector<unsigned int>& gpioPins)->bool;
private:
	auto setInput(unsigned int gpio, PinBias bias = PinBias::PullDown);
	auto setOutput(unsigned int gpio, bool initState = false);

	const char* consumer = "gpiodhandler";
	//std::shared_ptr<gpiod_chip> chip{ nullptr };
	gpiod_chip* chip{ nullptr };
	std::vector<gpiod_line*> lines{};

};

#endif // GPIODHANDLER_H