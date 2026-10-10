#include "LcuWebSocket.h"

#include <Windows.h>
#include <winhttp.h>

#include <array>
#include <chrono>
#include <memory>
#include <thread>
#include <utility>

#include "LCU.h"

#pragma comment(lib, "winhttp.lib")

namespace
{
	class InternetHandle
	{
	public:
		explicit InternetHandle(const HINTERNET handle = nullptr) : handle_(handle)
		{
		}

		~InternetHandle()
		{
			if (handle_)
				WinHttpCloseHandle(handle_);
		}

		InternetHandle(const InternetHandle&) = delete;
		InternetHandle& operator=(const InternetHandle&) = delete;

		[[nodiscard]] HINTERNET Get() const
		{
			return handle_;
		}

	private:
		HINTERNET handle_;
	};

	std::wstring ToWideAscii(const std::string& value)
	{
		return { value.begin(), value.end() };
	}
}

League::LcuWebSocket::LcuWebSocket(EventHandler eventHandler, ConnectedHandler connectedHandler) :
	eventHandler_(std::move(eventHandler)), connectedHandler_(std::move(connectedHandler))
{
}

void League::LcuWebSocket::Run()
{
	while (true)
	{
		const ClientInfo clientInfo = LCU::league;
		if (clientInfo.port <= 0 || clientInfo.token.empty())
		{
			std::this_thread::sleep_for(std::chrono::seconds(1));
			continue;
		}

		ConnectAndListen(clientInfo);
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}
}

bool League::LcuWebSocket::ConnectAndListen(const ClientInfo& clientInfo) const
{
	const InternetHandle internet(WinHttpOpen(L"KBotExt/1.0", WINHTTP_ACCESS_TYPE_NO_PROXY,
		WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0));
	if (!internet.Get())
		return false;

	WinHttpSetTimeouts(internet.Get(), 2000, 2000, 5000, 0);

	const InternetHandle connection(WinHttpConnect(internet.Get(), L"127.0.0.1",
		static_cast<INTERNET_PORT>(clientInfo.port), 0));
	if (!connection.Get())
		return false;

	const InternetHandle request(WinHttpOpenRequest(connection.Get(), L"GET", L"/", nullptr,
		WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE));
	if (!request.Get())
		return false;

	DWORD securityFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA | SECURITY_FLAG_IGNORE_CERT_CN_INVALID |
		SECURITY_FLAG_IGNORE_CERT_DATE_INVALID | SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE;
	if (!WinHttpSetOption(request.Get(), WINHTTP_OPTION_SECURITY_FLAGS, &securityFlags, sizeof(securityFlags)) ||
		!WinHttpSetOption(request.Get(), WINHTTP_OPTION_UPGRADE_TO_WEB_SOCKET, nullptr, 0))
	{
		return false;
	}

	const std::wstring authorization = ToWideAscii("Authorization: Basic " + clientInfo.token + "\r\n");
	if (!WinHttpSendRequest(request.Get(), authorization.c_str(), static_cast<DWORD>(-1L),
		WINHTTP_NO_REQUEST_DATA, 0, 0, 0) || !WinHttpReceiveResponse(request.Get(), nullptr))
	{
		return false;
	}

	const InternetHandle webSocket(WinHttpWebSocketCompleteUpgrade(request.Get(), 0));
	if (!webSocket.Get())
		return false;

	static constexpr char subscription[] = R"([5,"OnJsonApiEvent"])";
	if (WinHttpWebSocketSend(webSocket.Get(), WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE,
		const_cast<char*>(subscription), static_cast<DWORD>(sizeof(subscription) - 1)) != NO_ERROR)
	{
		return false;
	}

	if (connectedHandler_)
		connectedHandler_(clientInfo);

	std::array<char, 8192> buffer{};
	std::string message;
	while (true)
	{
		DWORD bytesRead = 0;
		WINHTTP_WEB_SOCKET_BUFFER_TYPE bufferType = WINHTTP_WEB_SOCKET_UTF8_FRAGMENT_BUFFER_TYPE;
		const DWORD error = WinHttpWebSocketReceive(webSocket.Get(), buffer.data(),
			static_cast<DWORD>(buffer.size()), &bytesRead, &bufferType);
		if (error != NO_ERROR || bufferType == WINHTTP_WEB_SOCKET_CLOSE_BUFFER_TYPE)
			return false;

		if (bufferType != WINHTTP_WEB_SOCKET_UTF8_FRAGMENT_BUFFER_TYPE &&
			bufferType != WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE)
		{
			continue;
		}

		message.append(buffer.data(), bytesRead);
		if (bufferType == WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE)
		{
			Dispatch(message);
			message.clear();
		}
	}
}

void League::LcuWebSocket::Dispatch(const std::string& message) const
{
	Json::CharReaderBuilder builder;
	const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	Json::Value envelope;
	JSONCPP_STRING error;
	if (!reader->parse(message.data(), message.data() + message.size(), &envelope, &error) ||
		!envelope.isArray() || envelope.size() < 3 || envelope[0].asInt() != 8 || !envelope[2].isObject())
	{
		return;
	}

	const Json::Value& payload = envelope[2];
	League::LcuEvent event;
	event.eventType = payload["eventType"].asString();
	event.uri = payload["uri"].asString();
	event.data = payload["data"];
	if (eventHandler_ && !event.uri.empty())
		eventHandler_(event);
}
