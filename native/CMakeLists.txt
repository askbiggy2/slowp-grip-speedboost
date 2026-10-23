
cmake_minimum_required(VERSION 3.22)
project(SlowpGripSpeedBoost LANGUAGES CXX)

if(NOT ANDROID)
  message(FATAL_ERROR "Build with the Android NDK toolchain for arm64-v8a")
endif()

if(NOT ANDROID_ABI STREQUAL "arm64-v8a")
  message(FATAL_ERROR "Slowp Quest build requires ANDROID_ABI=arm64-v8a")
endif()

add_library(SlowpGripSpeedBoost SHARED SlowpGripSpeedBoost.cpp)

target_compile_features(SlowpGripSpeedBoost PRIVATE cxx_std_17)
target_compile_options(SlowpGripSpeedBoost PRIVATE -fvisibility=hidden -Wall -Wextra)

target_link_libraries(SlowpGripSpeedBoost PRIVATE log dl)

if(NOT DEFINED DOBBY_LIBRARY OR NOT EXISTS "${DOBBY_LIBRARY}")
  message(FATAL_ERROR "DOBBY_LIBRARY is missing or does not exist: ${DOBBY_LIBRARY}")
endif()

target_link_libraries(SlowpGripSpeedBoost PRIVATE "${DOBBY_LIBRARY}")
