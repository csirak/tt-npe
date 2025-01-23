// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: © 2025 Tenstorrent AI ULC

#pragma once

#include <fmt/core.h>
#include <stdio.h>
#include <unistd.h>

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <tuple>
#include <unordered_map>

namespace tt_npe {

inline bool is_tty_interactive() { return isatty(fileno(stdin)); }
inline bool enable_color() { return is_tty_interactive(); }

namespace TTYColorCodes {
inline const char *red = "\u001b[31m";
inline const char *green = "\u001b[32m";
inline const char *yellow = "\u001b[33m";
inline const char *gray = "\u001b[37m";
inline const char *reset = "\u001b[0m";
inline const char *clear_screen = "\033[2J\033[H";
inline const char *move_cursor_topleft = "\033[H";
inline const char *show_cursor = "\033[?25h";
inline const char *hide_cursor = "\033[?251";
inline const char *dark_bg = "\e[48;2;30;30;30m";
inline const char *bold = "\033[1m";
}  // namespace TTYColorCodes

template <typename... Args>
static void log_error(fmt::format_string<Args...> fmt, Args &&...args) {
    fmt::println(
        stderr,
        "{}{}E: {}{}",
        TTYColorCodes::bold,
        TTYColorCodes::red,
        fmt::format(fmt, std::forward<Args>(args)...),
        TTYColorCodes::reset);
}
template <typename... Args>
static void log_warn(fmt::format_string<Args...> fmt, Args &&...args) {
    fmt::println(
        stderr,
        "{}{}W: {}{}",
        TTYColorCodes::bold,
        TTYColorCodes::yellow,
        fmt::format(fmt, std::forward<Args>(args)...),
        TTYColorCodes::reset);
}
template <typename... Args>
static void log(fmt::format_string<Args...> fmt, Args &&...args) {
    fmt::println(fmt, std::forward<Args>(args)...);
}

inline bool promptUser(const std::string &prompt_msg) {
    char response;
    fmt::print(
        stderr,
        "{}{}{} (y/n) : {}",
        TTYColorCodes::bold,
        TTYColorCodes::yellow,
        prompt_msg,
        TTYColorCodes::reset);
    std::cin >> response;
    fmt::println("");
    return tolower(response) == 'y' || tolower(response) == 'Y';
}

inline void printDiv(const std::string &title = "") {
    constexpr size_t total_width = 80;
    std::string padded_title = (title != "") ? std::string(" ") + title + " " : "";
    size_t bar_len = total_width - padded_title.size() - 4;
    std::string bar(bar_len, '-');
    fmt::println("\n--{}{}", padded_title, bar);
}

inline int64_t wrapToRange(int64_t number, int64_t range) {
    return ((number % range) + range) % range;
}

template <typename K, typename V, typename X>
inline const V &getWithDefault(
    const std::unordered_map<K, V> &container, const X &key, const V &default_val) {
    auto it = container.find(key);
    if (it == container.end()) {
        return default_val;
    } else {
        return it->second;
    }
}

// python-like enumerate wrapper for range based loops
// shamelessly stolen from : www.reedbeta.com/blog/python-like-enumerate-in-cpp17/
template <
    typename T,
    typename TIter = decltype(std::begin(std::declval<T>())),
    typename = decltype(std::end(std::declval<T>()))>
constexpr auto enumerate(T &&iterable) {
    struct iterator {
        size_t i;
        TIter iter;
        bool operator!=(const iterator &other) const { return iter != other.iter; }
        void operator++() {
            ++i;
            ++iter;
        }
        auto operator*() const { return std::tie(i, *iter); }
    };
    struct iterable_wrapper {
        T iterable;
        auto begin() { return iterator{0, std::begin(iterable)}; }
        auto end() { return iterator{0, std::end(iterable)}; }
    };
    return iterable_wrapper{std::forward<T>(iterable)};
}

struct Coord {
    Coord() : row(-1), col(-1) {}
    Coord(int row, int col) : row(row), col(col) {}
    int16_t row, col;
    bool operator==(const auto &rhs) const {
        return std::make_pair(row, col) == std::make_pair(rhs.row, rhs.col);
    }
};

// A pair of coords describing the 2D multicast target
struct MCastCoordPair {
    Coord start_coord, end_coord;
    bool operator==(const auto &rhs) const {
        return std::make_pair(start_coord, end_coord) ==
               std::make_pair(rhs.start_coord, rhs.end_coord);
    }
    // create begin and end iterators over the bounding box formed by start_coord and end_coord
    struct iterator {
        Coord current;
        const MCastCoordPair *mcast;
        iterator &operator++() {
            if (current.col < mcast->end_coord.col) {
                current.col++;
            } else {
                current.col = mcast->start_coord.col;
                current.row++;
            }
            return *this;
        }
        bool operator!=(const iterator &rhs) const { return current != rhs.current; }
        Coord operator*() const { return current; }
    };
    iterator begin() const { return iterator{start_coord, this}; }
    // must return one past the end
    iterator end() const { return iterator{Coord(end_coord.row, end_coord.col + 1), this}; }
};
// Variant holding either a Coord (unicast) or MCastCoordPair (multicast)
using NocDestination = std::variant<Coord,MCastCoordPair>;

// taken from https://medium.com/@nerudaj/std-visit-is-awesome-heres-why-f183f6437932
// Allows writing nicer handling for std::variant types, like so:
// >  std::visit(overloaded{
// >    [&] (const TypeA&) { ... },
// >    [&] (const TypeB&) { ... }
// >  }, variant);
template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};

// Some compilers might require this explicit deduction guide
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

}  // namespace tt_npe

// specialize std::hash for Coord
namespace std {
template <>
struct hash<tt_npe::Coord> {
    size_t operator()(const tt_npe::Coord &c) const {
        size_t seed = 0xBAADF00DBAADF00D;
        seed ^= c.row + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= c.col + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }
};
}  // namespace std

template <>
class fmt::formatter<tt_npe::Coord> {
   public:
    constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }
    template <typename Context>
    constexpr auto format(tt_npe::Coord const &coord, Context &ctx) const {
        return format_to(ctx.out(), "({},{})", coord.row, coord.col);
    }
};
