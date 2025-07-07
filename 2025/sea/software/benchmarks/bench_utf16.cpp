#include "benchmarker.h"
#include <vector>
#include <random>
#include <fmt/core.h>
#include <cstdint>
#include <span>

bool is_valid_utf16(const std::span<uint16_t> code_units) {
    size_t i = 0;
    while (i < code_units.size()) {
        uint16_t unit = code_units[i];
        if (unit <= 0xD7FF || unit >= 0xE000 ) {
            ++i; continue;
        }
        if (unit >= 0xD800 && unit <= 0xDBFF) {
            if (i + 1 >= code_units.size()) {
                return false; // Missing low surrogate
            }
            uint16_t next_unit = code_units[i + 1];
            if (next_unit < 0xDC00 || next_unit > 0xDFFF) {
                return false; // Invalid low surrogate
            }
            i += 2; // Valid surrogate pair
            continue;
        }
        return false;
    }
    return true;
}

void pretty_print(const std::string& name, size_t num_values,
                  event_aggregate agg) {
  fmt::print("{:<50} : ", name);
  fmt::print(" {:5.2f} ns ", agg.fastest_elapsed_ns()/num_values);
  if (collector.has_events()) {
    fmt::print(" {:5.2f} GHz ", agg.fastest_cycles() / agg.fastest_elapsed_ns());
    fmt::print(" {:5.2f} c ", agg.fastest_cycles()/ num_values);
    fmt::print(" {:5.2f} i ", agg.fastest_instructions()/ num_values);
    fmt::print(" {:5.2f} i/c ", agg.fastest_instructions() / agg.fastest_cycles());
  }
  fmt::print("\n");
}

int main() {
    constexpr size_t num_values = 1'000'000;

    std::vector<uint16_t> valid_utf16(num_values, 0x0041); // 'A' repeated

    // Mixed random (déjà présent)
    std::vector<uint16_t> mixed_utf16;
    mixed_utf16.reserve(num_values);
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(0, 1);
    size_t i = 0;
    while (i < num_values) {
        int kind = dist(rng);
        if (kind == 0 && i + 1 < num_values) {
            // Add a valid surrogate pair
            mixed_utf16.push_back(0xD800 + (rng() % 0x400)); // high surrogate
            mixed_utf16.push_back(0xDC00 + (rng() % 0x400)); // low surrogate
            i += 2;
        } else if (kind == 1) {
            // Add a lone BMP value
            mixed_utf16.push_back(0x0041 + (rng() % 0x1000));
            i++;
        }
    }

    // Mixed alterné (alternance stricte)
    std::vector<uint16_t> mixed_utf16_alternate;
    mixed_utf16_alternate.reserve(num_values);
    size_t j = 0;
    bool add_pair = true;
    while (j < num_values) {
        if (add_pair && j + 1 < num_values) {
            mixed_utf16_alternate.push_back(0xD800 + ((j/2) % 0x400)); // high surrogate
            mixed_utf16_alternate.push_back(0xDC00 + ((j/2) % 0x400)); // low surrogate
            j += 2;
        } else {
            mixed_utf16_alternate.push_back(0x0041 + (j % 0x1000));
            j++;
        }
        add_pair = !add_pair;
    }
    

    for (int i = 0; i < 4; ++i) {
        fmt::print("Run {}\n", i + 1);

        auto valid_agg = bench([&valid_utf16]() {
            volatile bool ok = is_valid_utf16(std::span<uint16_t>(valid_utf16));
            (void)ok;
        });
        pretty_print("is_valid_utf16 (valid)", num_values, valid_agg);

        auto mixed_agg = bench([&mixed_utf16]() {
            volatile bool ok = is_valid_utf16(std::span<uint16_t>(mixed_utf16));
            (void)ok;
        });
        pretty_print("is_valid_utf16 (mixed, random)", num_values, mixed_agg);

        auto mixed_alt_agg = bench([&mixed_utf16_alternate]() {
            volatile bool ok = is_valid_utf16(std::span<uint16_t>(mixed_utf16_alternate));
            (void)ok;
        });
        pretty_print("is_valid_utf16 (mixed, alternate)", num_values, mixed_alt_agg);
    }
    return 0;
}
