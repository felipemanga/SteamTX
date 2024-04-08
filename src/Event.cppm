module;

import <cstddef>;
import <cstdint>;

export module Event;

export namespace msg
{

    struct OnResize {
	std::size_t width;
	std::size_t height;
    };

    struct OnDraw {};

    struct OnKeyUp {
	std::uint32_t scancode;
	int sym;
    };

    struct OnKeyDown {
	std::uint32_t scancode;
	int sym;
    };

    struct OnAxisUpdate {
	int id;
	float value;
    };

    struct OnButtonDown {
	int button;
    };

    struct OnButtonUp {
	int button;
    };
}
