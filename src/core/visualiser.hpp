#pragma once
#include "../global.hpp"
#include "../adaptive-radix-trie/adaptive-radix-trie.hpp"

#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>
#include <string>
#include <vector>

namespace pinguqueen::core
{
    enum class SearchMode { Prefix, Substring };

    class Visualiser {
        static constexpr int PAGE_SIZE = 50;

        datastructs::AdaptiveRadixTrie& _trie;
        std::string _search;
        std::string _prev_search;
        std::vector<std::string> _results;
        std::vector<std::string> _displayed;
        int _page = 0;
        int _total_pages = 0;
        ftxui::Component _input;
        ftxui::Component _menu;
        int _menu_selected = 0;
        SearchMode _mode = SearchMode::Prefix;

        void update_results();
        void update_page();
        ftxui::Element render();
        [[nodiscard]] const char* mode_label() const noexcept;

     public:
        explicit Visualiser(datastructs::AdaptiveRadixTrie& trie);
        ~Visualiser();
        Visualiser(const Visualiser&) = delete;
        Visualiser& operator=(const Visualiser&) = delete;
        Visualiser(Visualiser&&) = default;
        Visualiser& operator=(Visualiser&&) = delete;

        void run();
    };
}
