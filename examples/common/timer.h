#pragma once
#include <chrono>
#include <type_traits>

class Timer {
    std::chrono::time_point<std::chrono::high_resolution_clock> _preTimePoint{};
    mutable std::chrono::milliseconds _elapsedTime{};
    mutable std::chrono::milliseconds _dt{};  //

    bool _running = false;

public:
    using Hour = std::chrono::hours;
    using Min = std::chrono::minutes;
    using Sec = std::chrono::seconds;
    using Ms = std::chrono::milliseconds;
    using Us = std::chrono::microseconds;
    using Ns = std::chrono::nanoseconds;

    Timer() {}
    // constexpr void Reset()
    //{
    //	_elapsedTime = std::chrono::milliseconds::zero();
    // }
    constexpr bool IsRunning() const { return _running; }
    constexpr void Pause() {
        if (_running) {
            _running = false;
            _elapsedTime += std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::high_resolution_clock::now() - _preTimePoint);
            _preTimePoint = std::chrono::high_resolution_clock::now();
        }
    }
    constexpr void Start() {
        if (_running) return;
        _running = true;
        _preTimePoint = std::chrono::high_resolution_clock::now();
    }
    constexpr void Start(uint64_t startTimeMs) {
        if (_running) return;
        _running = true;
        _elapsedTime = std::chrono::milliseconds(startTimeMs);
        _preTimePoint = std::chrono::high_resolution_clock::now();
    }
    constexpr void Stop() {
        _running = false;
        _elapsedTime = std::chrono::milliseconds::zero();
    }
    constexpr void Restart() {
        Stop();
        Start();
    }
    constexpr void Restart(uint64_t startTimeMs) {
        Stop();
        Start(startTimeMs);
    }
    void SetTimeMs(uint64_t tMs) {
        _elapsedTime = std::chrono::milliseconds{tMs};
        _preTimePoint = std::chrono::high_resolution_clock::now();
    }
    uint64_t GetDt() {
        if (_running) {
            _dt = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() -
                                                                        _preTimePoint);
            _elapsedTime += _dt;
            _preTimePoint = std::chrono::high_resolution_clock::now();
        }
        return _dt.count();
    }

    constexpr uint64_t getElapsedTimeInMs() const {
        return _running ? (_elapsedTime + std::chrono::duration_cast<std::chrono::milliseconds>(
                                              std::chrono::high_resolution_clock::now() - _preTimePoint))
                              .count()
                        : _elapsedTime.count();
    }

    // uint64_t getElapsedTime(TimeUnit unit)
    //{
    //	auto end = std::chrono::high_resolution_clock::now();
    //	switch (unit)
    //	{
    //	case TimeUnit::NANOSECONS:
    //		return std::chrono::duration_cast<std::chrono::nanoseconds>(end -
    //_start).count(); 	case TimeUnit::MICROSECONDS: 		return
    // std::chrono::duration_cast<std::chrono::microseconds>(end - _start).count();
    //	case TimeUnit::MILLISECONDS:
    //		FALLTHROUGH
    //	default:
    //		return std::chrono::duration_cast<std::chrono::milliseconds>(end -
    //_start).count(); 	case TimeUnit::SECONDS: 		return
    // std::chrono::duration_cast<std::chrono::seconds>(end - _start).count(); 	case
    // TimeUnit::MINUTES: 		return
    // std::chrono::duration_cast<std::chrono::minutes>(end - _start).count(); 	case
    // TimeUnit::HOURS: 		return std::chrono::duration_cast<std::chrono::hours>(end -
    //_start).count();
    //	}
    // }
};