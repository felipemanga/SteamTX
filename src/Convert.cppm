module;

import <cstddef>;
import <cstdint>;
import <exception>;
import <iostream>;
import <type_traits>;
import <typeindex>;
import <typeinfo>;
import <unordered_map>;

export module Convert;

namespace convert {

using ConverterFunc = void (*)(void*, const void*);
using ConverterMap = std::unordered_map<std::type_index, ConverterFunc>;

template <typename ToType>
ConverterMap& converterMap() {
    static ConverterMap map;
    return map;
}

template<typename ToType, typename FromType>
class Converter {
public:
    Converter(void (*func)(ToType*, const FromType*)) {
	converterMap<ToType>()[std::type_index(typeid(FromType))] = reinterpret_cast<ConverterFunc>(func);
    }
};

export template <typename ToType>
bool to(ToType& to, const void* from, std::type_index fromTypeHash) {
    if (std::type_index(typeid(ToType)) == fromTypeHash) {
	to = *reinterpret_cast<const ToType*>(from);
	return true;
    }
    auto& map = converterMap<ToType>();
    auto it = map.find(fromTypeHash);
    if (it == map.end())
	return false;
    it->second(&to, from);
    return true;
}

export template <typename ToType>
ToType to(const void* from, std::type_index fromTypeHash) {
    ToType t{};
    if (!to(t, from, fromTypeHash))
	throw std::exception{};
    return t;
}

inline Converter Int64FromString = +[](int64_t *o, const std::string *i) {*o = std::strtoll(i->c_str(), nullptr, 0);};
inline Converter UInt64FromString = +[](uint64_t *o, const std::string *i) {*o = std::strtoull(i->c_str(), nullptr, 0);};
inline Converter Int32FromString = +[](int32_t *o, const std::string *i) {*o = std::strtoll(i->c_str(), nullptr, 0);};
inline Converter UInt32FromString = +[](uint32_t *o, const std::string *i) {*o = std::strtoull(i->c_str(), nullptr, 0);};
inline Converter Int16FromString = +[](int16_t *o, const std::string *i) {*o = std::strtoll(i->c_str(), nullptr, 0);};
inline Converter UInt16FromString = +[](uint16_t *o, const std::string *i) {*o = std::strtoull(i->c_str(), nullptr, 0);};
inline Converter Int8FromString = +[](int8_t *o, const std::string *i) {*o = std::strtoll(i->c_str(), nullptr, 0);};
inline Converter UInt8FromString = +[](uint8_t *o, const std::string *i) {*o = std::strtoull(i->c_str(), nullptr, 0);};
inline Converter FloatFromString = +[](float *o, const std::string *i) {*o = std::strtof(i->c_str(), nullptr);};
inline Converter DoubleFromString = +[](double *o, const std::string *i) {*o = std::strtod(i->c_str(), nullptr);};
inline Converter BoolFromString = +[](bool *o, const std::string *i) {
    *o = false;
    if (i->empty())
	return;
    switch (i->c_str()[0]) {
    case 't':
    case 'T':
    case 'y':
    case 'Y':
    case '1':
	*o = true;
	break;
    default:
	break;
    }
};

inline Converter StringFromCStr = +[](std::string *o, const char * const *i) {*o = reinterpret_cast<const char*>(i);};
inline Converter CStrFromString = +[](const char **o, const std::string *i) {*o = i->c_str();};

inline Converter Int64FromCStr = +[](int64_t *o, const char *i) {*o = std::strtoll(i, nullptr, 0);};
inline Converter UInt64FromCStr = +[](uint64_t *o, const char *i) {*o = std::strtoull(i, nullptr, 0);};
inline Converter Int32FromCStr = +[](int32_t *o, const char *i) {*o = std::strtoll(i, nullptr, 0);};
inline Converter UInt32FromCStr = +[](uint32_t *o, const char *i) {*o = std::strtoull(i, nullptr, 0);};
inline Converter Int16FromCStr = +[](int16_t *o, const char *i) {*o = std::strtoll(i, nullptr, 0);};
inline Converter UInt16FromCStr = +[](uint16_t *o, const char *i) {*o = std::strtoull(i, nullptr, 0);};
inline Converter Int8FromCStr = +[](int8_t *o, const char *i) {*o = std::strtoll(i, nullptr, 0);};
inline Converter UInt8FromCStr = +[](uint8_t *o, const char *i) {*o = std::strtoull(i, nullptr, 0);};
inline Converter FloatFromCStr = +[](float *o, const char *i) {*o = std::strtof(i, nullptr);};
inline Converter DoubleFromCStr = +[](double *o, const char *i) {*o = std::strtod(i, nullptr);};
inline Converter BoolFromCStr = +[](bool *o, const char *i) {
    *o = false;
    if (!i || !i[0])
	return;
    switch (i[0]) {
    case 't':
    case 'T':
    case 'y':
    case 'Y':
    case '1':
	*o = true;
	break;
    default:
	break;
    }
};

inline Converter StringFromInt64 = +[](std::string *o, const int64_t *i) {*o = std::to_string(*i);};
inline Converter StringFromUInt64 = +[](std::string *o, const uint64_t *i) {*o = std::to_string(*i);};
inline Converter StringFromInt32 = +[](std::string *o, const int32_t *i) {*o = std::to_string(*i);};
inline Converter StringFromUInt32 = +[](std::string *o, const uint32_t *i) {*o = std::to_string(*i);};
inline Converter StringFromInt16 = +[](std::string *o, const int16_t *i) {*o = std::to_string(*i);};
inline Converter StringFromUInt16 = +[](std::string *o, const uint16_t *i) {*o = std::to_string(*i);};
inline Converter StringFromInt8 = +[](std::string *o, const int8_t *i) {*o = std::to_string(*i);};
inline Converter StringFromUInt8 = +[](std::string *o, const uint8_t *i) {*o = std::to_string(*i);};
inline Converter StringFromFloat = +[](std::string *o, const float *i) {*o = std::to_string(*i);};
inline Converter StringFromDouble = +[](std::string *o, const double *i) {*o = std::to_string(*i);};
inline Converter StringFromBool = +[](std::string *o, const bool *i) {*o = *i ? "true" : "false";};

inline Converter Int64FromFloat = +[](int64_t* o, const float* i) {*o = static_cast<int64_t>(*i);};
inline Converter UInt64FromFloat = +[](uint64_t* o, const float* i) {*o = static_cast<uint64_t>(*i);};
inline Converter Int32FromFloat = +[](int32_t* o, const float* i) {*o = static_cast<int32_t>(*i);};
inline Converter UInt32FromFloat = +[](uint32_t* o, const float* i) {*o = static_cast<uint32_t>(*i);};
inline Converter Int16FromFloat = +[](int16_t* o, const float* i) {*o = static_cast<int16_t>(*i);};
inline Converter UInt16FromFloat = +[](uint16_t* o, const float* i) {*o = static_cast<int16_t>(*i);};
inline Converter Int8FromFloat = +[](int8_t* o, const float* i) {*o = static_cast<int8_t>(*i);};
inline Converter UInt8FromFloat = +[](uint8_t* o, const float* i) {*o = static_cast<uint8_t>(*i);};

inline Converter FloatFromInt64 = +[](float* o, const int64_t* i) {*o = *i;};
inline Converter FloatFromUInt64 = +[](float* o, const uint64_t* i) {*o = *i;};
inline Converter FloatFromInt32 = +[](float* o, const int32_t* i) {*o = *i;};
inline Converter FloatFromUInt32 = +[](float* o, const uint32_t* i) {*o = *i;};
inline Converter FloatFromInt16 = +[](float* o, const int16_t* i) {*o = *i;};
inline Converter FloatFromUInt16 = +[](float* o, const uint16_t* i) {*o = *i;};
inline Converter FloatFromInt8 = +[](float* o, const int8_t* i) {*o = *i;};
inline Converter FloatFromUInt8 = +[](float* o, const uint8_t* i) {*o = *i;};

inline Converter Int64FromDouble = +[](int64_t* o, const double* i) {*o = static_cast<int64_t>(*i);};
inline Converter UInt64FromDouble = +[](uint64_t* o, const double* i) {*o = static_cast<uint64_t>(*i);};
inline Converter Int32FromDouble = +[](int32_t* o, const double* i) {*o = static_cast<int32_t>(*i);};
inline Converter UInt32FromDouble = +[](uint32_t* o, const double* i) {*o = static_cast<uint32_t>(*i);};
inline Converter Int16FromDouble = +[](int16_t* o, const double* i) {*o = static_cast<int16_t>(*i);};
inline Converter UInt16FromDouble = +[](uint16_t* o, const double* i) {*o = static_cast<int16_t>(*i);};
inline Converter Int8FromDouble = +[](int8_t* o, const double* i) {*o = static_cast<int8_t>(*i);};
inline Converter UInt8FromDouble = +[](uint8_t* o, const double* i) {*o = static_cast<uint8_t>(*i);};

inline Converter DoubleFromInt64 = +[](double* o, const int64_t* i) {*o = *i;};
inline Converter DoubleFromUInt64 = +[](double* o, const uint64_t* i) {*o = *i;};
inline Converter DoubleFromInt32 = +[](double* o, const int32_t* i) {*o = *i;};
inline Converter DoubleFromUInt32 = +[](double* o, const uint32_t* i) {*o = *i;};
inline Converter DoubleFromInt16 = +[](double* o, const int16_t* i) {*o = *i;};
inline Converter DoubleFromUInt16 = +[](double* o, const uint16_t* i) {*o = *i;};
inline Converter DoubleFromInt8 = +[](double* o, const int8_t* i) {*o = *i;};
inline Converter DoubleFromUInt8 = +[](double* o, const uint8_t* i) {*o = *i;};

}
