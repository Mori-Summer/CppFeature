

## 📂 CMake 中的 `file` 命令详解

CMake 中的 `file` 命令是一个非常强大的工具，用于执行各种**文件系统操作**，例如读写文件、操作路径、查找文件、计算哈希值等。

### 语法结构

`file` 命令的基本语法结构是：

CMake

```
file(<操作> <参数>...)
```

其中 `<操作>` 决定了命令要执行的具体功能。

### 常用操作及其功能

以下是 `file` 命令一些最常用的操作及其解释：

|**操作 (Operation)**|**功能描述 (Description)**|
|---|---|
|**READ**|从文件中读取内容并存储到变量中。|
|**WRITE** / **APPEND**|向文件中写入内容（`WRITE` 会覆盖原有内容，`APPEND` 会追加）。|
|**GLOB** / **GLOB_RECURSE**|查找匹配给定模式的文件。`GLOB_RECURSE` 会递归查找子目录。|
|**MAKE_DIRECTORY**|创建一个或多个目录。|
|**REMOVE** / **REMOVE_RECURSE**|删除文件（`REMOVE`）或文件/目录（`REMOVE_RECURSE`）。|
|**RENAME**|重命名或移动一个文件或目录。|
|**COPY** / **INSTALL**|复制文件或目录。`COPY` 是在配置或构建时执行，`INSTALL` 用于安装阶段。|
|**SIZE**|获取文件的大小。|
|**HASH**|计算文件的哈希值 (如 MD5, SHA1 等)。|
|**STRINGS**|从文件中读取字符串列表（常用于读取配置或源文件列表）。|
|**RELATIVE_PATH**|计算两个路径之间的相对路径。|
|**TO_CMAKE_PATH** / **TO_NATIVE_PATH**|将路径转换为 CMake 风格或本地操作系统风格。|

---

## 🛠️ 使用范例

以下是针对几种常见文件操作的 CMake 脚本范例：

### 1. 查找匹配文件 (`GLOB`)

查找当前目录下所有 `.cpp` 源文件并存入变量 `SOURCE_FILES` 中：

```
# 查找当前目录下所有 .cpp 文件
file(GLOB SOURCE_FILES "*.cpp")

# 打印找到的文件列表
message("找到的源文件: ${SOURCE_FILES}")

# 使用找到的源文件构建可执行文件
add_executable(my_app ${SOURCE_FILES})
```

**递归查找范例 (`GLOB_RECURSE`):**

查找当前目录及其所有子目录下所有 `.h` 头文件并存入变量 `HEADER_FILES` 中：

```
# 递归查找所有 .h 文件
file(GLOB_RECURSE HEADER_FILES "include/*.h") 
```

### 2. 读写文件 (`READ`, `WRITE`, `APPEND`)

**写入文件范例:** 创建一个名为 `version.txt` 的文件，并写入项目版本号。

```
set(PROJECT_VERSION "1.0.0")

# 使用 WRITE 覆盖或创建文件
file(WRITE "${CMAKE_BINARY_DIR}/version.txt" "项目版本: ${PROJECT_VERSION}\n")

# 使用 APPEND 向文件追加内容
file(APPEND "${CMAKE_BINARY_DIR}/version.txt" "构建时间: ${CMAKE_DATE_TIME}\n")
```

**读取文件范例:** 读取上面创建的 `version.txt` 的内容到变量 `file_content`。

```
# 读取文件内容
file(READ "${CMAKE_BINARY_DIR}/version.txt" file_content)

# 打印文件内容
message("读取到的文件内容:\n${file_content}")
```

### 3. 创建目录和复制文件 (`MAKE_DIRECTORY`, `COPY`)

创建一个用于存放日志的目录，并复制一个配置文件到该目录。

```
set(LOG_DIR "${CMAKE_BINARY_DIR}/logs")
set(CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/config.ini")

# 创建目录，如果不存在会创建所有父目录
file(MAKE_DIRECTORY ${LOG_DIR})

# 复制文件到新目录
# 注意：COPY 是在 CMake 配置阶段执行的
file(COPY ${CONFIG_FILE} DESTINATION ${LOG_DIR})

# 打印结果
message("配置文件已复制到: ${LOG_DIR}/config.ini")
```

### 4. 获取文件大小和哈希值 (`SIZE`, `HASH`)

获取一个文件的 MD5 值和文件大小。

```
set(TARGET_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

# 获取文件大小
file(SIZE ${TARGET_FILE} FILE_SIZE)

# 计算 MD5 哈希值
file(HASH ${TARGET_FILE} MD5 HASH_VALUE_MD5)

# 打印结果
message("文件 ${TARGET_FILE} 的大小: ${FILE_SIZE} 字节")
message("文件 ${TARGET_FILE} 的 MD5: ${HASH_VALUE_MD5}")
```
