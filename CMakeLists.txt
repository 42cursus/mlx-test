# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    CMakeLists.txt                                     :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: abelov <abelov@student.42london.com>       +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/05/09 16:13:19 by abelov            #+#    #+#              #
#    Updated: 2025/05/09 16:13:20 by abelov           ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

cmake_minimum_required(VERSION 3.8)
set(CMAKE_C_COMPILER clang)
#set(CMAKE_C_COMPILER gcc)

project(mlx-test C)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_C_STANDARD 99)
include_directories(include)

set(FT_CFLAGS
        -Wall -Wextra -Werror
        -Wwrite-strings
        -Wimplicit
        -mavx
        -mavx2
        -Wformat -Werror=format-security
        -Wstrict-prototypes
        -Wmissing-prototypes
        -fstack-protector-strong
        -Wno-error=unused-parameter
        -Wno-error=unused-variable
        -O0
        -pg
        -masm=att
        -g3 -gdwarf-3
        -fasynchronous-unwind-tables
        -fno-omit-frame-pointer
        -fsanitize=address
        -fsanitize=undefined
        -fno-sanitize-recover=all
        -fsanitize=float-divide-by-zero
        -fsanitize=float-cast-overflow
)

set(COMMON_LDFLAGS
        -pg
        -fsanitize=address
        -fsanitize=undefined
        -fno-sanitize-recover=all
        -fsanitize=float-divide-by-zero
        -fsanitize=float-cast-overflow
)

file(GLOB_RECURSE TARGET_HEADERS "src/*.h")
file(GLOB_RECURSE TARGET_SOURCES "src/*.c")

set(TARGET_NAME "${PROJECT_NAME}_exe")

find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(X11 REQUIRED) # https://cmake.org/cmake/help/latest/module/FindX11.html
find_library(MATH_LIB m)
# Locate additional libraries that are not covered by FindX11
find_library(XT_LIBRARY Xt REQUIRED)
find_library(XM_LIBRARY Xm REQUIRED)
find_library(MRM_LIBRARY Mrm REQUIRED)
find_library(XPM_LIBRARY Xpm REQUIRED)

set(TARGET_LIBRARIES
        ${MATH_LIB}
        ${FT_LIB_NAME}
        Threads::Threads
        ${X11_LIBRARIES}
        ${XT_LIBRARY}
        ${XM_LIBRARY}
        ${MRM_LIBRARY}
        ${XPM_LIBRARY}
        Xrandr  # Xrandr is usually part of X11_LIBRARIES but may need explicit linking
        Xext    # Xext is typically in X11_LIBRARIES, but we ensure it's linked
)

add_executable(${TARGET_NAME}
        ${TARGET_HEADERS}
        ${TARGET_SOURCES})

target_include_directories(${TARGET_NAME} PRIVATE
        ${PROJECT_SOURCE_DIR}
        /usr/include
        /usr/include/x86_64-linux-gnu)

target_link_libraries(${TARGET_NAME} "${TARGET_LIBRARIES}")
target_link_options(${TARGET_NAME} PRIVATE ${COMMON_LDFLAGS})
target_compile_options(${TARGET_NAME} PRIVATE "${FT_CFLAGS}")
target_compile_definitions(${TARGET_NAME} PRIVATE FRAMERATE=50)
set_target_properties(${TARGET_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}"
        OUTPUT_NAME "${PROJECT_NAME}")

#==============================================================================#
# Custom targets
#==============================================================================#
add_custom_target(fclean
        COMMAND make -C ${FT_LIB_DIR} fclean)

add_custom_target(re
        COMMAND make -C ${FT_LIB_DIR} re
        COMMAND make -C ${PROJECT_SOURCE_DIR} re)

add_custom_target(norm
        COMMAND make -C ${FT_LIB_DIR} norm
        COMMAND make -C ${PROJECT_SOURCE_DIR} norm)
