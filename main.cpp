#include "cli_utils.hpp"
#include "config.hpp"
#include <argparse/argparse.hpp>
#include <filesystem>
#include <fmt/format.h>
#include <fstream>
#include <iostream>
#include <yaml-cpp/yaml.h>

void action_init(const argparse::ArgumentParser &arg) {
    const auto project_name = arg.get("name");
    std::cout << "initializing project \"" << project_name << "\"..." << std::endl;

    // create project directory
    std::filesystem::create_directory(project_name);
    std::filesystem::current_path(project_name);

    std::filesystem::create_directory("src");
    {
        std::ofstream fs(".gitignore");
        fs << "build/" << std::endl;
    }
    {
        std::ofstream fs(config_file_name);

        YAML::Node main_target;
        main_target["src"].push_back("main.cpp");

        YAML::Node conf;
        conf["version"] = evp_version;
        conf["mainTarget"] = project_name;
        conf["targets"][project_name] = main_target;

        fs << conf;
    }
    {
        std::ofstream fs("src/main.cpp");

        fs << "#include <iostream>\n"
              "\n"
              "int main() {\n"
              "  std::cout << \"Hello World!\" << std::endl;\n"
              "  return 0;\n"
              "}";
    }

    std::cout << "done." << std::endl;
}

void build(const argparse::ArgumentParser &arg, const Config &config) {
    std::cout << "starting build..." << std::endl;

    CmakeCli cmake_cli;
    VcpkgCli vcpkg_cli;

    // prepare packages
    for (const auto &pkg : config.depend_packages) {
        if (vcpkg_cli.exec(fmt::format("install {} >nul 2>&1", pkg.name)) != 0) { // error occurs when to use log.txt
            throw std::runtime_error(fmt::format("failed to install package \"{}\"", pkg.name));
        }
    }

    // create build directory
    if (!std::filesystem::exists(build_directory_name))
        std::filesystem::create_directory(build_directory_name);
    std::filesystem::current_path(build_directory_name);

    // write CMakeLists.txt
    {
        std::ofstream fs("CMakeLists.txt");

        for (const auto &target : config.targets) {
            // target definition
            switch (target.type) {
            case Target::TargetType::Executable:
                fs << "add_executable(" << target.name << " ";
                break;
            case Target::TargetType::DynamicLib:
                fs << "add_library(" << target.name << " SHARED ";
                break;
            case Target::TargetType::StaticLib:
                fs << "add_library(" << target.name << " STATIC ";
                break;
            default:
                throw std::runtime_error("Unknown target type");
            }
            for (const auto &src_file : target.src) {
                fs << "../src/" << src_file << ' ';
            }
            fs << ")" << std::endl;

            if (target.dependLibs.size() > 0) {
                fs << "target_link_libraries(" << target.name << " PRIVATE ";
                for (const auto &lib : target.dependLibs) {
                    fs << lib << ' ';
                }
                fs << ")" << std::endl;
            }
        }
    }

    // build
    cmake_cli.exec(fmt::format(". -DCMAKE_TOOLCHAIN_FILE={}/scripts/buildsystems/vcpkg.cmake >>log.txt 2>&1", vcpkg_cli.get_path()));
    cmake_cli.exec("--build . >> log.txt 2>&1");

    std::cout << "build done" << std::endl;
}

void action_build(const argparse::ArgumentParser &arg) {
    Config config;
    build(arg, config);
}

void action_run(const argparse::ArgumentParser &arg) {
    Config config;
    if (config.main_target.empty()) {
        throw std::runtime_error("Executable target not appeared");
    }

    build(arg, config);

    std::cout << "executing \"" << config.main_target << "\"\n"
              << std::endl;

    std::system(fmt::format(".\\Debug\\{}", config.main_target).c_str());
}

void action_add(const argparse::ArgumentParser &arg) {
    auto config = YAML::LoadFile(config_file_name);
    auto pkg_name = arg.get("name");

    auto depend_packages = config["dependPackages"];
    if (depend_packages[pkg_name].IsDefined()) {
        std::cout << "\"" << pkg_name << "\" already registered" << std::endl;
    } else {
        depend_packages[pkg_name] = YAML::Node();

        std::ofstream fs(config_file_name, std::ios_base::trunc);
        fs << config;
        std::cout << "\"" << pkg_name << "\" successfully registered" << std::endl;
    }
}

