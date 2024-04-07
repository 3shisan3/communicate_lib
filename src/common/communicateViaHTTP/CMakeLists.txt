# 设置CMake最低版本要求  
cmake_minimum_required(VERSION 3.10)  
  
# 设置项目名称  
project(http_depend_workflow)

# 设置build的文件目录 （无法生效）
set(CMAKE_CURRENT_BINARY_DIR ${ROOT_BUILD_DIR}/${PROJECT_NAME})
message(STATUS "${PROJECT_NAME} BUILD_DIR: ${CMAKE_CURRENT_BINARY_DIR}")

# 设置C++标准为C++17
set(CMAKE_CXX_STANDARD 17)
# 强制要求使用C++17标准   
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 指定编译输出的库路径
if (EXPOSE_SUBMODULES)
	set(LIBRARY_OUTPUT_PATH ${SUBLIB_OUT_PATH})
endif()

# 添加源代码文件  
file(GLOB SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")  

# 生成静态库文件  
add_library(${PROJECT_NAME} STATIC ${SOURCES})

# 添加头文件目录  
target_include_directories(${PROJECT_NAME} PUBLIC 
	${CMAKE_CURRENT_SOURCE_DIR}
	${ROOT_PROJ_DIR}/src/base
	${ROOT_PROJ_DIR}/thirdparty/include/workflow)

# 添加需要链接的库文件
target_link_libraries(${PROJECT_NAME} ${ROOT_PROJ_DIR}/thirdparty/lib/libworkflow.a)

# 将对外输出文件导出
if (EXPOSE_SUBMODULES)
	install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/httpreq_ways.h DESTINATION ${ROOT_BUILD_DIR}/project/${PROJECT_NAME}/include)
	install(TARGETS ${PROJECT_NAME} 
		LIBRARY DESTINATION ${ROOT_BUILD_DIR}/project/${PROJECT_NAME}/lib
		ARCHIVE DESTINATION ${ROOT_BUILD_DIR}/project/${PROJECT_NAME}/static_lib)
endif()