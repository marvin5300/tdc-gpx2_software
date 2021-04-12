#ifndef READOUT_H
#define READOUT_H

#include "gpx2.h"
#include "queue.h"
#include <future>
#include <iostream>
#include <iomanip>
#include <mutex>
#include <shared_mutex>
#include <chrono>

inline static void print_hex(const std::string& str) {
	for (auto byte : str) {
		std::cout << "0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned int>(byte) << " ";
	}
	std::cout << std::endl;
}

class Readout {
public:
	Readout() = default;
	~Readout();

	void start(double max_interval, unsigned interrupt_pin);

	void stop();

	void join();

private:
	[[nodiscard]] auto setup()->int;
	[[nodiscard]] auto read_tdc()->int;
	[[nodiscard]] auto shutdown()->int;

	[[nodiscard]] auto process_loop()->int;

	void process_queue();

	const std::chrono::milliseconds process_loop_timeout = std::chrono::milliseconds(5);
	double m_max_interval{};
	unsigned m_interrupt_pin{};
	std::future<int> m_result{};
	std::future<int> process_result{};
	std::unique_ptr<SPI::GPX2_TDC::GPX2> gpx2;
	thrdsf::Queue<SPI::GPX2_TDC::Meas> stop0;
	thrdsf::Queue<SPI::GPX2_TDC::Meas> stop1;
	std::atomic<bool> m_run{ true };
};
#endif // READOUT_H
