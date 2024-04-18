module;

import <filesystem>;
import <functional>;
import <iostream>;
import <memory>;
import <string>;
import <typeindex>;

import Camera;
import Event;
import EventBus;
import Radio;
import Registry;
import Throttled;
import UI;
import UICamera;
import utils;
import System;

export module Main;

export class MainWindow {
    EventBus bus{this};

    std::shared_ptr<ui::Scene> scene;
    std::shared_ptr<ui::Image> imgBG;
    std::shared_ptr<ui::Camera> camera;
    std::shared_ptr<ui::Label> lblInfo;

    int quitButton = registry::lookup("controls.quit", 6);
    int nextCameraButton = registry::lookup("controls.nextCamera", 0);
    int nextPortButton = registry::lookup("controls.nextPort", 1);
    int setSteerLimitButton = registry::lookup("controls.setSteerLimit", 3);
    int trimMinusButton = registry::lookup("controls.trimMinus", 4);
    int trimPlusButton = registry::lookup("controls.trimPlus", 5);
    int revButton = registry::lookup("controls.reverse", 6);
    int toggle3Button = registry::lookup("controls.toggle3", 7);

    int turnAxis = registry::lookup("controls.turnAxis", 0);
    int gasAxis = registry::lookup("controls.gasAxis", 5);
    int brkAxis = registry::lookup("controls.brkAxis", 2);

    bool showFPS = registry::lookup("ui.showFPS", false);
    bool showButton = registry::lookup("ui.showButton", false);
    bool showAxis = registry::lookup("ui.showAxis", false);
    bool verboseOutput = registry::lookup("verbose.output", false);
    bool verboseInput = registry::lookup("verbose.input", false);

public:
    Radio radio;

    MainWindow(System& sys) {
	bus >> [&](msg::OnResize event){bus << msg::RequestLayout();}
	    >> &MainWindow::onKeyUp
	    >> &MainWindow::onKeyDown
	    >> &MainWindow::onAxisUpdate
	    >> &MainWindow::onButtonDown
	    >> &MainWindow::onButtonUp;

	auto fontName = registry::lookup<std::string>("ui.font.path", "DejaVuSans.ttf");
	auto fontSize = registry::lookup("ui.font.size", 0) ?: 8;
	auto font = std::shared_ptr{sys.loadFont(fontName, fontSize)};
	scene = std::make_shared<ui::Scene>(sys);
	scene->layout = [&]{layout(sys.width(), sys.height());};

	auto gui = ui::Widget::create();
	scene->addChild(gui);

	std::string wallpaperPath;
	auto wallpaperFolder = registry::lookup<std::string>("ui.wallpapers");
	if (!wallpaperFolder.empty()) {
	    auto files = ls(wallpaperFolder);
	    if (!files.empty())
		wallpaperPath = rnd(files);
	}
	wallpaperPath = registry::lookup("ui.wallpaper", wallpaperPath);

	imgBG = std::make_shared<ui::Image>(sys.loadImage(wallpaperPath));
	gui->addChild(imgBG);

	camera = std::make_shared<ui::Camera>();
	gui->addChild(camera);

	lblInfo = std::make_shared<ui::Label>(font, 0xFFFFFFFF);
	gui->addChild(lblInfo);

	imgBG->fill(sys.width(), sys.height());

	std::cout << "UI Ready" << std::endl;
    }

    void setFPS(int frames) {
	if (showFPS)
	    *lblInfo = "FPS: " + std::to_string(frames);
    }

    void layout(std::size_t width, std::size_t height) {
	camera->fit(width, height);
	camera->center(width, height);
	imgBG->fill(width, height);
	imgBG->center(width, height);
	lblInfo->position = {0, 0};
    }

    constexpr static inline float neutral = 0.0f;
    constexpr static inline float blip = 0.17f;
    constexpr static inline float slow = 0.3f;
    constexpr static inline float fast = 0.3f;
    constexpr static inline int blipLength = 4;

    float brk = neutral;
    float gas = neutral;
    float rev = neutral;
    float lastGas = 0.0f;

    int skip = 0;

    bool print = false;
    bool didBreak = false;
    bool breakEnded = false;
    bool revMode = false;

    bool wantsToBreak() {return brk < neutral;}
    bool wantsToReverse() {return rev < neutral;}
    bool wantsToDrive() {return rev >= neutral && brk >= neutral && gas > neutral;}
    bool canBreak() {return !didBreak && !breakEnded || lastGas > neutral;}
    bool canReverse() {return breakEnded || didBreak;}
    bool isNeutral(float v) {return std::abs(v - neutral) < 0.01;}

