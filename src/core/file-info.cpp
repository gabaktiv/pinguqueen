#include "file-info.hpp"

#include <chrono>
#include <iomanip>
#include <sstream>

namespace pinguqueen::core {

    // Formats the file metadata as a list of human-readable strings (name, size, modification date).
    std::vector<std::string> FileInfo::to_string() const
    {
        std::vector<std::string> lines;
        lines.reserve(4);
        lines.push_back("Name: " + _file_name);
        lines.push_back("Size: " + std::to_string(_file_size_bytes) + " bytes");

        if (_last_write != std::filesystem::file_time_type::min()) {
            // Convert file_time_type to system_clock time_point for formatting.
            // The arithmetic reconstructs the system_clock time by measuring the offset
            // between the file clock's "now" and system_clock's "now".
            auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                _last_write - std::filesystem::file_time_type::clock::now()
                + std::chrono::system_clock::now());
            auto tt = std::chrono::system_clock::to_time_t(sctp);
            auto tm = *std::localtime(&tt);
            std::ostringstream oss;
            oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
            lines.push_back("Modified: " + oss.str());
        }

        return lines;
    }

}