void action_target_add(const argparse::ArgumentParser &arg) {
    auto config = YAML::LoadFile(config_file_name);

    const auto new_target_name = arg.get("name");

    if (!std::filesystem::exists("src/bin"))
        std::filesystem::create_directory("src/bin");
    if (!std::filesystem::exists("src/bin/" + new_target_name + ".cpp")) {
        std::ofstream fs("src/bin/" + new_target_name + ".cpp");

        fs << "#include <iostream>\n"
              "\n"
              "int main() {\n"
              "  std::cout << \"Hello World!\" << std::endl;\n"
              "  return 0;\n"
              "}";
    }

    YAML::Node new_target;
    new_target["src"].push_back("bin/" + new_target_name + ".cpp");

    config["targets"][new_target_name] = new_target;

    std::ofstream fs(config_file_name, std::ios_base::trunc);
    fs << config;
}

void action_target_list(const argparse::ArgumentParser &arg) {
    Config config;

    for (const auto &target : config.targets) {
        std::cout << target.name << " : " << YAML::Node(target.type);
        if (target.name == config.main_target)
            std::cout << " (main target)";
        std::cout << std::endl;
    }
}

void action_target_remove(const argparse::ArgumentParser &arg) {
    auto config = YAML::LoadFile(config_file_name);

    const auto del_target_name = arg.get("name");
    config["targets"].remove(del_target_name);

    std::ofstream fs(config_file_name, std::ios_base::trunc);
    fs << config;
}

int main(int argc, char **argv) {
    argparse::ArgumentParser program("evp", evp_version);

    argparse::ArgumentParser cmd_init("init");
    cmd_init.add_description("Initialize a project");
    cmd_init.add_argument("name")
        .help("Specifie project name, This value will also be used as directory name");
    program.add_subparser(cmd_init);

    argparse::ArgumentParser cmd_build("build");
    cmd_build.add_description("Build project");
    program.add_subparser(cmd_build);

    argparse::ArgumentParser cmd_run("run");
    cmd_run.add_description("Build project and Run main target");
    program.add_subparser(cmd_run);

    argparse::ArgumentParser cmd_add("add");
    cmd_add.add_description("Add package dependency");
    cmd_add.add_argument("name")
        .help("Specifie package(port) name");
    program.add_subparser(cmd_add);

    argparse::ArgumentParser cmd_target("target");
    cmd_target.add_description("Edit target config");
    program.add_subparser(cmd_target);

    argparse::ArgumentParser cmd_target_add("add");
    cmd_target_add.add_description("Add new target");
    cmd_target_add.add_argument("name")
        .help("Specifie new target name");
    cmd_target.add_subparser(cmd_target_add);

    argparse::ArgumentParser cmd_target_list("list");
    cmd_target_list.add_description("List targets");
    cmd_target.add_subparser(cmd_target_list);

    argparse::ArgumentParser cmd_target_remove("remove");
    cmd_target_remove.add_description("Remove existing target");
    cmd_target_remove.add_argument("name")
        .help("Specifie removing target name");
    cmd_target.add_subparser(cmd_target_remove);

    try {
        program.parse_args(argc, argv);
    } catch (const std::exception &err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        return 1;
    }

    try {
        if (program.is_subcommand_used("init"))
            action_init(cmd_init);
        else if (program.is_subcommand_used("build"))
            action_build(cmd_build);
        else if (program.is_subcommand_used("run"))
            action_run(cmd_run);
        else if (program.is_subcommand_used("add"))
            action_add(cmd_add);
        else if (program.is_subcommand_used("target"))
            if (cmd_target.is_subcommand_used("add"))
                action_target_add(cmd_target_add);
            else if (cmd_target.is_subcommand_used("list"))
                action_target_list(cmd_target_list);
            else if (cmd_target.is_subcommand_used("remove"))
                action_target_remove(cmd_target_remove);
            else
                cmd_target.print_help();
        else
            program.print_help();
    } catch (std::exception e) {
        std::cerr << "error: " << e.what() << std::endl;
    }

    return 0;
}