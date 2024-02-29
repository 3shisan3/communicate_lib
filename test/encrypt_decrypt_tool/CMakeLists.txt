# 设置CMake最低版本要求  
cmake_minimum_required(VERSION 3.10)  

# 设置项目名称  
project(encrypt_decrypt_tool LANGUAGES CXX)

# 指定编译输出的动态库路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# 若要使用qt的ui相关功能，需开启，否则无找到对应ui_xxx.h
# set(CMAKE_AUTOUIC ON)
# 设置MOC，如果要使用信号槽，则必须开启该功能
set(CMAKE_AUTOMOC ON)
# 设置自动处理资源文件的功能，将自动查找项目中的.qrc文件，并将其转换为C++代码
# set(CMAKE_AUTORCC ON)

# 设置C++标准为C++17
set(CMAKE_CXX_STANDARD 17)

# 设置QT库编译路径
set(CMAKE_PREFIX_PATH "/home/shisan/Qt/6.6.0/gcc_64")

# 查找环境中的QT库
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

# 添加源代码文件  
file(GLOB_RECURSE SOURCES "${PROJECT_SOURCE_DIR}/src/*.cpp"
                          "${PROJECT_SOURCE_DIR}/main.cpp")
  
# 添加头文件目录  
include_directories(${PROJECT_SOURCE_DIR}/src
                    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/source/utils
)

# 生成编译程序
add_executable(${PROJECT_NAME} ${SOURCES})

# 链接库文件
target_link_libraries(${PROJECT_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

