module;

import <random>;
import <vector>;
import <filesystem>;
import <string>;

export module utils;

std::mt19937 mt{std::random_device{}()};

export template<typename T>
T rnd() {
    static std::uniform_int_distribution<T> dist{};
    return dist(mt);
}

export template<typename T>
T rnd(T min, T max) {
    return std::uniform_int_distribution<T>{min, max}(mt);
}

export template<typename T>
T rnd(const std::vector<T>& v) {
    return v[rnd<std::size_t>(0, v.size())];
}

export std::vector<std::string> ls(const std::filesystem::path& path) {
    std::vector<std::string> ret;
    for (auto& file : std::filesystem::directory_iterator{path})
	ret.push_back(file.path());
    return ret;
}
