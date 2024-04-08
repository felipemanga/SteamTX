module;

import <cstddef>;
import <functional>;
import <memory>;
import <string>;
import <vector>;

import Injectable;

export module Camera;

export class Camera {
public:
    virtual ~Camera() = default;
    virtual bool good() = 0;
    virtual const std::string& getError() = 0;
    virtual std::size_t width() = 0;
    virtual std::size_t height() = 0;
    virtual const std::string& name() = 0;

    using OnFrameCallback = std::function<void(const void* buffer, std::size_t length)>;
    virtual void setOnFrame(const OnFrameCallback& onFrame) = 0;

    struct Reference {
	virtual ~Reference() = default;
	virtual std::unique_ptr<Camera> create(const OnFrameCallback&) = 0;
    };

    class Poller : public Injectable<Poller> {
    public:
	virtual std::vector<std::unique_ptr<Camera::Reference>> poll() = 0;
    };

    static std::vector<std::unique_ptr<Camera::Reference>> poll() {
	return Poller::create()->poll();
    }
};
