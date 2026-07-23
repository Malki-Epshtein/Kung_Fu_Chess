#pragma once
// bundled fmt (spdlog's dependency) statically asserts that /utf-8 is set
// unless told otherwise - this project doesn't set that project-wide (and
// changing it project-wide risks mojibake in source files with existing
// non-ASCII, e.g. Hebrew, comments that aren't necessarily saved the way
// fmt expects), so opt fmt's strict Unicode check out instead. Must come
// before the first fmt/spdlog header is ever included anywhere.
#ifndef FMT_UNICODE
#define FMT_UNICODE 0
#endif
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"
// Lets spdlog's {}-style formatting fall back to an existing operator<<
// (e.g. Position's, in shared/model/Position.h) instead of requiring a
// fmt::formatter specialization for every custom type logged anywhere in
// the project.
#include "spdlog/fmt/ostr.h"
#include "../model/Position.h"
#include <memory>
#include <string>
#include <vector>

// fmt (spdlog's formatting backend) requires each custom type logged via
// {} to opt in explicitly - ostr.h only provides the machinery, not an
// automatic fallback to an existing operator<< (fmt deprecated that for
// safety). One line per type actually logged is all that's needed; add
// another here if a future spdlog call logs some other custom type.
template <> struct fmt::formatter<Position> : fmt::ostream_formatter {};

// Shared by both server_main and client_main - this is logging
// infrastructure, not game rules, so it doesn't cross the "server is the
// sole rules authority" boundary the rest of this codebase's layering
// enforces (same reasoning MessageCodec/EventBus/Piece already rely on).
namespace Log {
    // Call once, as the very first thing in main() - sets up the default
    // spdlog logger with two sinks: a colored console sink (dev visibility,
    // replaces the old std::cout output) and a plain file sink (so a
    // session's full traffic survives after the console window closes,
    // production-style persistence). `name` becomes both the logger's
    // registry name (shown in every log line, replacing the old manual
    // "[server]"/"[client]" prefix) and its log file's base name
    // ("server.log"/"client.log", next to the exe - same no-subfolder
    // convention users.db already uses).
    inline void init(const std::string& name) {
        auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(name + ".log", true);
        auto logger = std::make_shared<spdlog::logger>(
            name, spdlog::sinks_init_list{ consoleSink, fileSink });
        logger->set_pattern("%Y-%m-%d %H:%M:%S.%e [%n] [%l] %v");
        // debug (not the spdlog default of info) so per-message traffic
        // traces are visible out of the box in this dev build - toggle to
        // info at the call site below if that ever gets too noisy, no
        // rebuild required (spdlog levels are runtime-settable).
        logger->set_level(spdlog::level::debug);
        // spdlog buffers file-sink writes by default - flush after every
        // line rather than only on a clean shutdown/buffer-full, since the
        // whole point of the file sink is a session's traffic surviving
        // even an unclean exit (crash, force-kill).
        logger->flush_on(spdlog::level::debug);
        spdlog::set_default_logger(logger);
    }
}
