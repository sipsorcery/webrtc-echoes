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
#include <string>
#include <vector>

namespace http = httplib;
using json = nlohmann::json;

using namespace std::chrono_literals;

int main(int argc, char **argv) try {
	const std::string url = argc > 1 ? argv[1] : "http://localhost:8080/offer";

	const size_t separator = url.find_last_of('/');
	const std::string server = url.substr(0, separator);
	const std::string path = url.substr(separator);

	rtc::InitLogger(rtc::LogLevel::Warning);

	http::Client cl(server.c_str());

	rtc::Configuration config;
	config.disableAutoNegotiation = true;
	rtc::PeerConnection pc{std::move(config)};

	std::promise<void> promise;
	auto future = promise.get_future();

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

	rtc::Description::Video media("echo", rtc::Description::Direction::SendRecv);
	media.addVP8Codec(96);
	auto tr = pc.addTrack(std::move(media));

	// TODO: send media
	tr->onOpen([&promise]() { promise.set_value(); });

	pc.setLocalDescription(rtc::Description::Type::Offer);

	if (future.wait_for(10s) != std::future_status::ready)
		throw std::runtime_error("Timeout");

	future.get();
	return 0;

} catch (const std::exception &e) {
	std::cerr << "Error: " << e.what() << std::endl;
	return -1;
}
