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
    # coroutine/ZExample1.cpp
    # coroutine/ZExample1-2.cpp
    coroutine/ZExample2.cpp
)


add_executable(CppFeature ${SOURCE_FILES})
