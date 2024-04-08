module;

import <any>;
import <fstream>;
import <string>;
import <typeindex>;
import <typeinfo>;
import <unordered_map>;
import <vector>;

import Convert;

export module Registry;

export namespace registry {

class Value {
    const void* raw;
    std::any storage;
public:
    template<typename From>
    Value(From&& f) {
	storage = std::forward<From>(f);
	raw = &std::any_cast<From&>(storage);
    }

    Value(const char* cstr) {
	storage = cstr;
	raw = cstr;
    }

    template <typename To>
    To to(To o = {}) {
	convert::to<To>(o, raw, std::type_index{storage.type()});
	return o;
    }
};

}

inline std::unordered_map<std::string, registry::Value>& valueRegistry() {
    static std::unordered_map<std::string, registry::Value> reg;
    return reg;
}

export namespace registry {

template <typename Type = std::string>
Type lookup(const std::string& key, const Type& def = {}) {
    auto& reg = valueRegistry();
    auto it = reg.find(key);
    if (it == reg.end())
	return def;
    return it->second.to(def);
}

void load(const std::string& path) {
    auto& reg = valueRegistry();
    std::ifstream stream{path};
    std::string domain, key, value;

    enum State {
	Init,
	Key,
	PostKey,
	PreValue,
	Value,
	Domain,
	Comment
    } state{};

    while (true) {
	char c{};
	if (!stream.read(&c, 1)) {
	    if (state == Init)
		return;
	    c = '\n';
	}
	if (c == '\r')
	    c = '\n';
	switch (state) {
	case Init:
	    if (c <= 32)
		continue;
	    if (c == '[') {
		domain.clear();
		state = Domain;
		continue;
	    }
	    if (c == '#') {
		state = Comment;
		continue;
	    }
	    state = Key;
	    key = domain;
	    if (!domain.empty())
		key.push_back('.');
	    [[fallthrough]];
	case Key:
	    if (c == '\n') {
		state = Init;
		continue;
	    }
	    if (c == '=') {
		state = PreValue;
		continue;
	    }
	    if (c <= 32) {
		state = PostKey;
		continue;
	    }
	    key.push_back(c);
	    continue;
	case PostKey:
	    if (c == '\n')
		state = Init;
	    if (c != '=')
		continue;
	    state = PreValue;
	    continue;
	case PreValue:
	    if (c == '\n') {
		state = Init;
		continue;
	    }
	    if (c <= 32)
		continue;
	    state = Value;
	    [[fallthrough]];
	case Value:
	    if (c == '\n') {
		state = Init;
		reg.emplace(std::pair<std::string, registry::Value>(key, value));
		value.clear();
		continue;
	    }
	    value.push_back(c);
	    continue;
	case Comment:
	    if (c == '\n')
		state = Init;
	    continue;
	case Domain:
	    if (c == '\n' || c == ']') {
		state = Init;
		if (c != ']')
		    domain.clear();
		continue;
	    }
	    if (c <= 32)
		continue;
	    domain.push_back(c);
	    continue;
	}
    }
}

}
