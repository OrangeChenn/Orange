#pragma once

namespace orange {

class Noncopyable {
public:
    Noncopyable() = default;
    ~Noncopyable() = default;

    Noncopyable(const Noncopyable& oth) = delete;
    Noncopyable& operator=(const Noncopyable& oth) = delete;
    Noncopyable(Noncopyable&& oth) = delete;
    Noncopyable& operator=(Noncopyable&& oth) = delete;
};

}
