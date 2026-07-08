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
        FileInfo(std::string file_name, u32 file_size_bytes,
                 std::filesystem::file_time_type last_write = {})
            : _file_name(std::move(file_name))
            , _file_size_bytes(file_size_bytes)
            , _last_write(last_write) {}

        [[nodiscard]] std::vector<std::string> to_string() const;
    };
}
