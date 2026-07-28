#include "HttpClient.h"
#include <windows.h>
#include <winhttp.h>
#include <vector>

#pragma comment(lib, "winhttp.lib")

struct HttpClient::Impl {
    std::wstring host;
    uint16_t     port = 0;
    HINTERNET    hSession = nullptr;
};

namespace {
    std::wstring toWide(const std::string& s) {
        if (s.empty()) return L"";
        int len = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
        std::wstring result(len, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), result.data(), len);
        return result;
    }

    nlohmann::json transportError(const std::string& message) {
        return { {"success", false}, {"message", message} };
    }

    // Opens a fresh request on `hConnect`, sends `body` (if any) as JSON,
    // and reads back the response - synchronous/blocking, same "one call,
    // one reply" shape as BlockingRequest's sendAndWaitForReply for the
    // WebSocket side.
    nlohmann::json doRequest(HINTERNET hSession, const std::wstring& host, uint16_t port,
                              const wchar_t* method, const std::string& path, const std::string& body) {
        HINTERNET hConnect = WinHttpConnect(hSession, host.c_str(), port, 0);
        if (!hConnect) return transportError("could not connect to server");

        HINTERNET hRequest = WinHttpOpenRequest(hConnect, method, toWide(path).c_str(),
            nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
        if (!hRequest) {
            WinHttpCloseHandle(hConnect);
            return transportError("could not open request");
        }

        static const wchar_t* kHeaders = L"Content-Type: application/json\r\n";
        BOOL sent = body.empty()
            ? WinHttpSendRequest(hRequest, kHeaders, static_cast<DWORD>(-1L), WINHTTP_NO_REQUEST_DATA, 0, 0, 0)
            : WinHttpSendRequest(hRequest, kHeaders, static_cast<DWORD>(-1L),
                  const_cast<char*>(body.data()), static_cast<DWORD>(body.size()), static_cast<DWORD>(body.size()), 0);

        nlohmann::json result;
        if (sent && WinHttpReceiveResponse(hRequest, nullptr)) {
            std::string responseBody;
            DWORD available = 0;
            while (WinHttpQueryDataAvailable(hRequest, &available) && available > 0) {
                std::vector<char> buffer(available);
                DWORD read = 0;
                if (!WinHttpReadData(hRequest, buffer.data(), available, &read)) break;
                responseBody.append(buffer.data(), read);
            }
            try {
                result = nlohmann::json::parse(responseBody);
            } catch (const std::exception&) {
                result = transportError("malformed response from server");
            }
        } else {
            result = transportError("server unreachable");
        }

        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        return result;
    }
}

HttpClient::HttpClient(const std::string& host, uint16_t port) : impl(std::make_unique<Impl>()) {
    impl->host = toWide(host);
    impl->port = port;
    // NO_PROXY - the API Gateway is always localhost or a LAN address in
    // this project's deployments, never worth the proxy-detection overhead
    // WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY would add.
    impl->hSession = WinHttpOpen(L"KungFuChessClient/1.0",
        WINHTTP_ACCESS_TYPE_NO_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
}

HttpClient::~HttpClient() {
    if (impl->hSession) WinHttpCloseHandle(impl->hSession);
}

nlohmann::json HttpClient::get(const std::string& path) {
    if (!impl->hSession) return transportError("http session not initialized");
    return doRequest(impl->hSession, impl->host, impl->port, L"GET", path, "");
}

nlohmann::json HttpClient::post(const std::string& path, const nlohmann::json& body) {
    if (!impl->hSession) return transportError("http session not initialized");
    return doRequest(impl->hSession, impl->host, impl->port, L"POST", path, body.dump());
}
