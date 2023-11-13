#pragma once

constexpr auto evp_version = "0.5";
constexpr auto config_file_name = "evp.yml";
constexpr auto build_directory_name = "build";
#ifdef _WIN32
constexpr auto path_separator = '\\';
#else
constexpr auto path_separator = '/';
#endif
