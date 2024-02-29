# 设置CMake最低版本要求  
cmake_minimum_required(VERSION 3.10)  
  
# 设置项目名称  
project(encrypt_decrypt)

 # 设置C++标准为C++17
set(CMAKE_CXX_STANDARD 17)
# 强制要求使用C++17标准   
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 添加编译输出目录
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)

# 查找OpenSSL库
find_package(OpenSSL REQUIRED)
  
# 添加源代码文件  
file(GLOB_RECURSE SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")  
  
# 添加头文件目录  
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../src/source/utils
                    ${OPENSSL_INCLUDE_DIR}
                    )  

# 编译生成可执行程序
add_executable(${PROJECT_NAME} ${SOURCES})

# 添加需要链接的库文件  
target_link_libraries(${PROJECT_NAME} ${OPENSSL_LIBRARIES})