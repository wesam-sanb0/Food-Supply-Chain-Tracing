// timer.h - Role 3: phase timer for the sequential benchmark
// steady_clock is monotonic: it never jumps backwards or forwards when the
// system clock is changed, so it is the right clock for measuring durations.
#ifndef FOOD_TRACE_TIMER_H
#define FOOD_TRACE_TIMER_H

#include <chrono>

class Timer {
public:
    using Clock = std::chrono::steady_clock;

    // Start (or restart) the measurement.
    void start() { begin_ = Clock::now(); }

    // Milliseconds since the last start().
    double elapsedMs() const {
        return std::chrono::duration<double, std::milli>(Clock::now() - begin_).count();
    }

private:
    Clock::time_point begin_ = Clock::now();
};

#endif
