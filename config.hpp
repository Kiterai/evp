#pragma once

#include "const.hpp"
#include <iostream>
#include <yaml-cpp/yaml.h>

struct Target {
    std::string name;
    enum class TargetType {
        Executable,
        StaticLib,
        DynamicLib
    } type;
    std::vector<std::string> src;
    std::vector<std::string> dependLibs;
};

namespace YAML {
template <>
struct convert<Target::TargetType> {
    static Node encode(const Target::TargetType &type) {
        switch (type) {
        case Target::TargetType::Executable:
            return YAML::Node("executable");
        case Target::TargetType::StaticLib:
            return YAML::Node("static_lib");
        case Target::TargetType::DynamicLib:
            return YAML::Node("dynamic_lib");
        default:
            return YAML::Node("executable");
        }
    }
    static bool decode(const Node &node, Target::TargetType &type) {
        if (!node.IsScalar()) {
            return false;
        }
        auto type_str = node.as<std::string>();
        if (type_str == "executable") {
            type = Target::TargetType::Executable;
            return true;
        }
        if (type_str == "static_lib") {
            type = Target::TargetType::StaticLib;
            return true;
        }
        if (type_str == "dynamic_lib") {
            type = Target::TargetType::DynamicLib;
            return true;
        }
        return false;
    }
};

template <>
struct convert<std::vector<std::string>> {
    static Node encode(const std::vector<std::string> &v) {
        // TODO
    }
    static bool decode(const Node &node, std::vector<std::string> &v) {
        if (!node.IsSequence()) {
            return false;
        }
        for (const auto &elem : node) {
            if (!elem.IsScalar()) {
                return false;
            }
            v.push_back(elem.as<std::string>());
        }
        return true;
    }
};
} // namespace YAML

struct Config {
    std::string main_target;
    std::vector<Target> targets;
    Config();
};
