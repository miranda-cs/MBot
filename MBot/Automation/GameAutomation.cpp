#include "GameAutomation.h"

#include "ChampSelectAutomation.h"
#include "PickOrderSwapAutomation.h"
#include "ReadyCheckAutomation.h"
#include "JsonUtils.h"
#include "LcuHttpClient.h"
#include "LcuWebSocket.h"

#include <atomic>
#include <chrono>
#include <thread>

namespace
{
	bool IsSuccessful(const cpr::Response& response)
	{
		return response.status_code >= 200 && response.status_code < 300;
	}

	class Controller
	{
	public:
		Controller() : readyCheck_(http_), champSelect_(http_), pickOrderSwap_(http_)
		{
		}

		void Run()
		{
			std::thread tracker([this] { TrackChampSelect(); });
			tracker.detach();

			League::LcuWebSocket socket(
				[this](const League::LcuEvent& event) { HandleEvent(event); },
				[this](const ClientInfo& clientInfo) { OnConnected(clientInfo); });
			socket.Run();
		}

	private:
		void OnConnected(const ClientInfo& clientInfo)
		{
			http_.Configure(clientInfo);
			Synchronize();
		}

		void Synchronize()
		{
			Json::Value phase;
			const cpr::Response phaseResponse = http_.Request("GET", "/lol-gameflow/v1/gameflow-phase");
			if (IsSuccessful(phaseResponse) && JsonUtils::Parse(phaseResponse.text, phase))
				HandleGameflowPhase(phase.asString());
		}

		void HandleEvent(const League::LcuEvent& event)
		{
			if (event.uri == "/lol-gameflow/v1/gameflow-phase")
			{
				HandleGameflowPhase(event.data.asString());
				return;
			}

			if (event.uri == "/lol-matchmaking/v1/ready-check")
			{
				readyCheck_.HandleReadyCheck(event.data);
			}
		}

		void HandleGameflowPhase(const std::string& phase)
		{
			readyCheck_.HandlePhase(phase);
			champSelectActive_ = phase == "ChampSelect";
		}

		void HandleChampSelect(const Json::Value& session)
		{
			if (!session.isObject())
				return;

			const Json::Int64 gameId = session["gameId"].asInt64();
			if (gameId != currentGameId_)
			{
				ResetChampSelect();
				currentGameId_ = gameId;
			}

			pickOrderSwap_.Handle(session);
			champSelect_.Handle(session);
		}

		void ResetChampSelect()
		{
			currentGameId_ = 0;
			pickOrderSwap_.Reset();
			champSelect_.Reset();
		}

		bool FetchChampSelect(Json::Value& session)
		{
			const cpr::Response response = http_.Request("GET", "/lol-champ-select/v1/session");
			return IsSuccessful(response) && JsonUtils::Parse(response.text, session) && session.isObject();
		}

		void TrackChampSelect()
		{
			bool wasActive = false;
			while (true)
			{
				const bool isActive = champSelectActive_.load();
				if (isActive)
				{
					wasActive = true;
					Json::Value session;
					if (FetchChampSelect(session))
						HandleChampSelect(session);
				}
				else if (wasActive)
				{
					ResetChampSelect();
					wasActive = false;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}
		}

		League::LcuHttpClient http_;
		GameAutomation::ReadyCheckAutomation readyCheck_;
		GameAutomation::ChampSelectAutomation champSelect_;
		GameAutomation::PickOrderSwapAutomation pickOrderSwap_;
		std::atomic_bool champSelectActive_ = false;
		Json::Int64 currentGameId_ = 0;
	};
}

void GameAutomation::Run()
{
	Controller controller;
	controller.Run();
}
