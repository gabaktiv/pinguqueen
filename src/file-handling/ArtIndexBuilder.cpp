#include "ArtIndexBuilder.hpp"

namespace pinguqueen::file {


    //init function scans the computer where the Programm was called and puts the filenames as keys and the metadata (fileinfo) as values into his ART.

    void ArtIndexBuilder::init()
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

            // don't recurse into hidden directories (.cache, .git, .config, ...)
            if (entry.is_directory(ec) && !ec) {
                auto const& fn = entry.path().filename().generic_string();
                if (!fn.empty() && fn[0] == '.') {
                    it.disable_recursion_pending();
                }
                ec.clear();
                it.increment(ec);
                if (ec) ec.clear();
                continue;
            }
            ec.clear();

            if (entry.is_regular_file(ec) && !ec) {
                ec.clear();

                auto size = entry.file_size(ec);
                if (!ec) {
                    std::filesystem::path relative = std::filesystem::relative(entry.path(), _root, ec);
                    if (!ec) {
                        relative = relative.lexically_normal();
                        auto last_write = entry.last_write_time(ec);
                        if (ec) last_write = {};
                        _art.insert(
                             relative.generic_string(),
                            std::make_unique<core::FileInfo>(relative.generic_string(),
                                 static_cast<u32>(size), last_write)
                        );
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

