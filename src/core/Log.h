// =============================================================================
//  core/Log.h  —  极简日志（编译期可关闭）
// =============================================================================
#pragma once

namespace ls {

// 打开/关闭日志输出（控制台被隐藏时仍然有效，只是看不见）
void setLogEnabled(bool enabled);
bool logEnabled();

// printf 风格；自动换行
void logLine(const char* format, ...);

} // namespace ls

#ifdef LS_DEBUG
#define LS_LOG(...) ::ls::logLine(__VA_ARGS__)
#else
#define LS_LOG(...) ((void)0)
#endif
