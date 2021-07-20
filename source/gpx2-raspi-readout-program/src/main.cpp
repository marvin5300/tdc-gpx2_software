#include "readout.h"
#include <chrono>
#include <csignal>
#include <condition_variable>
#include <mutex>

constexpr double max_interval { 100e-9 }; // maximum interval between two stop bits to count as hit
//constexpr unsigned wait_timeout { 10000 }; // if no signal from detector for wait_timeout amount of time before restarting the waiting...
constexpr unsigned interrupt_pin { 20 }; // interrupt pin for the falling edge signal coming from GPX2 chip
//constexpr size_t min_buffer = 200; // minimum number of events stored in buffer

std::condition_variable main_run;
void signalHandler(int signum) {
	std::cerr << "received signal " << signum << std::endl;
	main_run.notify_all();
}

auto main()->int {
	std::signal(SIGINT, signalHandler);
	std::signal(SIGTERM, signalHandler);
	std::signal(SIGQUIT, signalHandler);
	Readout readout;
	readout.start(max_interval, interrupt_pin);
	std::mutex mx;
	std::unique_lock<std::mutex> lock{mx};
	main_run.wait(lock);
	readout.stop();
	readout.join();
	return 0;
}