#pragma once
#include "../global.hpp"


namespace pinguqueen::core {

    struct FileInfo {
        std::string _file_name;
        u32 _file_size_bytes = 0;

        FileInfo() = default;
        FileInfo(std::string file_name, u32 file_size_bytes)
            : _file_name(std::move(file_name))
            , _file_size_bytes(file_size_bytes) {}




    };
}