    void beforeUpdateESC() {
	if (print && verboseOutput) {
	    print = false;
	    std::cout << "did break " << didBreak;
	    std::cout << " break ended " << breakEnded;
	    std::cout << " can break " << canBreak() << std::endl;
	}
	if (wantsToBreak()) {
	    if (!canBreak()) {
		radio.gas = neutral + blip;
		skip = blipLength;
	    } else {
		radio.gas = brk;
		breakEnded = false;
	    }
	} else if (wantsToReverse()) {
	    if (canBreak()) {
		radio.gas = neutral - blip;
		skip = blipLength;
		didBreak = true;
		breakEnded = false;
	    } else if (!canReverse()) {
		radio.gas = neutral;
	    } else {
		radio.gas = rev;
	    }
	} else {
	    radio.gas = gas;
	}
    }

    void afterUpdateESC() {
	if (lastGas < neutral && isNeutral(radio.gas)) {
	    if (verboseOutput) std::cout << "is neutral " << radio.gas << std::endl;
	    breakEnded = true;
	    didBreak = true;
	}
	if (radio.gas > neutral) {
	    didBreak = false;
	    breakEnded = false;
	}
	lastGas = radio.gas;
    }

    void updateRadio() {
	if (skip) {
	    skip--;
	    radio.update();
	    return;
	}

	beforeUpdateESC();
	if (lastGas != radio.gas && verboseOutput)
	    std::cout << "gas = " << radio.gas << std::endl;
	radio.update();
	afterUpdateESC();
    }

    void onKeyUp(msg::OnKeyUp k) {
	print = true;
	// std::cout << "Up scancode: " << k.scancode << std::endl;
	switch (k.sym) {
	case 'n':
	    camera->nextCamera();
	    break;
	case 'q':
	    bus << msg::RequestQuit{};
	    break;
	case 'p':
	    radio.nextPort();
	    break;
	case 'r':
	    rev = neutral;
	    break;
	case 'f':
	    gas = neutral;
	    break;
	case 'b':
	    brk = neutral;
	    break;
	default:
	    break;
	}
    }

    void onKeyDown(msg::OnKeyDown k) {
	print = true;
	switch (k.sym) {
	case 'n':
	    break;
	case 'r':
	    rev = neutral - slow;
	    break;
	case 'f':
	    gas = neutral + fast;
	    break;
	case 'b':
	    brk = neutral - slow;
	    break;
	default:
	    break;
	}
    }

    void onAxisUpdate(msg::OnAxisUpdate& axis) {
	if (showAxis) *lblInfo = "Axis:" + std::to_string(axis.id);
	if (axis.id == turnAxis) {
	    radio.turn = axis.value;
	    if (verboseInput) std::cout << "turn:" << axis.id << " = " << axis.value << std::endl;
	} else if (axis.id == gasAxis) {
	    gas = axis.value * 0.5f + 0.5f;
	    if (revMode)
		rev = -gas;
	    if (verboseInput) std::cout << "gas:" << axis.id << " = " << axis.value << " n:" << gas << std::endl;
	} else if (axis.id == brkAxis) {
	    brk = - (axis.value * 0.5f + 0.5f);
	    if (verboseInput) std::cout << "brakes:" << axis.id << " = " << axis.value << " n:" << brk << std::endl;
	} else {
	    if (verboseInput) std::cout << "axis:" << axis.id << " = " << axis.value << std::endl;
	}
    }

    void onButtonDown(msg::OnButtonDown event) {
	auto button = event.button;
	if (verboseInput) std::cout << "button: " << button << std::endl;
	if (showButton) *lblInfo = "Btn:" + std::to_string(button);
	if (button == setSteerLimitButton) radio.setSteerLimit();
	if (button == trimMinusButton) radio.steerTrim -= 10;
	if (button == trimPlusButton) radio.steerTrim += 10;
	if (button == nextCameraButton) camera->nextCamera();
	if (button == nextPortButton) radio.nextPort();
	if (button == revButton) revMode = true;
	if (button == quitButton) bus << msg::RequestQuit{};
	if (button == toggle3Button) radio.ch3 = !radio.ch3;
    }

    void onButtonUp(msg::OnButtonUp event) {
	if (event.button == revButton) {
	    revMode = false;
	    rev = neutral;
	}
    }
};
