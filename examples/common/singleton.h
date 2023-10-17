#pragma once

template <typename T>
class Singleton {
public:
    static T &getSingleton() {
        static T instance;
        return instance;
    }
};