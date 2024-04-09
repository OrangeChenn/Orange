#pragma once

#include<memory>

namespace orange {

template<class T, class X = void, int N = 0>
class Singleton {
public:
    static T* GetInstance() {
        static T v;
        return &v;
    }

    Singleton(Singleton& s) = delete;
    Singleton& operator=(Singleton&) = delete;
};

template<class T, class X = void, int N = 0>
class SingletonPtr {
public:
    static std::shared_ptr<T> GetInstance() {
        static std::shared_ptr<T> v(new T);
        return v;
    }

    SingletonPtr(SingletonPtr&) = delete;
    SingletonPtr& operator=(SingletonPtr&) = delete;
};

}
