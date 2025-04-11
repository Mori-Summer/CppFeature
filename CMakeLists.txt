cmake_minimum_required(VERSION 3.28)
project(CppFeature)

set(CMAKE_CXX_STANDARD 23)

file(GLOB SOURCE_FILES
    ${SOURCE_FILES}
    *.h
    coroutine/*.h
)

file(GLOB SOURCE_FILES
    ${SOURCE_FILES}
    # *.cpp
    # DesignPattern/Bridge.hpp
    # main.cpp
    coroutine/*.cpp
)


add_executable(CppFeature ${SOURCE_FILES})
