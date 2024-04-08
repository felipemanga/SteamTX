module;

import <algorithm>;
import <chrono>;
import <iostream>;
import <memory>;

import SerialPort;

export module Radio;

export class Radio {
    std::unique_ptr<SerialPort> serial;

    void connect() {
	serial.reset();
	for (auto& ref : SerialPort::poll()) {
	    serial = ref->create();
	    if (serial->good()) {
		serial->apply();
		if (serial->good()) {
		    std::string out = "func 0 1";
		    serial->write(out + "\n");
		    std::cout << "Connected. enabling func 0. [" << out << "]" << std::endl;
		    break;
		}
	    }
	    std::cerr << serial->getError() << std::endl;
	    serial.reset();
	}
    }

public:
    int steerTrim = 0;
    int steerMax = 512;
    int steerMin = -512;

    float turn{};
    float brk{};
    float gas{};

    void setSteerLimit() {
	if (turn * 512 < -50)
	    steerMin = turn * 512;
	if (turn * 512 > 50)
	    steerMax = turn * 512;
    }

    void update() {
	if (!serial || !serial->good()) {
	    if (serial)
		std::cout << "serial error:" << serial->getError();
	    connect();
	}

	if (!serial || !serial->good())
	    return;

	auto& read = serial->read();
	auto len = read.size();
	if (len < 2 || std::string_view{read}.substr(len - 2) != "> ")
	    return; // ignore inputs, serial not ready yet
	// std::cout << "in[" << read << "]>>" << std::endl;
	serial->clear();

	int scaledTurn = static_cast<int>((turn * 0.5f + 0.5f) * (steerMax - steerMin) + steerMin) + steerTrim;
	int clampedTurn = std::clamp(scaledTurn, -512, 512);
	int throttle = std::clamp<int>((gas - brk) * 512, -512, 512);
	serial->write(
	    "frame " +
	    std::to_string(clampedTurn) +
	    " " +
	    std::to_string(throttle) +
	    "\r\n"
	    );
    }
};
