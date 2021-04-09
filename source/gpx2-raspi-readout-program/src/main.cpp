#include "spidevices/gpx2/gpx2.h"
#include "gpio.h"
#include <string>
#include <iostream>
#include <iomanip>
#include <queue>
#include <cmath>

constexpr double max_interval { 10e-8 }; // maximum interval between two stop bits to count as hit
constexpr unsigned wait_timeout { 10000 }; // if no signal from detector for wait_timeout amount of time before restarting the waiting...
constexpr unsigned interrupt_pin { 20 }; // interrupt pin for the falling edge signal coming from GPX2 chip
//constexpr size_t min_buffer = 200; // minimum number of events stored in buffer

void print_hex(const std::string& str) {
	for (auto byte : str) {
		std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned int>(byte) << " ";
	}
	std::cout << std::endl;
}

void process_queue(std::queue<SPI::GPX2_TDC::Meas>& stop0, std::queue<SPI::GPX2_TDC::Meas>& stop1){
	/*
	// debug output, prints first 10 readout values of stop0 channel to see if ordering works
	if (stop0.size() > 50){
		std::cout << "stop0" << std::endl;
		while(stop0.size()>0){
			std::cout << static_cast<int>(stop0.front().stop_channel) << " " << stop0.front().ref_index << " " << stop0.front().stop_result << std::endl;
			stop0.pop();
		}
		std::cout << std::endl;
		std::cout << "stop1" << std::endl;
		while(stop1.size()>0){
			std::cout << static_cast<int>(stop1.front().stop_channel) << " " << stop1.front().ref_index << " " << stop1.front().stop_result << std::endl;
			stop1.pop();
		}
		exit(0);
	}else{
		return;
	}
	*/

	// checks interval between ordered stop0 and stop1 channels.
	if (stop0.size() > 100000 || stop1.size() > 100000){
		std::cerr << "stop0 size: " << stop0.size() << " stop1 size: " <<  stop1.size() << std::endl;
	}
	while (stop0.size() > 0 && stop1.size() > 0){
		auto interval = SPI::GPX2_TDC::diff(stop0.front(),stop1.front());
		while(interval > max_interval && stop0.size() > 0){
			stop0.pop();
			interval = SPI::GPX2_TDC::diff(stop0.front(),stop1.front());
		}
		while(interval < -max_interval && stop0.size() > 0){
			stop1.pop();
			interval = SPI::GPX2_TDC::diff(stop0.front(),stop1.front());
		}
		if (fabs(interval) < max_interval){
			std::cout << interval << std::endl;
		}
	}
}

auto main()->int {
	SPI::GPX2_TDC::GPX2 gpx2{};
	SPI::GPX2_TDC::Config conf{};
	conf.loadDefaultConfig();
	conf.BLOCKWISE_FIFO_READ = 0U;
	conf.COMMON_FIFO_READ = 0U;

	//conf.PIN_ENA_STOP3 = 0;
	//conf.PIN_ENA_STOP4 = 0;
	//conf.HIT_ENA_STOP3 = 0;
	//conf.HIT_ENA_STOP4 = 0;

	gpx2.init();

	int verbosity = 0;

	bool status = gpx2.write_config(conf);
	std::string config = gpx2.read_config();
	if (status && (config == conf.str())) {
		if (verbosity > 1) {
			std::cout << "config written..." << std::endl;
		}
	}
	else {
		std::cout << "tried to write:" << std::endl;
		print_hex(conf.str());
		std::cout << "read back:" << std::endl;
		print_hex(config);
		std::cout << "failed to write config!" << std::endl;
		exit(-1);
	}

	/*
	gpio handler{};
	gpio::setting pin_setting{};
	pin_setting.gpio_pins = std::vector<unsigned int>{ interrupt_pin };

	auto callback = handler.list_callback(pin_setting);

	bool data_available = true;

	handler.start();
	*/

	gpx2.init_reset();
	//auto event = callback->wait_async(std::chrono::milliseconds {wait_timeout});
	std::queue<SPI::GPX2_TDC::Meas> stop0;
	std::queue<SPI::GPX2_TDC::Meas> stop1;
	while (true) {
		/*if (event.wait_for(std::chrono::microseconds(0)) == std::future_status::ready) {
			if (verbosity > 1) {
				auto now = std::chrono::system_clock::now();
                auto time = std::chrono::system_clock::to_time_t(now);
				std::cerr << "event was not detected for " << static_cast<double>(wait_timeout)*0.001 << " seconds (time: " << time << ")"<< std::endl;
			}
			continue;
			}
			data_available = event.get().type == gpio::event::Falling;
			event = callback->wait_async(std::chrono::milliseconds {wait_timeout});
		}
		if (data_available){
		}
		*/
		auto measurements = gpx2.read_results();

		if (measurements.size()>4){
			std::cerr << "read more than 4 channel. This can't be, closing..." << std::endl;
			break;
		}
		
		size_t first{};
		size_t second{};
		bool do_push;

		// do it for stop0 channel
		first = 2;
		second = 0;
		do_push = true;
		if (SPI::GPX2_TDC::diff(measurements[0], measurements[2])>0){
			first = 0;
			second = 2;
		}
		if (measurements[first].status == SPI::GPX2_TDC::Meas::Valid){
			if (stop0.size()>0){
				if (diff(stop0.back(), measurements[first]) == 0.){
					do_push = false;
				}
			}
			if (do_push){
				stop0.push(measurements[first]);				
			}
		}
		do_push = true;
		if (measurements[second].status == SPI::GPX2_TDC::Meas::Valid){
			if (stop0.size()>0){
				if (diff(stop0.back(), measurements[second]) == 0.){
					do_push = false;
				}
			}
			if (do_push){
				stop0.push(measurements[second]);				
			}
		}

		// do it for stop1 channel
		first = 3;
		second = 1;
		do_push = true;
		if (SPI::GPX2_TDC::diff(measurements[0], measurements[2])>0){
			first = 1;
			second = 3;
		}
		if (measurements[first].status == SPI::GPX2_TDC::Meas::Valid){
			if (stop1.size()>0){
				if (diff(stop1.back(), measurements[first]) == 0.){
					do_push = false;
				}
			}
			if (do_push){
				stop1.push(measurements[first]);				
			}
		}
		do_push = true;
		if (measurements[second].status == SPI::GPX2_TDC::Meas::Valid){
			if (stop1.size()>0){
				if (diff(stop1.back(), measurements[second]) == 0.){
					do_push = false;
				}
			}
			if (do_push){
				stop1.push(measurements[second]);				
			}
		}
		
		process_queue(stop0, stop1);
		/*
		for (auto meas : measurements){
			if (meas.status == SPI::GPX2_TDC::Meas::Valid){
				//std::cout << static_cast<int>(meas.stop_channel) << " " << meas.ref_index << " " << meas.stop_result << std::endl;
			}
		}
		*/
	}
	/*
	handler.stop();
	handler.join();
	*/
}