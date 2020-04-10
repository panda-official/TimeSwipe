#include "board_iface.hpp"
#include <nlohmann/json.hpp>

BoardInterface* BoardInterface::_instance = nullptr;

nlohmann::json str2json(const std::string& str) {
    nlohmann::json j;
    try {
        j = nlohmann::json::parse(str);
    } catch (nlohmann::json::parse_error& e) {
        std::cerr << "BoardInterface: json parse failed data:" << str << "error:" << e.what() << '\n';
        return nlohmann::json();
    }
    return j;
}

template <class OUT>
bool json_get(const nlohmann::json& j, const std::string& key, OUT& value);

template <>
bool json_get(const nlohmann::json& j, const std::string& key, std::string& value) {
    auto it = j.find(key);
    if (it == j.end()) return false;
    if (!it->is_string()) return false;
    value = it->get<std::string>();
    return true;
}

template <>
bool json_get(const nlohmann::json& j, const std::string& key, uint32_t& value) {
    auto it = j.find(key);
    if (it == j.end()) return false;
    if (!it->is_number_unsigned()) return false;
    value = it->get<uint32_t>();
    return true;
}

template <>
bool json_get(const nlohmann::json& j, const std::string& key, float& value) {
    auto it = j.find(key);
    if (it == j.end()) return false;
    if (!it->is_number_float()) return false;
    value = it->get<float>();
    return true;
}

template <>
bool json_get(const nlohmann::json& j, const std::string& key, bool& value) {
    auto it = j.find(key);
    if (it == j.end()) return false;
    if (!it->is_boolean()) return false;
    value = it->get<bool>();
    return true;
}

bool BoardInterface::getPWM(uint8_t num, bool& active, uint32_t& frequency, uint32_t& high, uint32_t& low, uint32_t& repeats, float& duty_cycle) {
    std::string pwm = std::string("PWM") + std::to_string(num+1);
    auto arr = nlohmann::json::array({pwm, pwm + ".freq", pwm + ".high", pwm + ".low", pwm + ".repeats", pwm + ".duty"});
    std::string err;
    auto settings = getGetSettings(arr.dump(), err);
    auto s = str2json(settings);
    if (s.empty()) return false;

    if (!json_get(s, pwm, active)) return false;

    if (!json_get(s, pwm + ".freq", frequency)) return false;

    if (!json_get(s, pwm + ".high", high)) return false;

    if (!json_get(s, pwm + ".low", low)) return false;

    if (!json_get(s, pwm + ".repeats", repeats)) return false;

    if (!json_get(s, pwm + ".duty", duty_cycle)) return false;

    return true;
}

bool BoardInterface::startPWM(uint8_t num, uint32_t frequency, uint32_t high, uint32_t low, uint32_t repeats, float duty_cycle) {

    std::string pwm = std::string("PWM") + std::to_string(num+1);
    auto obj = nlohmann::json::object({});
    obj.emplace(pwm + ".freq", frequency);
    obj.emplace(pwm + ".high", high);
    obj.emplace(pwm + ".low", low);
    obj.emplace(pwm + ".repeats", repeats);
    obj.emplace(pwm + ".duty", duty_cycle);
    std::string err;

    auto settings = getSetSettings(obj.dump(), err);
    if (str2json(settings).empty()) {
        return false;
    }

    obj.emplace(pwm, true);
    settings = getSetSettings(obj.dump(), err);

    return !str2json(settings).empty();
}
