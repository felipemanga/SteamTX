module;

import <algorithm>;
import <chrono>;
import <functional>;

export module Throttled;

export class Throttled {
    using Clock = std::chrono::high_resolution_clock;
    using Timepoint = Clock::time_point;
    using Duration = decltype(Timepoint{} - Timepoint{});

    Duration duration;
    std::function<void()> cb;
    Timepoint prev{};

public:
    template<typename CB>
    Throttled(Duration duration, CB&& cb) : duration{duration}, cb{std::move(cb)} {}

    template <typename ... Args>
    void operator () (Args&& ... args) {
	auto now = Clock::now();
	if (now - prev < duration)
	    return;
	prev = now;
	cb(std::forward<Args>(args)...);
    }
};
