#include "gpio.h"
#include <shared_mutex>
#include <vector>
#include <algorithm>
#include <chrono>
#include <future>
#include <memory>
#include <iostream>
#include <condition_variable>
extern "C" {
#include <gpiod.h>
}
/*
struct gpiod_line_bulk {
	struct gpiod_line *lines[GPIOD_LINE_BULK_MAX_LINES];
	//< Buffer for line pointers. //
	unsigned int num_lines;
	//< Number of lines currently held in this structure. //
	};
*/

std::size_t id{ 0U };

void gpio::start()
{
	if (m_result.valid()) {
		std::cout << "gpio manager already started\n";
		return;
	}

	m_result = std::async(std::launch::async, [&] {
		auto result{ setup() };
		if (result != 0) {
			return result;
		}
		while (m_run) {
			result = step();
			if (result != 0) {
				break;
			}
			std::this_thread::sleep_for(m_timeout);
		}

		return result + shutdown();
	});
}

void gpio::stop()
{
	m_run = false;
}

void gpio::join()
{
	if (!m_result.valid())
	{
		return;
	}
	m_result.wait();
}

gpio::~gpio()
{
	if (m_result.valid()) {
		stop();
		m_result.wait();
	}
}


auto gpio::list_callback(setting s) -> std::shared_ptr<callback>
{
	if (m_result.valid()) {
		return nullptr;
	}

	m_settings.emplace_back(s);

	m_callback.emplace(global_id_counter, std::make_shared<callback>(std::move(s)));
	global_id_counter++;

	return m_callback[global_id_counter - 1U];
}

void gpio::remove_callback(std::size_t id)
{
	if (m_callback.find(id) == m_callback.end()) {
		return;
	}

	m_callback.erase(id);
}

void gpio::notify_all(event e)
{
	for (auto& [id, callback] : m_callback) {
		callback->notify(e);
	}
}

auto gpio::setting::matches(const event& e)const -> bool {
	// if pin of the event is in m_setting gpio_pins list then it matches
	// maybe more efficient way of doing this
	if (std::find(gpio_pins.begin(), gpio_pins.end(), e.pin) != gpio_pins.end()) {
		return true;
	}
	else {
		return false;
	}
}

gpio::callback::callback(setting s, gpio& handler)
	: m_setting{ std::move(s) }
	, m_handler{ handler }
{
}

auto gpio::callback::wait_async(std::chrono::milliseconds timeout) -> std::future<event>
{
	return std::async(std::launch::async, [&] {wait(timeout); });
}

auto gpio::callback::wait(std::chrono::milliseconds timeout) -> event
{
	{
		std::shared_lock<std::shared_mutex> lock{ m_wait_mutex };

		if (m_wait.wait_for(lock, timeout) == std::cv_status::timeout) {
			return event{};
		}
	}
	std::shared_lock<std::shared_mutex> lock{ m_access_mutex };
	return m_event;
}

auto gpio::callback::write_async(const event& e) -> std::future<bool>
{
	return std::async(std::launch::async, [&] {write(e); });
}

auto gpio::callback::write(const event& e) -> bool
{
	return m_handler.write(e);
}

void gpio::callback::notify(const event& e)
{
	if (!m_setting.matches(e))
	{
		return;
	}
	{
		std::unique_lock<std::shared_mutex> lock{ m_access_mutex };
		m_event = e;
	}
	m_wait.notify_all();
}


auto gpio::setup() -> int
{
	chip = gpiod_chip_open_by_name(chipname.c_str());
	if (!chip) {
		std::cerr << "Open gpio chip failed" << std::endl;
		return false;
	}
	int ret{};
	std::vector<std::size_t> all_pins;
	for (auto s : m_settings) {
		for (auto pin : s.gpio_pins) {
			all_pins.push_back(pin);
			/*if (std::find(all_pins.begin(), all_pins.end(), pin) == all_pins.end()) {
				all_pins.push_back(pin);
			}*/
		}
	}
	std::sort(all_pins.begin(), all_pins.end());
	all_pins.erase(std::unique(all_pins.begin(), all_pins.end()), all_pins.end());
	for (auto pin : all_pins) {
		//std::shared_ptr<struct gpiod_line> line = nullptr;
		//line.reset( gpiod_chip_get_line(chip, pin) );
		gpiod_line* line = gpiod_chip_get_line(chip, pin);
		if (line == nullptr) {
			std::cerr << "Get line " << pin << " failed" << std::endl;
		}
		else {
			gpiod_line_request_config config{
				consumer.c_str() , // consumer
				0, // request_type
				0 // flags
			};
			ret = gpiod_line_request(line, &config, 0);
			if (ret == -1) {
				std::cerr << "Request for line " << pin << " did not work" << std::endl;
			}
			gpiod_line_bulk_add(lines, line);
		}
	}
	return true;
}

auto gpio::step() -> int
{
	std::scoped_lock<std::mutex> lock{ m_gpio_mutex };

}

auto gpio::shutdown() -> int
{
	gpiod_line_request_bulk_input(lines, consumer.c_str());
	gpiod_line_release_bulk(lines);
	gpiod_chip_close(chip);
}

auto gpio::write(const event& e) -> bool
{
	if (!mresult.valid()) {
		return false;
	}
	std::scoped_lock<std::mutex> lock{ m_gpio_mutex };


}


