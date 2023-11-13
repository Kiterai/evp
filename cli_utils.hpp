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
