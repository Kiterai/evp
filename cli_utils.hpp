#include "const.hpp"
#include <filesystem>
#include <fmt/format.h>
#include <fstream>
#include <iostream>

bool is_program_exists(std::string program) {
#ifdef WIN32
    auto chk_cmd = fmt::format("where {} >nul 2>nul", program);
#else
    auto chk_cmd = fmt::format("which {} >/dev/null 2>/dev/null", program);
#endif
    return std::system(chk_cmd.c_str()) == 0;
}

auto get_executable_path(const std::string cmd) {
    std::string path;
#ifdef WIN32
    std::system(fmt::format("where {} >tmp.txt", cmd).c_str());
    {
        std::ifstream fs("tmp.txt");
        std::getline(fs, path);
    }
    std::filesystem::remove("tmp.txt");
#else
    std::system(fmt::format("which {} >tmp.txt", cmd).c_str());
    {
        std::ifstream fs("tmp.txt");
        std::getline(fs, path);
    }
    std::filesystem::remove("tmp.txt");
#endif
    return path;
}

class GitCli {
    std::string git_bin_path;

  public:
    GitCli() {
        if (is_program_exists("git")) {
            git_bin_path = get_executable_path("git");
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
            cmake_bin_path = get_executable_path("cmake");
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
            vcpkg_path = std::filesystem::path(get_executable_path("vcpkg")).parent_path().string();
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
