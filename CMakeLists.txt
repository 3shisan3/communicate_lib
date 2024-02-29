# 设置CMake最低版本要求  
cmake_minimum_required(VERSION 3.10)  
  
# 设置项目名称  
project(communicate_lib)

set(CMAKE_INSTALL_PREFIX "~/program_running/ota_by_workflow")

# 设置构建选项前提
option(BUILD_TEST "build test proj option"	ON)
option(BUILD_PROJECT "build proj option"	ON)
option(EXPOSE_SUBMODULES "create .a to libpath"	ON)

#设置构建中用到的变量
set(ROOT_PROJ_DIR ${PROJECT_SOURCE_DIR})
set(ROOT_BUILD_DIR ${CMAKE_BINARY_DIR})
message(STATUS "ROOT_BUILD_DIR: ${CMAKE_CURRENT_BINARY_DIR}")

# 设置C++标准为C++17
set(CMAKE_CXX_STANDARD 17)
# 强制要求使用C++17标准   
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 对外输出主目录
set(SHARED_ROOT_DIR ${PROJECT_SOURCE_DIR}/sdk)
# 指定编译输出的动态库路径
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${SHARED_ROOT_DIR}/lib)
set(SUBLIB_OUT_PATH ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/inner)

# 将源文件夹中的内容拷贝到目标文件夹下, 该操作与构建流程无关
add_custom_target(
	LINK_HEADERS ALL
	COMMENT "link headers and config..."
)
add_custom_command(
	TARGET LINK_HEADERS PRE_BUILD
	COMMAND ${CMAKE_COMMAND} -E make_directory ${SHARED_ROOT_DIR}/include
	COMMAND ${CMAKE_COMMAND} -E make_directory ${SHARED_ROOT_DIR}/config
)
add_custom_command(
	TARGET LINK_HEADERS PRE_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PROJECT_SOURCE_DIR}/src/interface/*/*.h ${SHARED_ROOT_DIR}/include
	COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PROJECT_SOURCE_DIR}/src/source/*/*.json ${SHARED_ROOT_DIR}/config
	DEPENDS ${PROJECT_SOURCE_DIR}/sdk/include AND ${SHARED_ROOT_DIR}/config
)

# 查找OpenSSL库
# find_package(OpenSSL REQUIRED)

# 子模块库存储
list(APPEND SUBMOUDLES_DIR_LIST
		${PROJECT_SOURCE_DIR}/src/common/communicateViaHTTP
		${PROJECT_SOURCE_DIR}/src/common/communicateViaMQTT)

# 添加源代码文件  
file(GLOB_RECURSE SOURCES "${PROJECT_SOURCE_DIR}/src/*.cpp")  

# 记录子项目的源文件
set(SubSources "")
# 添加模块编译项
foreach(subproject ${SUBMOUDLES_DIR_LIST})
	# 添加子模块编译
	add_subdirectory(${subproject})
	# 排除子模块文件在父项目的源文件
	file(GLOB_RECURSE subSources "${subproject}/*.cpp")
	list(APPEND SubSources ${subSources})
	foreach(subSource ${subSources})
		list(REMOVE_ITEM SOURCES "${subSource}")
	endforeach()
endforeach()
# message(STATUS "SOURCES: ${SOURCES}")

# 设置排除链接编译的目录（混在主目录下的子目录）
# add_subdirectory(${PROJECT_SOURCE_DIR}/src/interface/dpi_ota EXCLUDE_FROM_ALL)
# add_subdirectory(${PROJECT_SOURCE_DIR}/src/source/dpi_ota EXCLUDE_FROM_ALL)
  
# 添加头文件目录  
include_directories(${PROJECT_SOURCE_DIR}/src
					${SUBMOUDLES_DIR_LIST}
                    )


# 指定了在安装目标库时，库的运行时依赖项应该被安装到哪个目录
set(CMAKE_INSTALL_RPATH "$ORIGIN")
# 设置变量来禁用构建目标库时生成运行时搜索路径,与上变量互斥
# set(CMAKE_SKIP_RPATH TRUE)
# 生成动态库文件
add_library(${PROJECT_NAME} SHARED ${SOURCES})
# 链接依赖关系，将子项目的源文件添加到主项目的构建过程中
target_sources(${PROJECT_NAME} PUBLIC ${SubSources})

# 设置需要一并输出依赖的动态库列表
set(DEPENDENCY_LIB "")

# 添加条件需要链接的库文件 
# if(OpenSSL_FOUND)
#     message(STATUS "OpenSSL found")
# 	include_directories(${OPENSSL_INCLUDE_DIR})
#	list(APPEND DEPENDENCY_LIB ${OPENSSL_LIBRARIES})
# 	target_link_libraries(${PROJECT_NAME} ${OPENSSL_LIBRARIES})
# else()
#     message(STATUS "OpenSSL not found")
	include_directories(${PROJECT_SOURCE_DIR}/thirdparty/include/openssl_3)
	list(APPEND DEPENDENCY_LIB
		${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/lib/libssl.so.3
		${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/lib/libcrypto.so.3
	)
	target_link_libraries(${PROJECT_NAME} ${DEPENDENCY_LIB})
# endif()

# 添加动态库的搜索路径
# link_directories(${CMAKE_CURRENT_SOURCE_DIR}/sdk/lib)
# 添加需要链接的库文件
target_link_libraries(${PROJECT_NAME} http_depend_workflow
									  mqtt_depend_mosquitto)

# 测试项目
if (BUILD_TEST)
	# 设置日志打印级别
	add_definitions(-DGLOBAL_LOG_LEVEL=1)

	message(STATUS "build test modules")
    # add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/test/ota_app ${CMAKE_BINARY_DIR}/ota_app)
	# add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/test/mqtt_demo ${CMAKE_BINARY_DIR}/mqtt_demo)
	# add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/test/websocket_test ${CMAKE_BINARY_DIR}/websocket_test)
endif()
# 输出项目
if (ROOT_PROJ_DIR)
	message(STATUS "build project")
	# add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/project/dpi_ota ${CMAKE_BINARY_DIR}/dpi_ota)
endif()

# 将对外输出文件导出
install(DIRECTORY ${SHARED_ROOT_DIR}/ DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/project/${PROJECT_NAME})
install(FILES ${DEPENDENCY_LIB} DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/project/third_lib)

# 设置RPATH, 确保运行时可以找到依赖库
set_target_properties(${PROJECT_NAME} PROPERTIES INSTALL_RPATH "$ORIGIN;$ORIGIN/lib;$ORIGIN/third_lib;$ORIGIN/../third_lib")