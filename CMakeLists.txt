# 设置CMake最低版本要求  
cmake_minimum_required(VERSION 3.10)  
  
# 设置项目名称  
project(communicate_lib)

# 判断操作系统类型  
if(UNIX)  
    # 在 Unix-like 系统中使用 $HOME 环境变量  
    execute_process(COMMAND sh -c "echo $HOME" OUTPUT_VARIABLE USER_HOME_DIR)  
    string(STRIP ${USER_HOME_DIR} USER_HOME_DIR)  
elseif(WIN32)  
    # 在 Windows 中使用 %USERPROFILE% 环境变量  
    execute_process(COMMAND cmd /C "echo %USERPROFILE%" OUTPUT_VARIABLE USER_HOME_DIR)  
    string(STRIP ${USER_HOME_DIR} USER_HOME_DIR)  
    # 将反斜杠替换为正斜杠（可选，但可以使路径更易于处理）  
    string(REPLACE "\\" "/" USER_HOME_DIR ${USER_HOME_DIR})  
endif() 

set(CMAKE_INSTALL_PREFIX "${USER_HOME_DIR}/program_running/${PROJECT_NAME}")

# 设置构建选项前提
option(BUILD_TEST "编译测试部分用例"	ON)
option(BUILD_PROJECT "编译整体项目的工程"	ON)
option(EXPOSE_SUBMODULES "子模块额外输出编译成果"	ON)

#设置构建中用到的变量
set(ROOT_PROJ_DIR ${PROJECT_SOURCE_DIR})
set(ROOT_BUILD_DIR ${CMAKE_BINARY_DIR})
set(THIRD_DEPEND_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty)
message(STATUS "ROOT_BUILD_DIR: ${CMAKE_CURRENT_BINARY_DIR}")

# 设置C++标准为C++17
set(CMAKE_CXX_STANDARD 17)
# 强制要求使用C++17标准   
set(CMAKE_CXX_STANDARD_REQUIRED True)
# 编译位置无关代码标志添加
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")  

# 对外输出主目录(编译产出)
set(SHARED_ROOT_DIR ${PROJECT_SOURCE_DIR}/sdk)
# 指定编译输出的动态库路径
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${SHARED_ROOT_DIR}/lib)
set(SUBLIB_OUT_PATH ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/inner)

# 设置自定义模块的存放位置
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(copy_shared)
include(install_systemLib)

# 查找OpenSSL库
find_package(OpenSSL REQUIRED)

# 添加条件需要链接的库文件
if(OpenSSL_FOUND)
    message(STATUS "OpenSSL found")
	set(OPENSSL_INCLUDE_DIRS_NOTE ${OPENSSL_INCLUDE_DIR})
	list(APPEND OPENSSL_LIBRARIES_NOTE ${OPENSSL_LIBRARIES})

	# 自定义安装命令，复制库文件  
	copy_system_libs("${CMAKE_INSTALL_PREFIX}/third_lib" "${OPENSSL_LIBRARIES}")
else()
    message(STATUS "OpenSSL not found")
	set(OPENSSL_INCLUDE_DIRS_NOTE ${PROJECT_SOURCE_DIR}/thirdparty/include/openssl_3)
	list(APPEND OPENSSL_LIBRARIES_NOTE
		${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/lib/libssl.so.3
		${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/lib/libcrypto.so.3
	)
endif()
message(STATUS "${PROJECT_NAME} OPENSSL_INCLUDE_DIRS: ${OPENSSL_INCLUDE_DIRS_NOTE}")
message(STATUS "${PROJECT_NAME} OPENSSL_LIBRARIES: ${OPENSSL_LIBRARIES_NOTE}")

# 添加源代码文件  
file(GLOB_RECURSE SOURCES "${PROJECT_SOURCE_DIR}/src/*.cpp")  
# 子模块库存储
list(APPEND SUBMOUDLES_DIR_LIST
		${PROJECT_SOURCE_DIR}/src/common/communicateViaHTTP
		${PROJECT_SOURCE_DIR}/src/common/communicateViaMQTT
)
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
# 临时单独去除websocket相关的编译
file(GLOB_RECURSE subSources "${PROJECT_SOURCE_DIR}/src/common/communicateViaWebSocket/*.cpp")
list(REMOVE_ITEM SOURCES ${subSources})
message(STATUS "SOURCES: ${SOURCES}")

# 设置排除链接编译的目录（混在主目录下的子目录）
# add_subdirectory(${PROJECT_SOURCE_DIR}/src/interface/dpi_ota EXCLUDE_FROM_ALL)
# add_subdirectory(${PROJECT_SOURCE_DIR}/src/source/dpi_ota EXCLUDE_FROM_ALL)
  
# 添加头文件目录  
include_directories(${OPENSSL_INCLUDE_DIRS_NOTE}
					${PROJECT_SOURCE_DIR}/src
					# ${SUBMOUDLES_DIR_LIST}	子项目头文件包含设为PUBLIC，此处可忽略
)

# 生成动态库文件
add_library(${PROJECT_NAME} SHARED ${SOURCES})


# 添加动态库的搜索路径
# link_directories(${CMAKE_CURRENT_SOURCE_DIR}/sdk/lib)
# 添加需要链接的库文件
target_link_libraries(${PROJECT_NAME} ${OPENSSL_LIBRARIES_NOTE}
									  http_depend_workflow
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


# 添加必要的安装动态库文件
file(GLOB SHARED_LIBS ${THIRD_DEPEND_DIR}/lib/*.so.*)
# 添加对外暴露头文件
file(GLOB_RECURSE SHARED_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/src/interface/*.h)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
install(FILES ${SHARED_LIBS} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
install(FILES ${SHARED_INCLUDES} DESTINATION ${CMAKE_INSTALL_PREFIX}/include)

# 确保在make install时运行自定义安装命令  
install(CODE "EXECUTE_PROCESS(COMMAND ${CMAKE_COMMAND} --build . --target install_libs)")

# 设置RPATH, 确保运行时可以找到依赖库
set_target_properties(${PROJECT_NAME} PROPERTIES INSTALL_RPATH "$ORIGIN;$ORIGIN/third_lib;$ORIGIN/../third_lib;$ORIGIN/lib;$ORIGIN/../lib")