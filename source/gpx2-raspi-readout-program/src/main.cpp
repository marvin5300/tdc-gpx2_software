#include "readout.h"
#include <chrono>
constexpr double max_interval { 10e-8 }; // maximum interval between two stop bits to count as hit
//constexpr unsigned wait_timeout { 10000 }; // if no signal from detector for wait_timeout amount of time before restarting the waiting...
constexpr unsigned interrupt_pin { 20 }; // interrupt pin for the falling edge signal coming from GPX2 chip
//constexpr size_t min_buffer = 200; // minimum number of events stored in buffer

auto main()->int {
	Readout readout;
	readout.start(max_interval, interrupt_pin);
	while (true) {
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
	}
	readout.stop();
	readout.join();
	return 0;
}