#include "game/Save.h"

#include <cstdio>

#include "core/Log.h"

namespace ls {

namespace {

std::string recordPath(const std::string& directory)
{
    if (directory.empty()) return "lastShooter_record.txt";
    const char last = directory.back();
    if (last == '\\' || last == '/') return directory + "lastShooter_record.txt";
    return directory + "\\" + "lastShooter_record.txt";
}

} // namespace

Record loadRecord(const std::string& directory)
{
    Record record;
    const std::string path = recordPath(directory);
    FILE* file = std::fopen(path.c_str(), "rb");
    if (file == nullptr) return record;

    char buffer[512]{};
    const size_t read = std::fread(buffer, 1, sizeof(buffer) - 1, file);
    std::fclose(file);
    if (read == 0) return record;

    // 极简 key=value 解析
    const std::string content(buffer);
    auto parse = [&content](const char* key, int fallback) {
        const std::string needle = std::string(key) + "=";
        const size_t pos = content.find(needle);
        if (pos == std::string::npos) return fallback;
        return std::atoi(content.c_str() + pos + needle.size());
    };
    record.bestScore = parse("best_score", 0);
    record.bestWave = parse("best_wave", 0);
    record.bestKills = parse("best_kills", 0);
    record.runs = parse("runs", 0);
    LS_LOG("record loaded: score=%d wave=%d kills=%d runs=%d", record.bestScore, record.bestWave, record.bestKills,
           record.runs);
    return record;
}

bool saveRecord(const std::string& directory, const Record& record)
{
    const std::string path = recordPath(directory);
    FILE* file = std::fopen(path.c_str(), "wb");
    if (file == nullptr) {
        LS_LOG("cannot write record to %s", path.c_str());
        return false;
    }
    std::fprintf(file, "best_score=%d\nbest_wave=%d\nbest_kills=%d\nruns=%d\n", record.bestScore, record.bestWave,
                 record.bestKills, record.runs);
    std::fclose(file);
    return true;
}

} // namespace ls
