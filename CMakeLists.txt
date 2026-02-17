cmake_minimum_required(VERSION 3.14)
project(arduino_remote_storage VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
    arduino_core
    GIT_REPOSITORY https://github.com/compilerNayan/arduino_core.git
    GIT_TAG        main
)
FetchContent_Declare(
    cpp_core
    GIT_REPOSITORY https://github.com/compilerNayan/cpp_core.git
    GIT_TAG        main
)
FetchContent_MakeAvailable(arduino_core cpp_core)

add_library(arduino_remote_storage INTERFACE)

set_target_properties(arduino_remote_storage PROPERTIES
    VERSION ${PROJECT_VERSION}
)

target_include_directories(arduino_remote_storage INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>
)

if(TARGET arduino_core::arduino_core)
    target_link_libraries(arduino_remote_storage INTERFACE arduino_core::arduino_core)
elseif(TARGET arduino_core)
    target_link_libraries(arduino_remote_storage INTERFACE arduino_core)
else()
    FetchContent_GetProperties(arduino_core)
    if(arduino_core_SOURCE_DIR)
        target_include_directories(arduino_remote_storage INTERFACE
            $<BUILD_INTERFACE:${arduino_core_SOURCE_DIR}/src>
        )
    endif()
endif()

FetchContent_GetProperties(cpp_core)
if(cpp_core_SOURCE_DIR)
    target_include_directories(arduino_remote_storage INTERFACE
        $<BUILD_INTERFACE:${cpp_core_SOURCE_DIR}/include>
    )
endif()
