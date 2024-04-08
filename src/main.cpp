import <algorithm>;
import <atomic>;
import <chrono>;
import <cstdint>;
import <cstring>;
import <fstream>;
import <functional>;
import <iostream>;
import <memory>;
import <queue>;
import <string>;
import <string_view>;
import <thread>;
import <typeindex>;
import <utility>;
import <random>;
import <filesystem>;

import EventBus;
import System;
import UI;
import Registry;
import SerialPort;
import Camera;
import Radio;
import utils;
import Event;
import Throttled;
import UICamera;

using namespace std::chrono_literals;

class MainWindow {
    EventBus bus{this};

    std::shared_ptr<ui::Scene> scene;
    std::shared_ptr<ui::Image> imgBG;
    std::shared_ptr<ui::Camera> camera;
    std::shared_ptr<ui::Label> lblInfo;

public:
    Radio radio;

    MainWindow(System& sys) {
	bus >> [&](msg::OnResize event){onResize(event.width, event.height);}
	    >> &MainWindow::onKeyUp
	    >> &MainWindow::onKeyDown
	    >> &MainWindow::onAxisUpdate
	    >> &MainWindow::onButtonDown
	    >> &MainWindow::onButtonUp;

	auto fontName = registry::lookup<std::string>("ui.font.path", "DejaVuSans.ttf");
	auto fontSize = registry::lookup("ui.font.size", 0) ?: 8;
	auto font = std::shared_ptr{sys.loadFont(fontName, fontSize)};
	scene = std::make_shared<ui::Scene>(sys);

	auto gui = ui::Widget::create();
	scene->addChild(gui);

	std::string wallpaperPath;
	auto wallpaperFolder = registry::lookup<std::string>("ui.wallpapers");
	if (!wallpaperFolder.empty())
	    wallpaperPath = rnd(ls(wallpaperFolder));
	wallpaperPath = registry::lookup("ui.wallpaper", wallpaperPath);

	imgBG = std::make_shared<ui::Image>(sys.loadImage(wallpaperPath));
	gui->addChild(imgBG);

	camera = std::make_shared<ui::Camera>();
	gui->addChild(camera);

	lblInfo = std::make_shared<ui::Label>(font, 0xFFFFFFFF);
	gui->addChild(lblInfo);

	imgBG->fill(sys.width(), sys.height());

	onResize(sys.width(), sys.height());
    }

    void setFPS(int frames) {
	*lblInfo = "FPS: " + std::to_string(frames);
    }

    void onResize(std::size_t width, std::size_t height) {
	camera->position = {
	    static_cast<int>(width/2.0f - camera->width()/2.0f),
	    static_cast<int>(height/2.0f - camera->height()/2.0f),
	};
	imgBG->fill(width, height);
	lblInfo->position = {
	    static_cast<int>(width * 0.5),
	    static_cast<int>(height * 0.5f)
	};
    }

    void onKeyUp(msg::OnKeyUp k) {
	std::cout << "Up scancode: " << k.scancode << std::endl;
	switch (k.sym) {
	case 'n':
	    camera->nextCamera();
	    break;
	default:
	    break;
	}
    }

    void onKeyDown(msg::OnKeyDown k) {
    }

    void onAxisUpdate(msg::OnAxisUpdate& axis) {
	// std::cout << "a:" << axis.id << " = " << axis.value << std::endl;
	switch (axis.id) {
	case 0: radio.turn = axis.value; break;
	case 2: radio.brk = axis.value * 0.5f + 0.5f; break;
	case 5: radio.gas = axis.value * 0.5f + 0.5f; break;
	default: break;
	}
    }

    void onButtonDown(msg::OnButtonDown event) {
	auto button = event.button;
	std::cout << "button: " << button << std::endl;
	if (button == 3) radio.setSteerLimit();
	if (button == 4) radio.steerTrim -= 10;
	if (button == 5) radio.steerTrim += 10;
    }

    void onButtonUp(msg::OnButtonUp event) {
    }
};

int main() {
    registry::load("config.ini");

    auto sys = System::create();
    if (!sys->loop())
	return 1;

    MainWindow main(*sys);

    Throttled updateRadio {1000ms/100, [&]{
	main.radio.update();
    }};

    int frames = 0;
    Throttled updateFPS {1s, [&]{
	main.setFPS(frames);
	frames = 0;
    }};

    Throttled updateUI {1000ms/30, [&]{
	sys->draw();
    }};

    while (sys->loop()) {
	frames++;
	updateRadio();
	updateFPS();
	updateUI();
	std::this_thread::sleep_for(10ms);
    }

    return 0;
}
