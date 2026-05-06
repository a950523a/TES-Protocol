# TES-Protocol: TES-0D-02-01 portable C99 CAN codec + types
# Zero platform dependencies -- compiles on any C99 toolchain.
#
# ESP-IDF:    used as a component (idf_component_register)
# STM32/PC:   add_library / target_include_directories

if(ESP_PLATFORM)
    idf_component_register(
        SRCS "tes_codec.c"
        INCLUDE_DIRS "include"
        # No REQUIRES: pure C99, zero platform/OS dependencies
    )
else()
    cmake_minimum_required(VERSION 3.16)
    project(tes_protocol C)
    add_library(tes_protocol STATIC tes_codec.c)
    target_include_directories(tes_protocol PUBLIC include)
    target_compile_features(tes_protocol PUBLIC c_std_99)
endif()
