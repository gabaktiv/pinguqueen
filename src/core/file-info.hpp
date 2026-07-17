#pragma once
#include "../global.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace pinguqueen::core
{

    struct FileInfo {
        std::string _file_name;
        u32 _file_size_bytes = 0;
        std::filesystem::file_time_type _last_write;

        FileInfo() = default;
        FileInfo(std::string fileName, u32 fileSizeBytes,
                 std::filesystem::file_time_type lastWrite = {})
            : _file_name(std::move(fileName))
            , _file_size_bytes(fileSizeBytes)
            , _last_write(lastWrite) {}

        [[nodiscard]] std::vector<std::string> to_string() const;
    };
}
