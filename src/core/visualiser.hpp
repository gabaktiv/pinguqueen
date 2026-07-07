//
// Created by gabriel on 7/3/26.
//
#pragma once
#include "../global.hpp"
#include "../radix-trie/radix-trie.hpp"

#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>

#include <string>
#include <vector>

namespace pinguqueen::core
{
    class Visualiser {
        static constexpr int PAGE_SIZE = 50;

        datastructs::RadixTrie& _trie;
        std::string _search;
        std::string _prev_search;
        std::vector<std::string> _results;
        std::vector<std::string> _displayed;
        int _page = 0;
        int _total_pages = 0;
        ftxui::Component _input;
        ftxui::Component _menu;
        int _menu_selected = 0;

        void update_results();
        void update_page();
        ftxui::Element render();

     public:
        explicit Visualiser(datastructs::RadixTrie& trie);
        ~Visualiser();
        Visualiser(const Visualiser&) = delete;
        Visualiser& operator=(const Visualiser&) = delete;
        Visualiser(Visualiser&&) = default;
        Visualiser& operator=(Visualiser&&) = delete;

        void run();
    };
}
