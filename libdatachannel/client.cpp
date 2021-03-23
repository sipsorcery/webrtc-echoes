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
#include <future>
#include <iostream>
#include <memory>

namespace http = httplib;
using json = nlohmann::json;

using namespace std::chrono_literals;

int main(int argc, char **argv) try {
	const std::string server = argc > 1 ? argv[1] : "http://localhost:8080";
	const std::string message = "Hello world!";

	rtc::InitLogger(rtc::LogLevel::Warning);

	http::Client cl(server.c_str());
	rtc::PeerConnection pc{rtc::Configuration{}};

	std::promise<void> promise;
	auto future = promise.get_future();

	pc.onGatheringStateChange([&pc, &cl, &promise](rtc::PeerConnection::GatheringState state) {
		if (state == rtc::PeerConnection::GatheringState::Complete) {
			try {
				auto local = pc.localDescription().value();
				json msg;
				msg["sdp"] = std::string(local);
				msg["type"] = local.typeString();

				auto res = cl.Post("/offer", msg.dump().c_str(), "application/json");
				if (!res)
					throw std::runtime_error("HTTP request failed");

				if (res->status != 200)
					throw std::runtime_error("HTTP request failed with status " +
					                         std::to_string(res->status));

				auto parsed = json::parse(res->body);
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

	auto dc = pc.createDataChannel("echo");

	dc->onOpen([dc, message]() { dc->send(message); });

	dc->onMessage([message, &promise](auto msg) {
		if (std::holds_alternative<std::string>(msg)) {
			auto str = std::get<std::string>(msg);
			if (str == message)
				promise.set_value();
			else
				promise.set_exception(
				    std::make_exception_ptr(std::runtime_error("Echo test failed")));
		}
	});

	if (future.wait_for(10s) != std::future_status::ready) {
		if (dc->isOpen())
			throw std::runtime_error("Timeout waiting on echo message");
		else if (pc.state() == rtc::PeerConnection::State::Connected)
			throw std::runtime_error("Timeout waiting on Data Channel opening");
		else
			throw std::runtime_error("Timeout waiting on Peer Connection");
	}
	return 0;

} catch (const std::exception &e) {
	std::cerr << "Error: " << e.what() << std::endl;
	return -1;
}
