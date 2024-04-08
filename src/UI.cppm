module;

import <algorithm>;
import <cstddef>;
import <iostream>;
import <memory>;
import <vector>;

import Event;
import EventBus;
import System;

export module UI;

export namespace ui {

    struct Coord {
	int x{}, y{};
	Coord& operator += (Coord o) {
	    x += o.x;
	    y += o.y;
	    return *this;
	}
    };

    struct Size {
	std::size_t w{}, h{};
    };

    struct Box : public Coord, public Size {
	int l() const {return x;}
	int r() const {return x + w;}
	int t() const {return y;}
	int b() const {return y + h;}

	void expand(const Coord& c) {
	    if (c.x < x) {
		w += x - c.x;
		x = c.x;
	    }
	    if (c.x > r()) {
		w += r() - c.x;
	    }
	    if (c.y < y) {
		h += y - c.y;
		y = c.y;
	    }
	    if (c.y > b()) {
		h += b() - c.y;
	    }
	}

	void expand(const Box& b) {
	    if (b.w || b.h) {
		expand(Coord{b.x, b.y});
		expand(Coord{b.r(), b.b()});
	    }
	}
    };

    class Widget : public std::enable_shared_from_this<Widget> {
	std::vector<std::shared_ptr<Widget>> children;
	std::weak_ptr<Widget> parent;
    public:
	std::shared_ptr<Widget> getParent() {
	    return parent.lock();
	}

	void removeChild(std::shared_ptr<Widget> child) {
	    if (!child || child->getParent().get() != this)
		return;
	    auto it = std::find(children.begin(), children.end(), child);
	    if (it == children.end())
		return;
	    children.erase(it);
	}

	void addChild(std::shared_ptr<Widget> child) {
	    auto oldParent = child->getParent();
	    if (oldParent.get() == this)
		return;
	    if (oldParent)
		oldParent->removeChild(child);
	    children.push_back(child);
	    child->parent = shared_from_this();
	}

	Coord position;
	Size size;
	Box box;

	virtual void updateBounds(const Coord& position) {
	    box.w = 0;
	    box.h = 0;
	    box.x = position.x;
	    box.y = position.y;
	}

	virtual ~Widget() {
	    for (auto& child : children)
		child->parent.reset();
	}

	virtual void draw(System& sys, Coord offset = {}) {
	    offset += position;
	    updateBounds(offset);
	    for (auto& child : children) {
		child->draw(sys, offset);
		box.expand(child->box);
	    }
	}

	static std::shared_ptr<Widget> create() {
	    return std::make_shared<Widget>();
	}
    };

    class Scene : public Widget {
	System& sys;
	EventBus bus;
    public:

	Scene(System& sys) : sys{sys} {
	    bus >> [](const msg::OnResize& onResize) {
	    };

	    bus >> [&](const msg::OnDraw& onDraw) {draw(sys);};
	}
    };

    class Image : public Widget {
    public:
	std::shared_ptr<System::Image> image;

	Image(std::shared_ptr<System::Image> image = {}) : image{image} {}

	float scale = 1.0f;

	void fit(std::size_t width, std::size_t height) {
	    if (!image)
		return;
	    scale = std::min<float>(static_cast<float>(width) / image->width(),
				    static_cast<float>(height) / image->height());
	}

	void fill(std::size_t width, std::size_t height) {
	    if (!image)
		return;
	    scale = std::max<float>(static_cast<float>(width) / image->width(),
				    static_cast<float>(height) / image->height());
	}

	void updateBounds(const Coord& position) override {
	    Widget::updateBounds(position);
	    if (image) {
		box.w = image->width() * scale;
		box.h = image->height() * scale;
	    }
	}

	void draw(System& sys, Coord offset) override {
	    if (image) {
		offset += position;
		image->blit(offset.x, offset.y, image->width() * scale, image->height() * scale);
	    }
	}
    };

    class Label : public Widget {
	mutable std::unique_ptr<System::Image> image;
	std::string text;
	std::shared_ptr<System::Font> font;
    public:
	System::Color color;

	void setFont(std::shared_ptr<System::Font> fnt) {
	    font = fnt;
	    image.reset();
	}

	Label(std::shared_ptr<System::Font> font, const System::Color& c = {}) : font{font}, color{c} {}

	Label& operator = (std::string_view str) {
	    if (str != text) {
		text = str;
		image.reset();
	    }
	    return *this;
	}

	void updateBounds(const Coord& position) override {
	    Widget::updateBounds(position);
	    if (image) {
		box.w = image->width();
		box.h = image->height();
	    }
	}

	void draw(System& sys, Coord offset) override {
	    if (text.empty())
		return;
	    if (!image)
		image = sys.createImage(text.c_str(), font.get(), color);
	    if (image) {
		offset += position;
		image->blit(offset.x, offset.y);
	    }
	}
    };

}
