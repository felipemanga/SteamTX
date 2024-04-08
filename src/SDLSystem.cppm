#ifndef USE_SDL_SYSTEM
export module SDLSystem;
#else

module;

import <cstdint>;
import <cstdio>;
import <iostream>;
import <string>;
import <unordered_map>;
import <memory>;
import <vector>;
import <functional>;

import EventBus;
import Event;
import System;

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>

export module SDLSystem;

export class SDLSystem : public System {
public:
    EventBus bus;
    std::unordered_map<std::string, TTF_Font*> fonts;
    SDL_Window* window{};
    SDL_Renderer* renderer{};
    SDL_Joystick* joystick{};
    int joyIndex{};
    std::size_t _width{};
    std::size_t _height{};

    SDLSystem() {
	SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
	SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO | SDL_INIT_JOYSTICK);
	initGraphics();
	TTF_Init();
	IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
    }

    ~SDLSystem() {
	IMG_Quit();
	TTF_Quit();
	if (joystick)
	    SDL_JoystickClose(joystick);
	if (window)
	    SDL_DestroyWindow(window);
	SDL_Quit();
    }

    void initGraphics() {
	SDL_DisplayMode DM;
	SDL_GetCurrentDisplayMode(0, &DM);
	auto width = 640; // DM.w;
	auto height = 480; // DM.h;
	window = SDL_CreateWindow("Title", 0, 0, DM.w, DM.h, SDL_SWSURFACE);// | SDL_WINDOW_FULLSCREEN_DESKTOP);
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	updateSize();
    }

    std::size_t width() override {return _width;}

    std::size_t height() override {return _height;}

    void updateSize() {
	int w, h;
	SDL_GetWindowSizeInPixels(window, &w, &h);
	_width = w;
	_height = h;
    }

    class SDLImage : public Image {
    public:
	SDL_Texture* texture{};
	std::size_t pitch{};
	int w{}, h{};
	SDLSystem& sys;

	SDLImage(SDLSystem& sys, std::size_t width, std::size_t height, ImageFormat format) : sys{sys} {
	    auto sdlFormat = SDL_PIXELFORMAT_YUY2;
	    w = width;
	    h = height;
	    pitch = width * SDL_BYTESPERPIXEL(sdlFormat);
	    texture = SDL_CreateTexture(sys.renderer, sdlFormat, SDL_TEXTUREACCESS_STREAMING, width, height);
	}

	SDLImage(SDLSystem& sys, SDL_Surface* surface) : sys{sys} {
	    w = surface->w;
	    h = surface->h;
	    pitch = w * surface->format->BytesPerPixel;
	    texture = SDL_CreateTextureFromSurface(sys.renderer, surface);
	}

        std::size_t width() override {
	    return w;
	}

        std::size_t height() override {
	    return h;
	}

	void update(const void* data) override {
	    SDL_UpdateTexture(texture, nullptr, data, pitch);
	}

	void blit(int x, int y) override {
	    SDL_Rect dest {.x = x, .y = y, .w = w, .h = h};
	    SDL_RenderCopy(sys.renderer, texture, nullptr, &dest);
	}

	void blit(int x, int y, int w, int h) override {
	    SDL_Rect dest {.x = x, .y = y, .w = w, .h = h};
	    SDL_RenderCopy(sys.renderer, texture, nullptr, &dest);
	}

	~SDLImage() {
	    if (texture)
		SDL_DestroyTexture(texture);
	}
    };

    class SDLFont : public Font {
    public:
	TTF_Font* font{};
	virtual ~SDLFont() {
	    if (font)
		TTF_CloseFont(font);
	}
    };

    std::unique_ptr<Font> loadFont(const std::string& fontPath, int size) override {
	auto f = std::make_unique<SDLFont>();
	f->font = TTF_OpenFont(fontPath.c_str(), size);
	if (!f->font)
	    f.reset();
	return f;
    }

    std::unique_ptr<Image> loadImage(const std::string& imagePath) override {
	auto surface = IMG_Load(imagePath.c_str());
	if (!surface) {
	    std::cout << "Could not open " << imagePath << std::endl;
	    return {};
	}
	auto image = std::make_unique<SDLImage>(*this, surface);
	SDL_FreeSurface(surface);
	return image;
    }

    std::unique_ptr<Image> createImage(std::size_t width, std::size_t height, ImageFormat format) override {
	return std::make_unique<SDLImage>(*this, width, height, format);
    }

    std::unique_ptr<Image> createImage(const char* str, Font* font, const Color& color) override {
	if (!font)
	    return {};
        auto surface = TTF_RenderUTF8_Blended(
	    static_cast<SDLFont*>(font)->font,
	    str,
	    {
		.r = color.r,
		.g = color.g,
		.b = color.b,
		.a = color.a
	    });
	auto image = std::make_unique<SDLImage>(*this, surface);
	SDL_FreeSurface(surface);
	return image;
    }

    void draw() override {
	onDraw();
	SDL_RenderPresent(renderer);
	SDL_RenderClear(renderer);
    }

    void updateJoysticks() {
	if (joystick) {
	    SDL_JoystickClose(joystick);
	    joystick = nullptr;
	}
	int numSticks = SDL_NumJoysticks();
	for (joyIndex = 0; !joystick && joyIndex < numSticks; ++joyIndex)
	    joystick = SDL_JoystickOpen(joyIndex);
	if (joystick)
	    std::cout << "Joystick Name: " << SDL_JoystickName(joystick) << std::endl;
    }

    bool loop() override {
	if (!renderer)
	    return false;
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
	    switch (event.type) {
	    case SDL_QUIT:
		return false;
	    case SDL_WINDOWEVENT:
		switch (event.window.event) {
		case SDL_WINDOWEVENT_RESIZED:
		    updateSize();
		    onResize(_width, _height);
		    continue;
		default:
		    continue;
		}
		continue;
	    case SDL_KEYUP:
		onKeyUp({
			.scancode = event.key.keysym.scancode,
			.sym = event.key.keysym.sym
		    });
		continue;
	    case SDL_KEYDOWN:
		onKeyDown({
			.scancode = event.key.keysym.scancode,
			.sym = event.key.keysym.sym
		    });
		continue;
	    case SDL_JOYDEVICEADDED:
	    case SDL_JOYDEVICEREMOVED:
		updateJoysticks();
		continue;
	    case SDL_JOYBUTTONUP:
		onButtonUp(event.jbutton.button);
		continue;
	    case SDL_JOYBUTTONDOWN:
		onButtonDown(event.jbutton.button);
		continue;
	    case SDL_JOYHATMOTION:
		std::cout << "hat " << int(event.jhat.hat) << " v:" << int(event.jhat.value) << std::endl;
		continue;
	    case SDL_JOYAXISMOTION:
		onAxisUpdate({
			.id = int(event.jaxis.axis),
			.value = static_cast<float>(event.jaxis.value) / (INT16_MAX + (event.jaxis.value < 0))
		    });
		continue;
	    case SDL_JOYBALLMOTION:
		std::cout << "ball " << int(event.jball.ball) << " x:" << int(event.jball.xrel) << std::endl;
		continue;
	    }
	}
	return true;
    }
};

System::Register<SDLSystem> _;

#endif
