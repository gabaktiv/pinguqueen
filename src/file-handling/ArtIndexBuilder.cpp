//
// Created by gabriel on 7/2/26.
//

#include "ArtIndexBuilder.hpp"

namespace pinguqueen::file {


    void ArtIndexHandler::init()
    {
        namespace fs = std::filesystem;

        std::error_code ec;

        _root = fs::current_path(ec);
        if (ec) {
            return;
        }

        _root = fs::weakly_canonical(_root, ec);
        if (ec) {
            return;
        }

        if (!fs::is_directory(_root, ec) || ec) {
            return;
        }

        const auto options = fs::directory_options::skip_permission_denied;

        fs::recursive_directory_iterator it(_root, options, ec);
        fs::recursive_directory_iterator end;

        if (ec) {
            return;
        }

        while (it != end) {
            const fs::directory_entry& entry = *it;

            ec.clear();

            if (entry.is_regular_file(ec) && !ec) {
                ec.clear();

                auto size = entry.file_size(ec);
                if (!ec) {
                    std::filesystem::path relative =
                    std::filesystem::relative(entry.path(), _root, ec);

                    if (!ec) {
                        auto* FileMetaData = new core::FileInfo(relative.generic_string(),static_cast<u32>(size));
                        _art.insert(relative.generic_string(), FileMetaData);
                    }
                }
            }

            ec.clear();
            it.increment(ec);

            if (ec) {
                ec.clear();
            }
        }
    }






}

