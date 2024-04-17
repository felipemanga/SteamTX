module;

import <algorithm>;
import <atomic>;
import <cstring>;
import <functional>;
import <memory>;
import <vector>;
import <iostream>;

import Camera;
import Event;
import EventBus;
import System;
import UI;

export module UICamera;

export namespace ui {

    class Camera : public Image {
	std::vector<uint8_t> data;
	mutable std::atomic_bool dirty{};
	std::unique_ptr<::Camera> camera;

	void connect() {
	    if (camera)
		return;

	    auto cameras = ::Camera::poll();
	    if (cameras.empty())
		return;

	    dirty = false;
	    image.reset();

	    std::ranges::rotate(cameras, cameras.begin() + (cameraIndex % cameras.size()));
	    for (auto& ref : cameras) {
		camera = ref->create([&](auto buffer, std::size_t len) {
		    dirty = false;
		    data.resize(len);
		    memcpy(data.data(), buffer, len);
		    dirty = true;
		});
		if (camera->good()) {
		    break;
		}
		std::cout << camera->getError() << std::endl;
		camera.reset();
	    }
	}

    public:
	std::size_t cameraIndex{};

	void nextCamera() {
	    cameraIndex++;
	    camera.reset();
	    connect();
	}

	const std::string& name() {
	    static std::string empty;
	    return camera ? camera->name() : empty;
	}

	std::size_t width() const override {
	    return camera ? camera->width() : 0;
	}

	std::size_t height() const override {
	    return camera ? camera->height() : 0;
	}

	void draw(System& sys, Coord offset) override {
	    if (!camera)
		connect();

	    if (dirty) {
		if (!image) {
		    image = sys.createImage(camera->width(), camera->height(), System::ImageFormat::YUY2);
		    EventBus::send<msg::RequestLayout>({});
		}
		image->update(data.data());
		dirty = false;
	    }

	    Image::draw(sys, offset);
	}
    };

}
