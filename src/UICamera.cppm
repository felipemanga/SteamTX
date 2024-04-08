module;

import <algorithm>;
import <atomic>;
import <cstring>;
import <functional>;
import <memory>;
import <vector>;
import <iostream>;

import UI;
import System;
import Camera;

export module UICamera;

export namespace ui {

    class Camera : public Widget {
	std::vector<uint8_t> data;
	mutable std::atomic_bool dirty{};
	mutable std::unique_ptr<System::Image> image;
	std::unique_ptr<::Camera> camera;

	void connect() {
	    if (camera)
		return;

	    auto cameras = ::Camera::poll();
	    if (cameras.empty())
		return;

	    std::ranges::rotate(cameras, cameras.begin() + (cameraIndex % cameras.size()));
	    for (auto& ref : cameras) {
		camera = ref->create([&](auto buffer, std::size_t len) {
		    dirty = false;
		    data.resize(len);
		    memcpy(data.data(), buffer, len);
		    dirty = true;
		});
		if (camera->good())
		    break;
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

	std::size_t width() {
	    return camera ? camera->width() : 0;
	}

	std::size_t height() {
	    return camera ? camera->height() : 0;
	}

	void updateBounds(const Coord& position) override {
	    Widget::updateBounds(position);
	    if (image) {
		box.w = image->width();
		box.h = image->height();
	    }
	}

	void draw(System& sys, Coord offset) override {
	    if (!camera)
		connect();

	    if (dirty) {
		if (!image) {
		    image = sys.createImage(camera->width(), camera->height(), System::ImageFormat::YUY2);
		}
		image->update(data.data());
		dirty = false;
	    }

	    if (image) {
		offset += position;
	        image->blit(offset.x, offset.y);
	    }
	}
    };

}
