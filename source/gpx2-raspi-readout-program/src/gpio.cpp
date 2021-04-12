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
#include <ctime>

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

	m_callback.emplace(global_id_counter, std::make_shared<callback>(std::move(s),*this));
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

gpio::callback::callback(setting s, gpio& handler)
	: m_setting{ std::move(s) }
	, m_handler{ handler }
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
	return m_handler.write(e);
}

auto gpio::callback::read(unsigned pin_num) -> int {
	return m_handler.read(pin_num);
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
	chip = gpiod_chip_open_by_name(m_chipname.c_str());
	if (chip==nullptr) {
		std::cerr << "Open gpio chip failed" << std::endl;
		return -1;
	}
	for (auto s : m_settings) {
		for (auto pin : s.gpio_pins) {
			pins_used_by_listeners.push_back(pin);
		}
	}
	std::sort(pins_used_by_listeners.begin(), pins_used_by_listeners.end());
	pins_used_by_listeners.erase(std::unique(pins_used_by_listeners.begin(), pins_used_by_listeners.end()), pins_used_by_listeners.end());

	if (pins_used_by_listeners.empty()) {
		return 0;
	}
	lines = new gpiod_line_bulk{};
	gpiod_line_bulk_init(lines);
	int status = gpiod_chip_get_lines(chip, pins_used_by_listeners.data(), pins_used_by_listeners.size(),lines);
	if (status<0){
		std::cerr << "Chip get lines failed" << std::endl;
		return status;
	}
	status = gpiod_line_request_bulk_both_edges_events(lines, m_consumer.c_str());
	if (status<0){
		std::cerr << "Line request bulk both edges failed" << std::endl;
		return status;
	}
	return 0;
}

auto gpio::step() -> int
{
	auto* fired = new gpiod_line_bulk{};
	gpiod_line_bulk_init(fired);
	int status = gpiod_line_event_wait_bulk(lines, &c_wait_timeout, fired);
	if (status <= 0){
		//  0 -> timeout
		// -1 -> error
		delete fired;
		//std::cout << "leaving step, " << status << std::endl;
		return status;
	}
	for (unsigned i = 0; i < gpiod_line_bulk_num_lines(fired); i++){
		gpiod_line* line = gpiod_line_bulk_get_line(fired, i);
		gpiod_line_event gpio_e{};
		status = gpiod_line_event_read(line, &gpio_e);
		if (status!=0){
			delete fired;
			std::cout << "leaving step, line event read error" << std::endl;
			return status;
		}
		gpio::event e;
		e.pin = gpiod_line_offset(line);
		e.ts = gpio_e.ts;
		if (gpio_e.event_type==GPIOD_LINE_EVENT_RISING_EDGE){
			e.type = event::Type::Rising;
		}else if (gpio_e.event_type==GPIOD_LINE_EVENT_FALLING_EDGE){
			e.type = event::Type::Falling;
		}
		notify_all(e);
	}
	delete fired;
	return 0;
}

auto gpio::shutdown() -> int
{
	if (lines != nullptr){
		gpiod_line_request_bulk_input(lines, m_consumer.c_str());
		gpiod_line_release_bulk(lines);
		//delete lines;
		lines = nullptr;
	}
	for (auto it = other_lines.begin(); it != other_lines.end(); it++) {
		gpiod_line_release(it->second);
		other_lines.erase(it);
	}
	if (chip != nullptr){
		gpiod_chip_close(chip);
		//delete chip;
		chip = nullptr;
	}
	return 0;
}

auto gpio::write(const gpio::event& e) -> bool
{
	if (!m_result.valid()) {
		return false;
	}
	std::cout << "requested write pin " << e.pin << " to " << e.type;
	//gpiod_line* line =
	//gpiod_line_set_value(line, 1);
	//gpiod_line_request_output();

	return true;
}

auto gpio::read(unsigned pin_num) -> int
{
	if (std::find(pins_used_by_listeners.begin(), pins_used_by_listeners.end(), pin_num) != pins_used_by_listeners.end()) {
		std::cerr << "Error: tried to read from pin that has already an active event listener." << std::endl;
		m_run = false;
		return -1;
	}
	gpiod_line* line = new gpiod_line{};
	if (other_lines.count(pin_num)==0) {
		line = gpiod_chip_get_line(chip, pin_num);
		if (!line) {
			std::cerr << "Chip get lines failed for line " << pin_num << std::endl;
			return -1;
		}

		int ret = gpiod_line_request_input(line, m_consumer.c_str());
		if (ret < 0) {
			std::cerr << "Request line as input failed" << std::endl;
			gpiod_line_release(line);
			return -1;
		}
		other_lines[pin_num] = line;
	}
	else {
		line = other_lines.at(pin_num);
	}
	int val = gpiod_line_get_value(line);
	if (val < 0) {
		std::cerr << "Read line input failed" << std::endl;
		other_lines.erase(pin_num);
		gpiod_line_release(line);
		return -1;
	}
	return val;
}
