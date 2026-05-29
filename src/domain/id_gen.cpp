//
// Created by Максим Долганов on 7.01.26.
//

#include "domain/id_gen.h"

#include <atomic>
#include <chrono>
#include <random>
#include <sstream>

std::string id_gen_make() {
    using namespace std::chrono;

    static std::atomic<std::uint64_t> counter{0};
    static thread_local std::mt19937_64 rng{std::random_device{}()};

    auto ts_ms = static_cast<std::uint64_t>(
        duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count()
    );
    auto c = counter.fetch_add(1, std::memory_order_relaxed);
    auto r = rng();

    std::ostringstream oss{};
    oss << ts_ms << "_" << c << "_" << std::hex << r;
    return oss.str();
}
