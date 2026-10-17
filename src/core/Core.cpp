#include "core/Log.h"
#include "core/Random.h"

#include <cstdarg>
#include <cstdio>

namespace ls {

namespace {
bool g_logEnabled = true;
Rng g_rng(0xC0FFEEu);
} // namespace

void setLogEnabled(bool enabled) { g_logEnabled = enabled; }
bool logEnabled() { return g_logEnabled; }

void logLine(const char* format, ...)
{
    if (!g_logEnabled || format == nullptr) return;
    va_list args;
    va_start(args, format);
    std::vprintf(format, args);
    va_end(args);
    std::printf("\n");
    std::fflush(stdout);
}

Rng& rng() { return g_rng; }

} // namespace ls
