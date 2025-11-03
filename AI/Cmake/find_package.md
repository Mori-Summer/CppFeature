
## 🔍 `find_package` 命令详解

`find_package` 是 CMake 中用于管理外部依赖关系的核心命令。它的主要作用是定位并加载一个外部库或包的配置文件，从而将该包提供的目标、变量和宏导入到当前项目中。

### 1. 基本语法和模式

`find_package` 命令有多种语法形式，主要取决于您希望的查找严格程度和导入的组件。

#### 核心语法

CMake

```
find_package(<PackageName> [version] [REQUIRED] [COMPONENTS <comps>...])
```

|**参数**|**说明**|
|---|---|
|`<PackageName>`|必需，要查找的包的名称（例如：`Boost`, `OpenCV`, `Qt5`）。|
|`[version]`|可选，要求的最低版本（例如：`3.12.0`）。|
|`[REQUIRED]`|可选，如果找不到包，则立即终止配置（`FATAL_ERROR`）。这是最常见的用法。|
|`[COMPONENTS <comps>...]`|可选，指定该包内部必需的组件（例如：对于 Qt5，可以是 `Core` `Widgets`）。|

#### 查找模式（`QUIET` 和 `CONFIG`）

- **`QUIET`**: 抑制找不到包时的信息性输出（通常用于尝试查找可选依赖）。
    
- **`NO_MODULE`**: 强制只使用**配置模式 (Config Mode)** 进行查找。
    
- **`CONFIG`**: 强制只使用**模块模式 (Module Mode)** 进行查找。
    

### 2. 查找机制：模块模式 vs. 配置模式

CMake 通过两种主要模式查找包：

#### A. 模块模式 (Module Mode)

这是 CMake **先尝试**的模式。它查找一个名为 `Find<PackageName>.cmake` 的文件。

- **文件位置**: 通常位于 CMake 安装目录下的 `Modules/` 文件夹中，或通过 `CMAKE_MODULE_PATH` 变量指定的路径。
    
- **如何工作**: 这个 `Find` 文件是 CMake 社区或您自己编写的脚本，它使用 `find_library()`、`find_path()` 等基础命令来定位库文件、头文件、并设置一些变量（例如：`<PackageName>_INCLUDE_DIRS`、`<PackageName>_LIBRARIES`）。
    
- **输出**: 成功后，它通常会设置如下变量：
    
    - `PackageName_FOUND`：布尔值，表示是否找到包。
        
    - `PackageName_INCLUDE_DIRS`：头文件路径。
        
    - `PackageName_LIBRARIES`：库文件路径。
        

#### B. 配置模式 (Config Mode)

这是**现代 CMake 推荐**的模式，也是您配置自己项目的主要目标。它查找一个名为 `<PackageName>Config.cmake` 或 `<packagename>-config.cmake` 的文件。

- **文件位置**: 位于安装路径下的特定子目录中（例如：`lib/cmake/<PackageName>/` 或 `share/<PackageName>/`）。
    
- **如何工作**: 这个配置文件通常是由**被查找的包**在安装时自动生成或手动编写的。它不依赖于 `find_library()`，而是直接定义了导入的**目标 (Targets)**。
    
- **输出**: 成功后，它会创建**导入目标 (Imported Targets)**，例如 `PackageName::Component` 或 `PackageName::PackageName`。
    

### 3. 现代用法：导入目标 (Imported Targets)

在配置模式下成功调用 `find_package` 后，我们应该使用**导入目标**来链接依赖，而不是使用变量。

**现代 CMake 推荐做法：**

CMake

```
find_package(MyLib 3.0 REQUIRED)

# 使用导入目标进行链接，这是推荐的现代方式
# 导入目标包含了链接路径、头文件路径、定义等所有信息
target_link_libraries(my_app PRIVATE MyLib::Core) 
```

---

## 📦 配置自己的 Package (Config Mode)

要让您的项目 A 能够被另一个项目 B 通过 `find_package(A)` 找到，您需要利用 CMake 提供的 `install(EXPORT)` 和 `export()` 命令来生成配置模式所需的文件。

这被称为 **Creating a Package (创建包)**。

