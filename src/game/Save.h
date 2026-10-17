// =============================================================================
//  game/Save.h  —  本地记录（最高分/最高波次）
//  说明：原项目没有任何记录，每局结束直接 exit(0)。
//        这里用一个纯文本文件存最高分，读失败就当 0，绝不影响启动。
// =============================================================================
#pragma once

#include <string>

namespace ls {

struct Record
{
    int bestScore = 0;
    int bestWave = 0;
    int bestKills = 0;
    int runs = 0;
};

Record loadRecord(const std::string& directory);
bool saveRecord(const std::string& directory, const Record& record);

} // namespace ls
