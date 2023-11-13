#include "config.hpp"

Config::Config() {
    auto config = YAML::LoadFile(config_file_name);

    const auto c_targets = config["targets"];
    if (!c_targets.IsMap()) {
        throw std::runtime_error("'targets' must be map");
    }

    for (const auto &c_target : c_targets) {
        targets.push_back({});
        Target &target = targets.back();

        target.name = c_target.first.as<std::string>();

        if (const auto c_main_target = config["mainTarget"]; c_main_target.IsDefined() && c_main_target.IsScalar()) {
            const auto tmp = c_main_target.as<std::string>();
            if (const auto type_node = c_targets[tmp]["type"]; type_node.IsDefined() && c_targets[tmp]["type"].as<Target::TargetType>() != Target::TargetType::Executable)
                throw std::runtime_error("main target not executable");
            main_target = tmp;
        }

        if (auto c_type = c_target.second["type"]; c_type.IsDefined())
            target.type = c_type.as<Target::TargetType>();
        else
            target.type = Target::TargetType::Executable;

        if (target.type == Target::TargetType::Executable && main_target.empty())
            main_target = target.name;

        target.src = c_target.second["src"].as<std::vector<std::string>>();

        if (auto c_depend_libs = c_target.second["dependLibs"]; c_depend_libs.IsDefined()) {
            target.dependLibs = c_depend_libs.as<std::vector<std::string>>();
        }
    }

    const auto c_depend_pkgs = config["dependPackages"];
    if (c_depend_pkgs.IsDefined()) {
        if (!c_depend_pkgs.IsMap()) {
            throw std::runtime_error("'dependPackages' must be map");
        }

        for (const auto &c_depend_pkg : c_depend_pkgs) {
            depend_packages.push_back({});
            auto &depend_package = depend_packages.back();

            depend_package.name = c_depend_pkg.first.as<std::string>();
            if (const auto c_autolink = c_depend_pkg.second["autolink"]; c_autolink.IsDefined()) {
                depend_package.autolink = c_autolink.as<bool>();
            }
        }
    }

    std::cout << "config file loaded" << std::endl;
}