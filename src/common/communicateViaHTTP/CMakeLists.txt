# 设置CMake最低版本要求  
cmake_minimum_required(VERSION 3.10)  
  
# 设置项目名称  
project(http_depend_workflow)

# 设置build的文件目录 （无法生效）
set(CMAKE_CURRENT_BINARY_DIR ${ROOT_BUILD_DIR}/${PROJECT_NAME})
message(STATUS "${PROJECT_NAME} BUILD_DIR: ${CMAKE_CURRENT_BINARY_DIR}")

# # 设置C++标准为C++17
# set(CMAKE_CXX_STANDARD 17)
# # 强制要求使用C++17标准   
# set(CMAKE_CXX_STANDARD_REQUIRED True)

option(RESUME_MOUDLE_MAKE "Enable HTTP resume ability Compile" ON)
option(SERVER_MAKE "Enable HTTP server API Compile" OFF)
option(EXPOSE_UNIT_HEADER "Expose base unit to the public" OFF)

# 指定编译输出的库路径
if (EXPOSE_SUBMODULES)
	set(LIBRARY_OUTPUT_PATH ${SUBLIB_OUT_PATH})
endif()

# 依据配置生成包含的头文件
configure_file(  
  "${CMAKE_CURRENT_SOURCE_DIR}/httpcomm_structs.txt"  
  "${CMAKE_CURRENT_BINARY_DIR}/httpcomm_structs.h"  
)  

set(SHARED_HEADERS "${CMAKE_CURRENT_BINARY_DIR}/httpcomm_structs.h,${CMAKE_CURRENT_SOURCE_DIR}/reqTask/httpreq_ways.h")
set(LINK_HEADER_PATH "")
list(APPEND LINK_HEADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/reqTask)
# 添加源代码文件  
if (RESUME_MOUDLE_MAKE)
	list(APPEND SHARED_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/reqTask/special/specialsupply_req.h")
	list(APPEND LINK_HEADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/reqTask/special
							  	 ${ROOT_PROJ_DIR}/src/utils)
	file(GLOB SERVER_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/reqTask/special/*.cpp)
endif()
if (SERVER_MAKE)
	list(APPEND SHARED_HEADERS "")
	list(APPEND LINK_HEADER_PATH ${CMAKE_CURRENT_SOURCE_DIR}/server)
	file(GLOB SERVER_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/server/*.cpp")
endif()
file(GLOB COMMON_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
						 ${CMAKE_CURRENT_SOURCE_DIR}/reqTask/*.cpp) 
list(APPEND SUB_SOURCES ${COMMON_SOURCES}
						${SERVER_SOURCES})
message(STATUS "${PROJECT_NAME} SUB_SOURCES: ${SUB_SOURCES}")

# 生成静态库文件  
add_library(${PROJECT_NAME} STATIC ${SUB_SOURCES})

# 添加头文件目录  
target_include_directories(${PROJECT_NAME} PUBLIC 
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
	${LINK_HEADER_PATH}
	${ROOT_PROJ_DIR}/src/base
	${ROOT_PROJ_DIR}/thirdparty/include/workflow)

# 添加需要链接的库文件
target_link_libraries(${PROJECT_NAME} PRIVATE ${OPENSSL_LIBRARIES_NOTE}
									  PUBLIC ${ROOT_PROJ_DIR}/thirdparty/lib/libworkflow.a)

# 将对外输出文件导出
if (EXPOSE_SUBMODULES)
	install(FILES ${SHARED_HEADERS} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME})
	if (EXPOSE_UNIT_HEADER)
		install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME})
	endif()
	install(TARGETS ${PROJECT_NAME} 
		LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/inner
		ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/inner)
endif()