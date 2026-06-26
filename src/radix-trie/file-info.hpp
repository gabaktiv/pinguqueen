#pragma once
#include "../global.hpp"


namespace pinguqueen::intern {

    struct FileInfo {

        std::string file_name;
        u32 file_size_bytes = 0;
        bool is_directory = false;

        std::vector<FileInfo*>* directory_contents = nullptr;

        ~FileInfo() {
            delete directory_contents;
        }

    };
}



