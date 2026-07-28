#pragma once
#include "json.hpp"
#include <cstdint>
#include <memory>
#include <string>

// Blocking HTTP/1.1 client (WinHTTP) for the API Gateway's REST routes (see
// gateway/ApiGateway.h) - separate from WsClient, which stays the
// WebSocket connection to a game shard. One HttpClient per API Gateway
// host:port; the same instance is reused for every REST call.
//
// Every method returns the response body decoded as JSON, always with at
// least "success"/"message" fields - a transport failure (gateway
// unreachable, malformed response) is folded into that same
// {success:false, message:"..."} shape the gateway already returns for its
// own error cases, so callers never need a separate error path.
class HttpClient {
public:
    HttpClient(const std::string& host, uint16_t port);
    ~HttpClient();

    nlohmann::json get(const std::string& path);
    nlohmann::json post(const std::string& path, const nlohmann::json& body);

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
