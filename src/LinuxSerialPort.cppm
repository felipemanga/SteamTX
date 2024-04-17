#ifndef USE_LINUX_SERIALPORT
export module LinuxSerialPort;
#else

module;

import <atomic>;
import <filesystem>;
import <memory>;
import <mutex>;
import <shared_mutex>;
import <string_view>;
import <thread>;
import <vector>;

import SerialPort;

#include <stdio.h>
#include <string.h>
#include <fcntl.h> // Contains file controls like O_RDWR
#include <errno.h> // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h> // write(), read(), close()

export module LinuxSerialPort;

export class LinuxSerialPort : public SerialPort {
public:
    int fd{};
    std::string error;
    std::string buffer;
    std::string acc;
    std::shared_mutex bufferMutex;
    std::atomic_bool running {false};
    std::thread reader;
    termios tty;

    LinuxSerialPort(const std::filesystem::path& path) {
	fd = open(path.c_str(), O_RDWR);

	if (fd < 0) {
	    error = "Error " + std::to_string(errno) + " from open: " + strerror(errno);
	    fd = 0;
	    return;
	}

// Read in existing settings, and handle any error
// NOTE: This is important! POSIX states that the struct passed to tcsetattr()
// must have been initialized with a call to tcgetattr() overwise behaviour
// is undefined
	if (tcgetattr(fd, &tty) != 0) {
	    error = "Error " + std::to_string(errno) + " from tcgetattr: " + strerror(errno);
	    return;
	}

	tty.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)
	tty.c_lflag &= ~ICANON; // non-canonical mode
	tty.c_lflag &= ~ECHO; // Disable echo
	tty.c_lflag &= ~ECHOE; // Disable erasure
	tty.c_lflag &= ~ECHONL; // Disable new-line echo
	tty.c_lflag &= ~ISIG; // Disable interpretation of INTR, QUIT and SUSP

	tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl
	tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); // Disable any special handling of received bytes

	tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
	tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed
	#ifdef OXTABS
	tty.c_oflag &= ~OXTABS; // Prevent conversion of tabs to spaces (NOT PRESENT IN LINUX)
	#endif
	#ifdef ONOEOT
	tty.c_oflag &= ~ONOEOT; // Prevent removal of C-d chars (0x004) in output (NOT PRESENT IN LINUX)
	#endif

	tty.c_cc[VTIME] = 5;    // Wait for up to 0.5s (50 deciseconds), returning as soon as any data is received.
	tty.c_cc[VMIN] = 0;

	setParity(false);
	setDoubleStopBit(false);
	setBitsPerByte(8);
	setHardwareFlowControl(false);
	setBaudRate(9600);
    }

    void setParity(bool enable) override {
	if (enable) tty.c_cflag |= PARENB;  // Set parity bit, enabling parity
	else tty.c_cflag &= ~PARENB; // Clear parity bit, disabling parity (most common)
    }

    void setDoubleStopBit(bool enable) override {
	if (enable) tty.c_cflag |= CSTOPB;  // Set stop field, two stop bits used in communication
	else tty.c_cflag &= ~CSTOPB; // Clear stop field, only one stop bit used in communication (most common)
    }

    void setBitsPerByte(int bpp) override {
	tty.c_cflag &= ~CSIZE; // Clear all the size bits, then use one of the statements below
	if (bpp == 5) tty.c_cflag |= CS5; // 5 bits per byte
	if (bpp == 6) tty.c_cflag |= CS6; // 6 bits per byte
	if (bpp == 7) tty.c_cflag |= CS7; // 7 bits per byte
	if (bpp == 8) tty.c_cflag |= CS8; // 8 bits per byte (most common)
    }

    void setHardwareFlowControl(bool enable) override {
	if (enable) tty.c_cflag |= CRTSCTS;  // Enable RTS/CTS hardware flow control
	else tty.c_cflag &= ~CRTSCTS; // Disable RTS/CTS hardware flow control (most common)
    }

    void setBaudRate(int rate) override {
	int r = B9600;
	switch (rate) {
	case 50: r = B50; break;
	case 75: r = B75; break;
	case 110: r = B110; break;
	case 134: r = B134; break;
	case 150: r = B150; break;
	case 200: r = B200; break;
	case 300: r = B300; break;
	case 600: r = B600; break;
	case 1200: r = B1200; break;
	case 1800: r = B1800; break;
	case 2400: r = B2400; break;
	case 4800: r = B4800; break;
	case 9600: r = B9600; break;
	case 19200: r = B19200; break;
	case 38400: r = B38400; break;
	default: break;
	}
	cfsetispeed(&tty, r);
	cfsetospeed(&tty, r);
    }

    void apply() override {
	if (good() && tcsetattr(fd, TCSANOW, &tty) != 0) {
	    error = "Error " + std::to_string(errno) + " from tcsetattr: " + strerror(errno);
	    return;
	}
	if (!running) {
	    running = true;
	    reader = std::thread{[&]{readLoop();}};
	}
    }

    void readLoop() {
	while (running) {
	    char buffer[256];
	    int readCount = ::read(fd, buffer, sizeof(buffer));
	    if (readCount < 0) {
		running = false;
		return;
	    } else if (readCount > 0) {
		std::unique_lock lock{bufferMutex};
		this->buffer += std::string_view{buffer, static_cast<std::size_t>(readCount)};
	    }
	}
    }

    bool good() override {
	return error.empty();
    }

    const std::string& getError() override {
	return error;
    }

    void write(std::string_view msg) override {
	::write(fd, msg.data(), msg.size());
    }

    void clear() override {
	acc.clear();
    }

    const std::string& read() override {
	if (!running && good()) {
	    error = "Read error";
	} else {
	    std::unique_lock lok{bufferMutex};
	    acc += buffer;
	    buffer.clear();
	}
	return acc;
    }

    ~LinuxSerialPort() {
	running = false;
	if (reader.joinable())
	    reader.join();
	if (fd)
	    close(fd);
    }

    class LinuxReference : public SerialPort::Reference {
    public:
	std::filesystem::path path;

	LinuxReference(const std::filesystem::path& path) : path{path} {}

	std::unique_ptr<SerialPort> create() override {
	    return std::make_unique<LinuxSerialPort>(path);
	}

	std::string name() override {
	    return path.string();
	}
    };

    class LinuxPoller : public SerialPort::Poller {
    public:
	std::vector<std::unique_ptr<SerialPort::Reference>> poll() override {
	    std::filesystem::path dev{"/dev"};
	    std::vector<std::unique_ptr<SerialPort::Reference>> refs;
	    for (auto& file : std::filesystem::directory_iterator{dev}) {
		if (!file.is_character_file()) continue;
		auto filename = file.path().filename().string();
		if (std::string_view{filename}.substr(0, 6) != "ttyACM") continue;
		refs.emplace_back(std::make_unique<LinuxReference>(file.path()));
	    }
	    return refs;
	}
    };
};

SerialPort::Poller::Register<LinuxSerialPort::LinuxPoller> _;

#endif