### 步骤 1: 为目标添加安装规则

在您的项目 A 的 `CMakeLists.txt` 中，确保您为要暴露的库（目标）定义了安装规则。

CMake

```
# 1. 创建您的库
add_library(MyLib SHARED mylib.cpp mylib.h)
set_target_properties(MyLib PROPERTIES VERSION 1.0 SOVERSION 1) # 设置版本信息
target_include_directories(MyLib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include> # 告知安装后头文件在哪里
)

# 2. 为目标创建一个 EXPORT 名称
# 这将用于生成 MyLibTargets.cmake 文件
install(TARGETS MyLib
    EXPORT MyLibTargets # 关键：定义导出的名称
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

# 3. 安装头文件
install(DIRECTORY include/ DESTINATION include)
```

### 步骤 2: 生成和安装配置模块

接下来，您需要生成并安装查找所需的配置脚本（即 `MyLibConfig.cmake` 和 `MyLibTargets.cmake`）。

CMake

```
# 4. 生成和安装目标文件
# MyLibTargets.cmake 会包含所有导入目标（MyLib::MyLib）的定义
install(EXPORT MyLibTargets
    FILE MyLibTargets.cmake 
    NAMESPACE MyLib:: # 为目标添加命名空间前缀，现代推荐
    DESTINATION lib/cmake/MyLib # 安装到专用的配置目录下
)

# 5. 生成和安装 Config 文件
# 使用 CMake 提供的宏来生成 MyLibConfig.cmake
include(CMakePackageConfigHelpers)

# 生成一个 MyLibConfigVersion.cmake 文件
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/MyLibConfigVersion.cmake"
    VERSION 1.0
    COMPATIBILITY AnyNewerVersion
)

# 配置（创建）MyLibConfig.cmake 文件
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/MyLibConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/MyLibConfig.cmake"
    INSTALL_DESTINATION lib/cmake/MyLib
)

# 安装生成的 Config 文件
install(FILES 
    "${CMAKE_CURRENT_BINARY_DIR}/MyLibConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/MyLibConfigVersion.cmake"
    DESTINATION lib/cmake/MyLib
)
```

### 步骤 3: 编写配置模板 (`MyLibConfig.cmake.in`)

您需要在项目 A 的源代码目录中创建一个模板文件（例如 `cmake/MyLibConfig.cmake.in`），供 `configure_package_config_file` 使用。

CMake

```
# MyLibConfig.cmake.in 示例

# 检查版本
include(CMakeFindDependencyMacro)

# 如果 MyLib 依赖了其他包 (如 ZLIB)，则需要在这里 find_dependency
# find_dependency(ZLIB REQUIRED)

# 包含导入的目标定义文件
# ${CMAKE_CURRENT_LIST_DIR} 在安装后会指向 lib/cmake/MyLib
include("${CMAKE_CURRENT_LIST_DIR}/MyLibTargets.cmake")

# 现代 CMake 只需要导入目标，无需设置传统变量，但为了兼容性可以设置
if(NOT TARGET MyLib::MyLib)
    # 如果没有使用命名空间目标，可能需要设置传统变量
    # set(MyLib_INCLUDE_DIRS "${PACKAGE_PREFIX_DIR}/include")
    # set(MyLib_LIBRARIES MyLib::MyLib)
endif()

# 设置 MyLib_FOUND 为 TRUE
set(MyLib_FOUND TRUE)
```

### 总结使用流程

1. **项目 A (提供者)**:
    
    - `add_library(...)`
        
    - `install(TARGETS ... EXPORT MyLibTargets ...)`
        
    - `install(EXPORT MyLibTargets ...)`
        
    - 运行 `cmake` 配置，`make` 编译，最后执行 `make install` 将文件安装到系统或指定目录。
        
2. **项目 B (使用者)**:
    
    - 执行 `find_package(MyLib 1.0 REQUIRED)`。
        
    - 如果 MyLib 被安装在非标准位置，可能需要设置 `CMAKE_PREFIX_PATH` 来告诉 CMake 去哪里找。
        
    - 使用 **导入目标**：`target_link_libraries(myapp PRIVATE MyLib::MyLib)`。
        
