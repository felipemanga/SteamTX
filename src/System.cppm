module;

import <cstddef>;
import <cstdint>;
import <functional>;
import <memory>;
import <string>;
import <string_view>;

import Injectable;
import EventBus;
import Event;

export module System;

export class System : public Injectable<System> {
public:
    virtual ~System() = default;
    virtual bool loop() = 0;
    virtual std::size_t width() = 0;
    virtual std::size_t height() = 0;

    enum class ImageFormat {
	YUY2
    };

    class Color {
    public:
	uint8_t r = 0;
	uint8_t g = 0;
	uint8_t b = 0;
	uint8_t a = 255;
	constexpr Color() = default;
	constexpr Color(uint32_t c) {
	    r = c >> 24;
	    g = c >> 16;
	    b = c >> 8;
	    a = c;
	}
    };

    class Image {
    public:
	virtual ~Image() = default;
	virtual void update(const void* data) = 0;
	virtual void blit(int x, int y) = 0;
	virtual void blit(int x, int y, int w, int h) = 0;
	virtual std::size_t width() = 0;
	virtual std::size_t height() = 0;
    };

    class Font {
    public:
	virtual ~Font() = default;
    };

    virtual std::unique_ptr<Font> loadFont(const std::string& fontPath, int size) = 0;
    virtual std::unique_ptr<Image> loadImage(const std::string& imagePath) = 0;
    virtual std::unique_ptr<Image> createImage(std::size_t width, std::size_t height, ImageFormat format) = 0;
    virtual std::unique_ptr<Image> createImage(const char* str, Font*, const Color& color) = 0;
    virtual void draw() = 0;

    struct Key {
	uint32_t scancode;
	int sym;
    };

    struct Axis {
	int id{};
	float value{};
    };

    using OnKeyEvent = std::function<void(const Key&)>;
    OnKeyEvent onKeyDown = [](const Key& key){
	EventBus::send<msg::OnKeyDown>({
		.scancode = key.scancode,
		.sym = key.sym
	    });
    };

    OnKeyEvent onKeyUp = [](const Key& key){
	EventBus::send<msg::OnKeyUp>({
		.scancode = key.scancode,
		.sym = key.sym
	    });
    };

    using OnButtonEvent = std::function<void(int button)>;
    OnButtonEvent onButtonDown = [](int button) {
	EventBus::send<msg::OnButtonDown>({button});
    };
    OnButtonEvent onButtonUp = [](int button) {
	EventBus::send<msg::OnButtonUp>({button});
    };

    using OnAxisUpdate = std::function<void(const Axis&)>;
    OnAxisUpdate onAxisUpdate = [](const Axis& axis){
	EventBus::send<msg::OnAxisUpdate>({
		axis.id,
		axis.value
	    });
    };

    using OnResize = std::function<void(std::size_t width, std::size_t height)>;
    OnResize onResize = [](std::size_t width, std::size_t height){
	EventBus::send<msg::OnResize>({width, height});
    };

    using OnDraw = std::function<void()>;
    OnDraw onDraw = []{
	EventBus::send<msg::OnDraw>({});
    };
};
