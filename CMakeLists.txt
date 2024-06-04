cmake_minimum_required(VERSION 3.28)
project(CppFeature)

set(CMAKE_CXX_STANDARD 20)

file(GLOB SOURCE_FILES
    ${SOURCE_FILES}
    *.h
    #coroutine/*.h
)

file(GLOB SOURCE_FILES
    ${SOURCE_FILES}
    *.cpp
    #coroutine/*.cpp
)


add_executable(CppFeature ${SOURCE_FILES})
