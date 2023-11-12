#include <argparse/argparse.hpp>
#include <filesystem>
#include <fmt/format.h>
#include <fstream>
#include <iostream>
#include <yaml-cpp/yaml.h>

constexpr auto evp_version = "0.5";
constexpr auto config_file_name = "evp.yml";
constexpr auto build_directory_name = "build";
#ifdef _WIN32
constexpr auto path_separator = '\\';
#else
constexpr auto path_separator = '/';
#endif

bool is_program_exists(std::string program) {
#ifdef WIN32
    auto chk_cmd = fmt::format("where {} >nul 2>nul", program);
#else
    auto chk_cmd = fmt::format("which {} >/dev/null 2>/dev/null", program);
#endif
    return std::system(chk_cmd.c_str()) == 0;
}

class GitCli {
    std::string git_bin_path;

  public:
    GitCli() {
        if (is_program_exists("git")) {
#ifdef WIN32
            std::system("where git >tmp.txt");
            {
                std::ifstream fs("tmp.txt");
                std::getline(fs, git_bin_path);
            }
            std::filesystem::remove("tmp.txt");
#else
            std::system("which git >tmp.txt");
            {
                std::ifstream fs("tmp.txt");
                std::getline(fs, git_bin_path);
            }
            std::filesystem::remove("tmp.txt");
#endif
            return;
        }
        std::cout << "git not found" << std::endl;
        // install git
    }
    auto exec(std::string arg) {
        return std::system(fmt::format("\"{}{}git\" {}", git_bin_path, path_separator, arg).c_str());
    }
};

class CmakeCli {
    std::string cmake_bin_path;

  public:
    CmakeCli() {
        if (is_program_exists("cmake")) {
#ifdef WIN32
            std::system("where cmake >tmp.txt");
            {
                std::ifstream fs("tmp.txt");
                std::getline(fs, cmake_bin_path);
            }
            std::filesystem::remove("tmp.txt");
#else
            std::system("which cmake >tmp.txt");
            {
                std::ifstream fs("tmp.txt");
                std::getline(fs, cmake_bin_path);
            }
            std::filesystem::remove("tmp.txt");
#endif
            return;
        }
        std::cout << "cmake not found" << std::endl;
        // install cmake
    }
    auto exec(std::string arg) {
        return std::system(fmt::format("\"{}\" {}", cmake_bin_path, arg).c_str());
    }
};

class VcpkgCli {
    std::string vcpkg_path;

  public:
    VcpkgCli() {
        auto tmp = std::getenv("VCPKG_ROOT");
        if (tmp != nullptr)
            vcpkg_path = tmp;
        else if (is_program_exists("vcpkg")) {
            std::cout << "VCPKG_ROOT not set" << std::endl;
#ifdef WIN32
            std::system("where vcpkg >tmp.txt");
            {
                std::ifstream fs("tmp.txt");
                std::getline(fs, vcpkg_path);
            }
            std::filesystem::remove("tmp.txt");
#else
            std::system("which vcpkg >tmp.txt");
            {
                std::ifstream fs("tmp.txt");
                std::getline(fs, vcpkg_path);
            }
            std::filesystem::remove("tmp.txt");
#endif

            vcpkg_path = std::filesystem::path(vcpkg_path).parent_path().string();
            std::cout << "vcpkg found: " << vcpkg_path << std::endl;
        } else {
            std::cout << "vcpkg not found" << std::endl;
            // git clone vcpkg
            // update.bat
            // set vcpkg_root
        }
    }
    auto exec(std::string arg) {
        return std::system(fmt::format("\"{}{}vcpkg\" {}", vcpkg_path, path_separator, arg).c_str());
    }
    auto get_path() {
        return vcpkg_path;
    }
};

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
    Config() {
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
};

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
    // vcpkg_cli.exec(fmt::format("install {}", pkg));

    // create build directory
    if (!std::filesystem::exists(build_directory_name))
        std::filesystem::create_directory(build_directory_name);
    std::filesystem::current_path(build_directory_name);

    // write CMakeLists.txt
    {
        std::ofstream fs("CMakeLists.txt");

        for (const auto &target : config.targets) {
            // target definition
            fs << "add_executable(" << target.name << ' ';
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
    cmake_cli.exec(fmt::format(". -DCMAKE_TOOLCHAIN_FILE={}/scripts/buildsystems/vcpkg.cmake >log.txt 2>&1", vcpkg_cli.get_path()));
    cmake_cli.exec("--build . >> log.txt 2>&1");

    std::cout << "build done" << std::endl;
}

void action_build(const argparse::ArgumentParser &arg) {
    Config config;
    build(arg, config);
}

void action_run(const argparse::ArgumentParser &arg) {
    Config config;
    build(arg, config);

    std::cout << "executing \"" << config.main_target << "\"\n" << std::endl;

    std::system(fmt::format(".\\Debug\\{}", config.main_target).c_str());
}

void action_add(const argparse::ArgumentParser &arg) {
    std::cout << "add" << std::endl;
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
        std::cout << target.name << " : " << YAML::Node(target.type) << std::endl;
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
    cmd_build.add_description("Build current directory project");
    program.add_subparser(cmd_build);

    argparse::ArgumentParser cmd_run("run");
    cmd_run.add_description("Run current directory project");
    program.add_subparser(cmd_run);

    argparse::ArgumentParser cmd_add("add");
    cmd_add.add_description("Add package dependency (default to main target)");
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