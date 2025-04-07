cmake_minimum_required(VERSION 3.28)
project(CppFeature)

set(CMAKE_CXX_STANDARD 23)

file(GLOB SOURCE_FILES
    ${SOURCE_FILES}
    *.h
    coroutine/*.h
    coroutine/ZExample4/*.h
)

file(GLOB SOURCE_FILES
    ${SOURCE_FILES}
    # *.cpp
    coroutine/ZExample1.cpp
    # coroutine/ZExample1-2.cpp
    # coroutine/ZExample2.cpp
    # coroutine/ZExample3.cpp
    # coroutine/ZExample4/*.cpp
    # DesignPattern/Bridge.hpp
    # main.cpp
)


add_executable(CppFeature ${SOURCE_FILES})
