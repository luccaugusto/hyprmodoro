cmake_minimum_required(VERSION 3.27)

project(hyprmodoro
    DESCRIPTION "Hyprmodoro plugin for Hyprland"
    VERSION 0.1
)

set(CMAKE_CXX_STANDARD 23)

file(GLOB_RECURSE SRC "src/*.cpp")

add_library(hyprmodoro SHARED ${SRC})

find_package(PkgConfig REQUIRED)
pkg_check_modules(deps REQUIRED IMPORTED_TARGET
    hyprland
    libdrm
    pangocairo
    pixman-1
)
target_link_libraries(hyprmodoro PRIVATE rt PkgConfig::deps)

install(TARGETS hyprmodoro)