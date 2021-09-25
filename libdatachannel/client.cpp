/*
 * libdatachannel echo client
 * Copyright (c) 2021 Paul-Louis Ageneau
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; If not, see <http://www.gnu.org/licenses/>.
 */

#include <httplib.h>
#include <nlohmann/json.hpp>
#include <rtc/rtc.hpp>

#include <chrono>
#include <cstddef>
#include <future>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <vector>

namespace http = httplib;
using json = nlohmann::json;

using namespace std::chrono_literals;

std::string randomString(std::size_t length) {
	static const std::string chars = "0123456789abcdefghijklmnopqrstuvwxyz";
	std::default_random_engine rng(std::random_device{}());
	std::uniform_int_distribution<int> dist(0, int(chars.size() - 1));
	std::string result(length, '\0');
	std::generate(result.begin(), result.end(), [&]() { return chars[dist(rng)]; });
	return result;
}

int main(int argc, char **argv) try {
	// Default arguments
	int test = 0;
	std::string url = "http://localhost:8080/offer";

	// Parse arguments
	for (int i = 1; i < argc; ++i) {
		std::string arg = argv[i];
		if (!arg.empty() && arg[0] == '-') {
			std::string option = arg.substr(1);
			if (option == "h") {
				std::cout
				    << "Usage: " << argv[0] << "[URL|<options>]\n"
				    << "Options:\n"
				    << "\t-h,\t\tShow this help message\n"
				    << "\t-t NUMBER\tSpecify the test number (default 0)\n"
				    << "\t-s URL\t\tSpecify the server URL (default http://localhost:8080/offer)\n"
				    << std::endl;
				return 0;
			} else if (option == "t") {
				if (i + 1 == argc)
					throw std::invalid_argument("Missing argument for option \"t\"");
				test = std::atoi(argv[++i]);
			} else if (option == "s") {
				if (i + 1 == argc)
					throw std::invalid_argument("Missing argument for option \"s\"");
				url = argv[++i];
			} else {
				throw std::invalid_argument("Unknown option \"" + option + "\"");
			}
		} else {
			if (i > 1)
				throw std::invalid_argument("Unexpected positional argument \"" + arg + "\"");
			url = std::move(arg);
		}
	}

	if (test != 0 && test != 1)
		throw std::invalid_argument("Invalid test number");

	const size_t separator = url.find_last_of('/');
	if (separator == std::string::npos)
		throw std::invalid_argument("Invalid URL");

	const std::string server = url.substr(0, separator);
	const std::string path = url.substr(separator);

	std::promise<void> promise;
	auto future = promise.get_future();

	http::Client cl(server.c_str());

	rtc::InitLogger(rtc::LogLevel::Warning);

	// Set up Peer Connection
	rtc::Configuration config;
	config.disableAutoNegotiation = true;
	rtc::PeerConnection pc{std::move(config)};

	pc.onGatheringStateChange([url, path, &pc, &cl,
	                           &promise](rtc::PeerConnection::GatheringState state) {
		if (state == rtc::PeerConnection::GatheringState::Complete) {
			try {
				auto local = pc.localDescription().value();
				json msg;
				msg["sdp"] = std::string(local);
				msg["type"] = local.typeString();

				auto dumped = msg.dump();
				auto res = cl.Post(path.c_str(), dumped.c_str(), "application/json");
				if (!res)
					throw std::runtime_error("HTTP request to " + url +
					                         " failed; error code: " + std::to_string(res.error()));

				if (res->status != 200)
					throw std::runtime_error("HTTP POST failed with status " +
					                         std::to_string(res->status) + "; content: " + dumped);
				json parsed;
				try {
					parsed = json::parse(res->body);

				} catch (const std::exception &e) {
					throw std::runtime_error("HTTP response parsing failed: " +
					                         std::string(e.what()) + "; content: " + res->body);
				}

				rtc::Description remote(parsed["sdp"].get<std::string>(),
				                        parsed["type"].get<std::string>());
				pc.setRemoteDescription(std::move(remote));

			} catch (...) {
				promise.set_exception(std::current_exception());
			}
		}
	});

	pc.onStateChange([&pc, &promise](rtc::PeerConnection::State state) {
		if (state == rtc::PeerConnection::State::Disconnected) {
			pc.close();
			promise.set_exception(
			    std::make_exception_ptr(std::runtime_error("Peer Connection failed")));
		}
	});

	std::shared_ptr<rtc::Track> tr;
	std::shared_ptr<rtc::DataChannel> dc;
	if (test == 0) { // Peer Connection test
		rtc::Description::Video media("echo", rtc::Description::Direction::SendRecv);
		media.addVP8Codec(96);
		tr = pc.addTrack(std::move(media));

		// TODO: send media
		tr->onOpen([&promise]() { promise.set_value(); });

	} else { // test == 1 Data Channel test
		auto test = randomString(5);
		dc = pc.createDataChannel("echo");

		dc->onOpen([test, &dc]() { dc->send(test); });

		dc->onMessage([test, &promise](auto message) {
			try {
				if (!std::holds_alternative<std::string>(message))
					throw std::runtime_error("Received unexpected binary message");

				auto str = std::get<std::string>(message);
				if (str != test)
					throw std::runtime_error("Received unexpected message \"" + str + "\"");

				promise.set_value();

			} catch (const std::exception &e) {
				promise.set_exception(std::make_exception_ptr(e));
			}
		});
	}

	pc.setLocalDescription(rtc::Description::Type::Offer);

	if (future.wait_for(10s) != std::future_status::ready)
		throw std::runtime_error("Timeout");

	future.get();
	return 0;

} catch (const std::exception &e) {
	std::cerr << "Error: " << e.what() << std::endl;
	return -1;
}
