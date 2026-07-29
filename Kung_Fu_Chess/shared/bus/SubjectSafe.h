#pragma once
#include <cctype>
#include <string>

// NATS subjects can't contain "." (it's the token separator) or whitespace -
// a shard address like "ws://gameserver-a:9002" needs sanitizing before it's
// usable as one token. allocator_main.cpp (requesting) and
// MessageDispatcher.cpp (subscribing) both apply this same transform to the
// same raw SHARD_ADDRESS, so they always agree on the resulting subject
// without coordinating anything else - previously kept as two independent
// copies, unified here so they can't silently drift apart.
inline std::string subjectSafe(const std::string& raw) {
    std::string safe = raw;
    for (char& c : safe)
        if (!std::isalnum(static_cast<unsigned char>(c)))
            c = '_';
    return safe;
}
