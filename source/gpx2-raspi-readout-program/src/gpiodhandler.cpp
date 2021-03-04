#include "gpiodhandler.h"
#include <string>
#include <iostream>
#include <vector>
#include <memory>
extern "C" {
#include <gpiod.h>
}

GpiodHandler::GpiodHandler() {

}

GpiodHandler::~GpiodHandler() {
	for (auto line : lines) {
		gpiod_line_request_input(line, consumer);
		gpiod_line_release(line);
	}
	gpiod_chip_close(chip);
}

bool GpiodHandler::init(const std::vector<unsigned int>& gpioPins) {
	//std::string chippath = "/dev/gpiochip0";
	std::string chippath = "gpiochip0";
	//chip.reset( gpiod_chip_open_by_name(chippath.c_str()));
	chip = gpiod_chip_open_by_name(chippath.c_str());
	if (!chip) {
		std::cerr << "Open gpio chip failed" << std::endl;
		return false;
	}
	int ret{};
	for (auto pin : gpioPins) {
		//std::shared_ptr<struct gpiod_line> line = nullptr;
		//line.reset( gpiod_chip_get_line(chip, pin) );
		gpiod_line *line = gpiod_chip_get_line(chip, pin);
		if (!line) {
			std::cerr << "Get line " << pin << " failed" << std::endl;
		}
		else {
			gpiod_line_request_config config{
				consumer, // consumer
				0, // request_type
				0 // flags
			};
			ret = gpiod_line_request(line, & config, 0);
			lines.push_back(line);
		}
	}
	return true;
}

/* TODO: 
* future promise implementation for
* gpiod_line_event_wait(line, &ts)
* gpiod_line_event_read(line, &event)
* https://github.com/starnight/libgpiod-example/blob/master/libgpiod-event/main.c
* https://github.com/starnight/libgpiod-example
* 
* Create managed pointer to weird cstyle structs
*/