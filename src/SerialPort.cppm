module;

import <memory>;
import <string>;
import <string_view>;
import <vector>;

import Injectable;

export module SerialPort;

export class SerialPort {
public:
    virtual ~SerialPort() = default;
    virtual void setParity(bool enable) = 0;
    virtual void setDoubleStopBit(bool enable) = 0;
    virtual void setBitsPerByte(int bpp) = 0;
    virtual void setHardwareFlowControl(bool enable) = 0;
    virtual void setBaudRate(int rate) = 0;

    virtual void apply() = 0;
    virtual const std::string& read() = 0;
    virtual void clear() = 0;
    virtual void write(std::string_view data) = 0;
    virtual bool good() = 0;
    virtual const std::string& getError() = 0;

    struct Reference {
	virtual ~Reference() = default;
	virtual std::unique_ptr<SerialPort> create() = 0;
	virtual std::string name() = 0;
    };

    class Poller : public Injectable<Poller> {
    public:
	virtual std::vector<std::unique_ptr<SerialPort::Reference>> poll() = 0;
    };

    static std::vector<std::unique_ptr<SerialPort::Reference>> poll() {
	return Poller::create()->poll();
    }
};
