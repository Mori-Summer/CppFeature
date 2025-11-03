好的，Cmake 大师这就为您提供一个**完整且符合现代实践**的示例，展示如何配置项目 A (提供库) 和项目 B (使用库)，实现通过 `find_package` 进行依赖管理。

我们将使用 **配置模式 (Config Mode)**，因为它更健壮，也是现代 CMake 的推荐做法。

---

## 🚀 完整示例：配置 `find_package`

### 📦 示例结构

假设您的文件系统结构如下：

```
.
├── ProjectA_Library  # 提供库的项目 A
│   ├── CMakeLists.txt
│   ├── include
│   │   └── mylib.h
│   ├── src
│   │   └── mylib.cpp
│   └── cmake
│       └── MyLibConfig.cmake.in  # 配置模板文件
└── ProjectB_App      # 使用库的项目 B
    ├── CMakeLists.txt
    └── main.cpp
```

### 1. 项目 A (提供库：`MyLib`)

项目 A 的目标是编译一个名为 `MyLib` 的共享库，并生成所有必需的安装文件和配置脚本。

#### 1.1 `ProjectA_Library/include/mylib.h`

C++

```
// 简单的头文件
#pragma once
#include <string>

namespace MyLib {
    void print_message(const std::string& msg);
    int add(int a, int b);
}
```

#### 1.2 `ProjectA_Library/src/mylib.cpp`

C++

```
// 简单的实现文件
#include "mylib.h"
#include <iostream>

namespace MyLib {
    void print_message(const std::string& msg) {
        std::cout << "MyLib Message: " << msg << std::endl;
    }

    int add(int a, int b) {
        return a + b;
    }
}
```

#### 1.3 `ProjectA_Library/cmake/MyLibConfig.cmake.in`

这是配置文件的模板，它会在安装时被填充和使用。

CMake

```
# @PACKAGE_INIT@ 宏会处理版本检查和前缀路径设置

# 引入导出目标文件，它定义了 MyLib::MyLib 导入目标
# ${CMAKE_CURRENT_LIST_DIR} 在安装后会指向安装目录的 lib/cmake/MyLib
include("${CMAKE_CURRENT_LIST_DIR}/MyLibTargets.cmake")

# 检查目标是否成功被引入
if(NOT TARGET MyLib::MyLib)
    # 如果目标未被引入，则配置失败，这通常不应该发生
    set(MyLib_FOUND FALSE)
    return()
endif()

# 现代 CMake 只需要 MyLib::MyLib 目标，无需设置传统变量
# 但是为了兼容性，可以设置 FOUND
set(MyLib_FOUND TRUE)
```

#### 1.4 `ProjectA_Library/CMakeLists.txt` (关键)

CMake

```
cmake_minimum_required(VERSION 3.12)
project(MyLib LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

# 1. 定义库目标
add_library(MyLib SHARED 
    src/mylib.cpp
)

# 2. 设置头文件路径和使用接口
target_include_directories(MyLib PUBLIC
    # 构建时：使用源代码目录下的 include
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    # 安装后：使用相对于安装前缀的 'include' 目录
    $<INSTALL_INTERFACE:include> 
)

# --- 安装规则定义 ---

# 3. 安装库和头文件
install(TARGETS MyLib
    EXPORT MyLibTargets  # 定义一个“导出”名称，用于生成目标文件
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include # 指示头文件安装位置，配合 target_include_directories
)

install(DIRECTORY include/ DESTINATION include) # 安装头文件目录

# 4. 生成和安装配置脚本
include(CMakePackageConfigHelpers)

# 4.1. 生成版本文件到构建目录
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/MyLibConfigVersion.cmake"
    VERSION ${MyLib_VERSION}
    COMPATIBILITY AnyNewerVersion
)

# 4.2. 配置 MyLibConfig.cmake.in 模板并生成 Config 文件到构建目录
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/MyLibConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/MyLibConfig.cmake"
    INSTALL_DESTINATION lib/cmake/MyLib # 安装后相对于前缀的位置
)

# 4.3. 导出目标文件 (MyLibTargets.cmake) 并安装
install(EXPORT MyLibTargets
    FILE MyLibTargets.cmake
    NAMESPACE MyLib::    # 设置导入目标的命名空间：MyLib::MyLib
    DESTINATION lib/cmake/MyLib
)

# 4.4. 安装最终的 Config 文件和 Version 文件
install(
    FILES
    "${CMAKE_CURRENT_BINARY_DIR}/MyLibConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/MyLibConfigVersion.cmake"
    DESTINATION lib/cmake/MyLib
)
```

### 2. 项目 B (使用库：`App`)

项目 B 使用 `find_package` 来定位并链接项目 A 提供的 `MyLib` 库。

#### 2.1 `ProjectB_App/main.cpp`

C++

```
#include <iostream>
#include "mylib.h" // 使用项目 A 提供的头文件

int main() {
    MyLib::print_message("Hello from Project B, using MyLib!");
    int result = MyLib::add(10, 5);
    std::cout << "10 + 5 = " << result << std::endl;
    return 0;
}
```

#### 2.2 `ProjectB_App/CMakeLists.txt` (关键)

CMake

```
cmake_minimum_required(VERSION 3.12)
project(MyApp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

# 1. 查找依赖包 MyLib
# REQUIRED 表示找不到就报错
find_package(MyLib 1.0 REQUIRED)

# 2. 检查是否找到包
if(MyLib_FOUND)
    message(STATUS "Found MyLib, ready to use target: MyLib::MyLib")
else()
    message(FATAL_ERROR "Could not find MyLib, please install it or set CMAKE_PREFIX_PATH.")
endif()

# 3. 创建可执行文件
add_executable(my_app main.cpp)

# 4. 链接依赖
# 使用 MyLib::MyLib 导入目标，它包含了所有头文件和链接信息
target_link_libraries(my_app PRIVATE MyLib::MyLib)
```

### 3. 构建和使用流程

要让 Project B 成功找到 Project A，您需要先构建并“安装”项目 A。

**第一步：构建和安装 Project A**

Bash

```
# 进入 ProjectA_Library 目录
cd ProjectA_Library

# 创建并进入构建目录
mkdir build
cd build

# 配置项目 A，指定安装前缀（这里使用自定义路径 _install）
# 生产环境中，通常会安装到 /usr/local 或其他标准路径
cmake .. -DCMAKE_INSTALL_PREFIX=./_install

# 编译项目
cmake --build .

# 执行安装。这是关键一步，它将 MyLib.so/dll、mylib.h 和 MyLibConfig.cmake/MyLibTargets.cmake 放入 _install 目录
cmake --install .
```

安装成功后，在 `ProjectA_Library/build/_install/lib/cmake/MyLib` 目录下应该可以看到 `MyLibConfig.cmake` 和 `MyLibTargets.cmake`。

**第二步：构建 Project B**

Project B 默认不知道 Project A 安装在哪里，所以我们需要通过设置 `CMAKE_PREFIX_PATH` 来告诉它。

Bash

```
# 回到根目录
cd ../..

# 进入 ProjectB_App 目录
cd ProjectB_App
mkdir build
cd build

# 配置项目 B
# 关键：通过 CMAKE_PREFIX_PATH 指向 Project A 的安装目录
cmake .. -DCMAKE_PREFIX_PATH="../../ProjectA_Library/build/_install"

# 编译项目
cmake --build .

# 运行应用程序
./my_app
# 预期输出:
# MyLib Message: Hello from Project B, using MyLib!
# 10 + 5 = 15
```

这样，您就成功地使用 `find_package(MyLib)` 导入了项目 A 提供的目标并完成了链接。