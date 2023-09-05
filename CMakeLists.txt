# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

set(ZEPHYR_EXTRA_MODULES $ENV{ZEPHYR_BASE}../../modules/zpp)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(Project)

FILE(GLOB app_sources src/main.cpp)
target_sources(app PRIVATE ${app_sources})

include_directories(inc)
