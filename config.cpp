#include "config.hpp"

Config::Config() {
    auto config = YAML::LoadFile(config_file_name);

    if (const auto c_main_target = config["main_target"]; c_main_target.IsDefined())
        main_target = c_main_target.as<std::string>();
    const auto c_targets = config["targets"];
    if (!c_targets.IsMap()) {
        throw std::runtime_error("'targets' must be map");
    }
    for (const auto &c_target : c_targets) {
        targets.push_back({});
        Target &target = targets.back();

        target.name = c_target.first.as<std::string>();
        if (main_target.empty())
            main_target = target.name;

        if (auto c_type = c_target.second["type"]; c_type.IsDefined())
            target.type = c_type.as<Target::TargetType>();
        else
            target.type = Target::TargetType::Executable;

        target.src = c_target.second["src"].as<std::vector<std::string>>();

        if (auto c_depend_libs = c_target.second["dependLibs"]; c_depend_libs.IsDefined()) {
            target.dependLibs = c_depend_libs.as<std::vector<std::string>>();
        }
    }

    std::cout << "config file loaded" << std::endl;
}