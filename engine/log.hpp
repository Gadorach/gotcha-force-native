#pragma once
// Tiny logging facade over ILogBackend.
//
// Exists so base code never calls printf or a platform logger directly; the
// backend decides where the text goes. printf-style because that is what
// existing ported code already reads like.
#include <cstdarg>
#include <cstdio>

#include "engine/core/backends.hpp"
#include "engine/core/context.hpp"

namespace gf {
namespace detail {

inline void log_write(Context& context, LogLevel level, const char* category, const char* format,
                      std::va_list args) {
    char buffer[1024];
    std::vsnprintf(buffer, sizeof(buffer), format, args);
    if (ILogBackend* log = context.backends().log) log->write(level, category, buffer);
}

} // namespace detail

#define GF_LOG_FN(name, level)                                                       \
    inline void name(Context& context, const char* category, const char* format, ...) { \
        std::va_list args;                                                            \
        va_start(args, format);                                                       \
        detail::log_write(context, level, category, format, args);                     \
        va_end(args);                                                                 \
    }

GF_LOG_FN(log_trace, LogLevel::Trace)
GF_LOG_FN(log_debug, LogLevel::Debug)
GF_LOG_FN(log_info, LogLevel::Info)
GF_LOG_FN(log_warn, LogLevel::Warn)
GF_LOG_FN(log_error, LogLevel::Error)

#undef GF_LOG_FN

} // namespace gf
