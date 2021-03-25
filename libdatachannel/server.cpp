/*
 * libdatachannel echo server
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
#include <string>
#include <future>
#include <iostream>
#include <memory>

namespace http = httplib;
using json = nlohmann::json;

using namespace std::chrono_literals;

int main(int argc, char **argv) try {
	const std::string host = "0.0.0.0";
	const int port = argc > 1 ? std::atoi(argv[1]) :  8080;

	rtc::InitLogger(rtc::LogLevel::Warning);

	http::Server srv;
	srv.Post("/offer", [](const httplib::Request &req, httplib::Response &res) {
		auto parsed = json::parse(req.body);
		rtc::Description remote(parsed["sdp"].get<std::string>(), parsed["type"].get<std::string>());

		auto pc = std::make_shared<rtc::PeerConnection>(rtc::Configuration{});

		std::promise<std::string> promise;
		auto future = promise.get_future();

		pc->onGatheringStateChange([pc, &promise](rtc::PeerConnection::GatheringState state) {
			if (state == rtc::PeerConnection::GatheringState::Complete) {
				try {
					auto local = pc->localDescription().value();
					json msg;
					msg["sdp"] = std::string(local);
					msg["type"] = local.typeString();
					promise.set_value(msg.dump());

				} catch (...) {
					promise.set_exception(std::current_exception());
				}
			}
		});

		pc->onStateChange([pc](rtc::PeerConnection::State state) {
			if (state == rtc::PeerConnection::State::Disconnected) {
				pc->close();
			}
		});

		pc->onDataChannel([](std::shared_ptr<rtc::DataChannel> dc) {
			dc->onMessage([dc](auto msg) { dc->send(msg); });
		});

		pc->onTrack([](std::shared_ptr<rtc::Track> tr) {
			tr->onMessage([tr](auto msg) { tr->send(msg); });
		});

		pc->setRemoteDescription(std::move(remote));

		auto body = future.get();
		res.set_content(body, "application/json");
	});

	srv.set_exception_handler([](const http::Request &req, http::Response &res, std::exception &e) {
		res.status = 500;
		res.set_content("500 Internal Server Error", "text/plain");
	});

	std::cout << "Listening on " << host << ":" << port << "..." << std::endl;
	if (!srv.listen(host.c_str(), port))
		throw std::runtime_error("Failed to listen on port " + std::to_string(port));

	return 0;

} catch (const std::exception &e) {
	std::cerr << "Fatal error: " << e.what() << std::endl;
	return -1;
}
