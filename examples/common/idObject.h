#pragma once
#include <cstdint>
template <typename T>
class IdObject {
    uint32_t _id{};

    uint32_t generateId() {
        static uint32_t i = 0;
        return i++;
    }

public:
    IdObject() { _id = generateId(); }
    // move
    IdObject(IdObject &&other) { _id = other._id; }
    // copy
    IdObject(IdObject const &other) { _id = generateId(); }

    // move
    IdObject &operator=(IdObject &&other) {
        _id = other._id;
        return *this;
    }
    // copy
    IdObject &operator=(IdObject const &other) {
        _id = generateId();
        return *this;
    }

    constexpr uint32_t getId() const { return _id; }
};