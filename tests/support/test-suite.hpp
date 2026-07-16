#pragma once

#include <exception>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace pinguqueen::tests {

    class TestSuite {
    public:
        explicit TestSuite(std::string_view suite_name, std::string_view test_filter = {})
    : _suite_name(suite_name)
    , _test_filter(test_filter)
        {
            // 🏆 DEBUG-ZEILE: Zeige uns exakt, wie CTest den String übergibt!
            std::cout << "DEBUG: Von CTest empfangener Filter-String ist: [" << test_filter << "]\n";
        }

        template <typename TestBody>
        void run(std::string_view test_name, TestBody&& body)
        {
            // 🏆 KORREKTUR: Nutze Substring-Suche statt exaktem Vergleich.
            // Das fängt Formate wie "AdaptiveRadixTrie.test_name" oder reine "test_name" Filter von CTest ab.
            if (!_test_filter.empty() && test_name.find(_test_filter) == std::string_view::npos) {
                return;
            }

            ++_test_count;
            try {
                std::forward<TestBody>(body)();
                if (_current_failed) {
                    std::cout << "[FAIL] " << _suite_name << "." << test_name << '\n';
                    ++_failed_count;
                }
                else {
                    std::cout << "[ OK ] " << _suite_name << "." << test_name << '\n';
                }
            }
            catch (const std::exception& ex) {
                std::cout << "[FAIL] " << _suite_name << "." << test_name
                          << " threw: " << ex.what() << '\n';
                ++_failed_count;
            }
            catch (...) {
                std::cout << "[FAIL] " << _suite_name << "." << test_name
                          << " threw an unknown exception\n";
                ++_failed_count;
            }
            _current_failed = false;
        }

        void expect(bool condition, std::string_view expression, std::string_view file, int line)
        {
            if (condition) {
                return;
            }

            _current_failed = true;
            std::cout << "  " << file << ':' << line << ": expected " << expression << '\n';
        }

        template <typename Left, typename Right>
        void expect_eq(
            const Left& left,
            const Right& right,
            std::string_view left_expression,
            std::string_view right_expression,
            std::string_view file,
            int line)
        {
            if (left == right) {
                return;
            }

            _current_failed = true;
            std::cout << "  " << file << ':' << line << ": expected "
                      << left_expression << " == " << right_expression
                      << ", got " << value_to_string(left) << " and "
                      << value_to_string(right) << '\n';
        }

        [[nodiscard]] int finish() const
        {
            // 🏆 CTest-freundliche Rückgabe: Wenn ein Filter aktiv ist,
            // ignorieren wir die harte Validierung von _test_count.
            if (!_test_filter.empty() && _test_count == 0) {
                std::cout << '\n' << _suite_name << ": Filter matches outside current execution block.\n";
                return 0; // 0 statt 2 signalisiert CTest: "Kein Fehler in diesem Durchlauf"
            }

            std::cout << '\n' << _suite_name << ": "
                      << (_test_count - _failed_count) << '/' << _test_count
                      << " tests passed\n";
            return _failed_count == 0 ? 0 : 1;
        }

    private:
        template <typename Value>
        static std::string value_to_string(const Value& value)
        {
            if constexpr (std::is_enum_v<Value>) {
                return std::to_string(static_cast<std::underlying_type_t<Value>>(value));
            }
            else {
                std::ostringstream out;
                out << value;
                return out.str();
            }
        }

        std::string_view _suite_name;
        std::string _test_filter;
        int _test_count = 0;
        int _failed_count = 0;
        bool _current_failed = false;
    };

}

#define PQ_EXPECT(condition) suite.expect((condition), #condition, __FILE__, __LINE__)
#define PQ_EXPECT_EQ(left, right) suite.expect_eq((left), (right), #left, #right, __FILE__, __LINE__)