//
// Created by Максим Долганов on 3.01.26.
//

#pragma once
#include <optional>
#include <string>
#include <utility>

struct Status {
    bool ok = true;
    std::string message;
    static Status OK() {
        return {true, ""};
    }
    static Status Error(std::string error) {
        return {false, std::move(error)};
    }
};

template <class T>
struct StatusOrT {
    Status st;
    std::optional<T> value;
    static StatusOrT OK(T v) {
        return {Status::OK(), std::move(v)};
    }
    static StatusOrT Error(std::string error) {
        return {Status::Error(std::move(error)), std::nullopt};
    }
    [[nodiscard]] bool ok() const {
        return st.ok;
    }
};