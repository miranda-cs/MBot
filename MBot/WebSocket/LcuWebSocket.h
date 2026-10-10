#pragma once

#include <functional>
#include <string>

#include <json/json.h>

#include "Auth.h"

namespace League
{
	struct LcuEvent
	{
		std::string eventType;
		std::string uri;
		Json::Value data;
	};

	class LcuWebSocket
	{
	public:
		using EventHandler = std::function<void(const LcuEvent&)>;
		using ConnectedHandler = std::function<void(const ClientInfo&)>;

		LcuWebSocket(EventHandler eventHandler, ConnectedHandler connectedHandler);

		// Keeps the event connection alive and reconnects when the League Client restarts.
		void Run();

	private:
		bool ConnectAndListen(const ClientInfo& clientInfo) const;
		void Dispatch(const std::string& message) const;

		EventHandler eventHandler_;
		ConnectedHandler connectedHandler_;
	};
}
