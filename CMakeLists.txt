cmake_minimum_required(VERSION 3.25)

project(DN1FM VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(DN1_BUILD_PLUGIN "Build the VST3 plug-in (requires the Steinberg VST3 SDK)" ON)
option(DN1_BUILD_TESTS "Build the portable engine tests" ON)

add_library(dn1_engine STATIC
    src/engine/dn1_engine.cpp
    src/engine/dn1_engine.hpp
    src/engine/dn1_parameters.hpp
)
target_include_directories(dn1_engine PUBLIC src)
target_compile_features(dn1_engine PUBLIC cxx_std_17)
set_target_properties(dn1_engine PROPERTIES POSITION_INDEPENDENT_CODE ON)

if(MSVC)
    target_compile_options(dn1_engine PRIVATE /W4 /permissive-)
else()
    target_compile_options(dn1_engine PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(DN1_BUILD_TESTS)
    enable_testing()
    add_executable(dn1_engine_tests tests/test_engine.cpp)
    target_link_libraries(dn1_engine_tests PRIVATE dn1_engine)
    add_test(NAME dn1_engine_tests COMMAND dn1_engine_tests)
endif()

if(DN1_BUILD_PLUGIN)
    set(vst3sdk_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/vst3sdk"
        CACHE PATH "Path to a checked-out Steinberg VST3 SDK")
    if(NOT EXISTS "${vst3sdk_SOURCE_DIR}/CMakeLists.txt")
        message(FATAL_ERROR
            "VST3 SDK not found at ${vst3sdk_SOURCE_DIR}. "
            "Run scripts/bootstrap_vst3sdk.sh or configure with -Dvst3sdk_SOURCE_DIR=/path.")
    endif()

    # The SDK's example projects and VSTGUI are not needed by this headless,
    # generic-editor plug-in. Keeping them off makes the build much smaller.
    set(SMTG_ENABLE_VST3_HOSTING_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(SMTG_ENABLE_VSTGUI_SUPPORT OFF CACHE BOOL "" FORCE)

    add_subdirectory("${vst3sdk_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/vst3sdk")
    smtg_enable_vst3_sdk()

    set(DN1_PLUGIN_SOURCES
        src/plugin/controller.cpp
        src/plugin/controller.hpp
        src/plugin/cids.hpp
        src/plugin/factory.cpp
        src/plugin/param_ids.hpp
        src/plugin/processor.cpp
        src/plugin/processor.hpp
        src/plugin/version.h
    )

    smtg_add_vst3plugin(DN1FM ${DN1_PLUGIN_SOURCES})
    target_compile_features(DN1FM PRIVATE cxx_std_17)
    target_include_directories(DN1FM PRIVATE src)
    target_link_libraries(DN1FM PRIVATE sdk dn1_engine)
    smtg_target_configure_version_file(DN1FM)

    if(SMTG_MAC)
        smtg_target_set_bundle(DN1FM
            BUNDLE_IDENTIFIER "com.arena.dn1fm"
            COMPANY_NAME "DN1FM"
        )
    endif()
endif()
