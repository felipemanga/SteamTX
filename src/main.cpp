import <algorithm>;
import <chrono>;
import <functional>;
import <iostream>;
import <thread>;

import EventBus;
import System;
import Registry;
import Throttled;
import Main;

using namespace std::chrono_literals;

int main() {
    std::cout << "Loading config" << std::endl;
    registry::load("config.ini");

    EventBus::verboseEvents = registry::lookup("verbose.events", false);

    std::cout << "Initializing system" << std::endl;
    auto sys = System::create();
    if (!sys->loop())
	return 1;

    MainWindow window(*sys);

    Throttled updateRadio {1000ms/100, [&]{
	window.updateRadio();
    }};

    int frames = 0;
    Throttled updateFPS {1s, [&]{
	window.setFPS(frames);
	frames = 0;
    }};

    Throttled updateUI {1000ms/30, [&]{
	sys->draw();
    }};

    std::cout << "Running" << std::endl;

    while (sys->loop()) {
	frames++;
	updateRadio();
	updateFPS();
	updateUI();
	std::this_thread::sleep_for(10ms);
    }

    return 0;
}
