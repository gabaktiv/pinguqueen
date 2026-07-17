#include "visualiser.hpp"

#include <ftxui/component/screen_interactive.hpp>

namespace pinguqueen::core
{

    Visualiser::Visualiser(datastructs::AdaptiveRadixTrie& trie)
        : _trie(trie), _prev_search("\1") {}

    Visualiser::~Visualiser() = default;

    // Recalculates total page count and slices _results into _displayed for the current page.
    void Visualiser::update_page()
    {
        _total_pages = std::max(1, (static_cast<int>(_results.size()) + PAGE_SIZE - 1) / PAGE_SIZE);
        if (_page >= _total_pages) {
            _page = _total_pages - 1;
        }

        auto start = static_cast<std::size_t>(_page * PAGE_SIZE);
        auto end = std::min(start + PAGE_SIZE, _results.size());
        _displayed.assign(_results.begin() + static_cast<std::ptrdiff_t>(start),
                          _results.begin() + static_cast<std::ptrdiff_t>(end));
    }

    // Re-queries the trie when the search input has changed and strips terminal symbols from results.
    void Visualiser::update_results()
    {
        if (_search == _prev_search) {
            return;
        }
        _prev_search = _search;

        auto newResults = (_mode == SearchMode::Prefix)
            ? _trie.get_all_paths_with_prefix(_search)
            : _trie.get_all_paths_with_substring(_search);
        for (auto& result : newResults) {
            auto const pos = result.find('\0');
            if (pos != std::string::npos) {
                result.resize(pos);
            }
        }
        if (newResults != _results) {
            _results = std::move(newResults);
            _page = 0;
            _menu_selected = 0;
            update_page();
        }
    }

    const char* Visualiser::mode_label() const noexcept
    {
        return _mode == SearchMode::Prefix ? " [Prefix]" : " [Substr]";
    }

    // Builds the ftxui layout with search input, paginated result list, and file info panel.
    ftxui::Element Visualiser::render()
    {
        auto pageStr = "Page " + std::to_string(_page + 1) + "/" + std::to_string(_total_pages);
        auto left = ftxui::vbox({
            ftxui::hbox(
                ftxui::text(mode_label()) | ftxui::bold | ftxui::color(ftxui::Color::Cyan),
                ftxui::text(" 🔍 "), _input->Render()) | ftxui::border,
            ftxui::separator(),
            _menu->Render()
                | ftxui::vscroll_indicator
                | ftxui::frame
                | ftxui::flex,
            ftxui::text(pageStr) | ftxui::center | ftxui::bold,
        });

        if (_displayed.empty()
            || _menu_selected < 0
            || _menu_selected >= static_cast<int>(_displayed.size()))
        {
            return left;
        }

        auto resultIndex = static_cast<std::size_t>(_page * PAGE_SIZE + _menu_selected);
        if (resultIndex >= _results.size()) {
            return left;
        }

        auto* info = _trie.search(_results[resultIndex]);
        if (!info) {
            return left;
        }

        auto lines = info->to_string();
        ftxui::Elements infoElems;
        infoElems.reserve(lines.size() + 3);
        infoElems.push_back(ftxui::text(" ℹ️  File Info") | ftxui::bold);
        infoElems.push_back(ftxui::separator());
        for (auto const& line : lines) {
            infoElems.push_back(ftxui::paragraph(line));
        }

        return ftxui::hbox({
            left | ftxui::flex,
            ftxui::separator(),
            ftxui::vbox(std::move(infoElems))
                | ftxui::border
                | ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 40),
        });
    }

    // Sets up the fullscreen TUI with input, menu, and keyboard event handling (Tab for paging, F2 for mode toggle).
    void Visualiser::run()
    {
        auto screen = ftxui::ScreenInteractive::Fullscreen();

        _input = ftxui::Input(&_search, "search...");
        _menu = ftxui::Menu(&_displayed, &_menu_selected);

        auto container = ftxui::Container::Vertical({_input, _menu});

        auto app = ftxui::CatchEvent(container, [&](const ftxui::Event& event) {
            if (event == ftxui::Event::Tab || event == ftxui::Event::TabReverse) {
                _page = (_page + 1) % _total_pages;
                _menu_selected = 0;
                update_page();
                return true;
            }
            if (event == ftxui::Event::F2) {
                _mode = (_mode == SearchMode::Prefix) ? SearchMode::Substring : SearchMode::Prefix;
                _prev_search = "\1";
                return true;
            }
            return false;
        });

        auto renderer = ftxui::Renderer(app, [&] {
            update_results();
            return render();
        });

        screen.Loop(renderer);
    }

}
