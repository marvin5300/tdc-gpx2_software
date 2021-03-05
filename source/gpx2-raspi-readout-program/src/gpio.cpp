#include "gpio.h"
#include <shared_mutex>
#include <vector>
#include <algorithm>
#include <chrono>
#include <future>
#include <memory>
#include <iostream>
#include <condition_variable>
#include <gpiod.h>
#include <time.h>

/*
struct gpiod_line_bulk {
	struct gpiod_line *lines[GPIOD_LINE_BULK_MAX_LINES];
	//< Buffer for line pointers. //
	unsigned int num_lines;
	//< Number of lines currently held in this structure. //
	};
*/

void gpio::start()
{
	if (m_result.valid()) {
		std::cout << "gpio manager already started\n";
		return;
	}

	m_result = std::async(std::launch::async, [&] {
		std::cout << "before setup" << std::endl;
		auto result{ setup() };
		std::cout << "after setup" << std::endl;
		if (result != 0) {
			return result;
		}
		while (m_run) {
			std::cout << "before step" << std::endl;
			result = step();
			if (result != 0) {
				break;
			}
			std::cout << "after step" << std::endl;
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

	m_callback.emplace(global_id_counter, std::make_shared<callback>(std::move(s), std::shared_ptr<gpio>{this}));
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
	for (auto& [id, cb] : m_callback) {
		cb->notify(e);
	}
}

auto gpio::setting::matches(const event& e)const -> bool {
	// if pin of the event is in m_setting gpio_pins list then it matches
	// maybe more efficient way of doing this
	return (std::find(gpio_pins.begin(), gpio_pins.end(), e.pin) != gpio_pins.end());
}

gpio::callback::callback(setting s, std::shared_ptr<gpio> handler)
	: m_setting{ std::move(s) }
	, m_handler{ std::move(handler) }
{
}

auto gpio::callback::wait_async(std::chrono::milliseconds timeout) -> std::future<event>
{
	return std::async(std::launch::async, [&] {return wait(timeout); });
}

auto gpio::callback::wait(std::chrono::milliseconds timeout) -> event
{
	{
		std::unique_lock<std::mutex> lock{ m_wait_mutex };

		if (m_wait.wait_for(lock, timeout) == std::cv_status::timeout) {
			return event{};
		}
	}
	std::unique_lock<std::shared_mutex> lock{ m_access_mutex };
	return m_event;
}

auto gpio::callback::write_async(const event& e) -> std::future<bool>
{
	return std::async(std::launch::async, [&] { return write(e); });
}

auto gpio::callback::write(const event& e) -> bool
{
	return m_handler->write(e);
}

void gpio::callback::notify(const event& e)
{
	if (!m_setting.matches(e))
	{
		return;
	}
	std::unique_lock<std::shared_mutex> lock{ m_access_mutex };
	m_event = e;
	m_wait.notify_all();
}

auto gpio::setup() -> int {
	chip = gpiod_chip_open_by_name(chipname.c_str());
	if (chip==nullptr) {
		std::cerr << "Open gpio chip failed" << std::endl;
		return -1;
	}
	std::vector<unsigned int> all_pins;
	for (auto s : m_settings) {
		for (auto pin : s.gpio_pins) {
			all_pins.push_back(pin);
		}
	}
	std::sort(all_pins.begin(), all_pins.end());
	all_pins.erase(std::unique(all_pins.begin(), all_pins.end()), all_pins.end());

	lines = new gpiod_line_bulk{};
	gpiod_line_bulk_init(lines);
	int status = gpiod_chip_get_lines(chip, all_pins.data(), all_pins.size(),lines);
	if (status<0){
		std::cerr << "Chip get lines failed" << std::endl;
		return status;
	}
	status = gpiod_line_request_bulk_both_edges_events(lines, consumer.c_str());
	if (status<0){
		std::cerr << "Line request bulk both edges failed" << std::endl;
		return status;
	}
	return 0;
}

auto gpio::step() -> int
{
	std::cout << "in step 1" << std::endl;
	std::scoped_lock<std::mutex> lock{ m_gpio_mutex };
	gpiod_line_bulk* fired = new gpiod_line_bulk{};
	gpiod_line_bulk_init(fired);
	std::cout << "in step 2" << std::endl;
	int status = gpiod_line_event_wait_bulk(lines, &c_wait_timeout, fired);
	std::cout << "in step 3" << std::endl;
	if (status <= 0){
		//  0 -> timeout
		// -1 -> error
		if (fired!=nullptr){
			delete fired;
		}
		return status;
	}
	for (unsigned i = 0; i < gpiod_line_bulk_num_lines(fired); i++){
		std::cout << "in step for loop " << i << std::endl;
		gpiod_line* line = gpiod_line_bulk_get_line(fired, i);
		gpiod_line_event gpio_e{};
		status = gpiod_line_event_read(line, &gpio_e);
		if (status!=0){
			if (fired!=nullptr){
				delete fired;
			}
			return status;
		}
		gpio::event e;
		e.pin = gpiod_line_offset(line);
		e.ts = gpio_e.ts;
		if (gpio_e.event_type==GPIOD_LINE_EVENT_RISING_EDGE){
			e.type = event::Type::Rising;
		}else if (gpio_e.event_type==GPIOD_LINE_EVENT_RISING_EDGE){
			e.type = event::Type::Falling;
		}
		notify_all(e);
		std::cout << "after notify all";
	}
	if (fired!=nullptr){
		delete fired;
	}
	return 0;
}

auto gpio::shutdown() -> int
{
	gpiod_line_request_bulk_input(lines, consumer.c_str());
	gpiod_line_release_bulk(lines);
	gpiod_chip_close(chip);
	if (lines != nullptr){
		delete lines;
	}
	return 0;
}

auto gpio::write(const event& e) -> bool
{
	if (!m_result.valid()) {
		return false;
	}
	std::scoped_lock<std::mutex> lock{ m_gpio_mutex };
	std::cout << "requested write pin " << e.pin << " to " << e.type;
	//gpiod_line* line = 
	//gpiod_line_set_value(line, 1);
	//gpiod_line_request_output();

	return true;
}
