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
# 复制库到安装目录的third_lib子目录  
set(INSTALL_LIB_DIR "${CMAKE_INSTALL_PREFIX}/third_lib")  

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

# 对外输出主目录(编译产出)
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
	DEPENDS ${SHARED_ROOT_DIR}/include AND ${SHARED_ROOT_DIR}/config
)
# 添加自定义安装命令，复制库文件  
function(copy_system_libs target_dir origin_list)  
	set(COPY_STAMP_FILES "")
    foreach(lib ${origin_list})  
        # 构建目标文件的完整路径  
        get_filename_component(lib_name "${lib}" NAME)  
        set(target_file "${target_dir}/${lib_name}")  
          
        # 添加自定义命令来复制单个库文件  
        add_custom_command(  
            OUTPUT ${CMAKE_BINARY_DIR}/install_libs_${lib_name}.stamp  
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${lib}" "${target_file}"  
            COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_BINARY_DIR}/install_libs_${lib_name}.stamp  
            COMMENT "Copying ${lib_name} to install directory"  
            VERBATIM  
        )  
          
        # 将所有单个库文件的stamp文件添加到install_openssl_libs目标的依赖中  
        list(APPEND COPY_STAMP_FILES ${CMAKE_BINARY_DIR}/install_libs_${lib_name}.stamp)  
    endforeach()  
      
    # 创建一个自定义目标，它依赖于所有单个库文件的stamp文件  
    add_custom_target(install_libs ALL DEPENDS ${COPY_STAMP_FILES})  
endfunction()  

# 查找OpenSSL库
find_package(OpenSSL REQUIRED)

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
# 临时单独去除websocket相关的编译
file(GLOB_RECURSE subSources "${PROJECT_SOURCE_DIR}/src/common/communicateViaWebSocket/*.cpp")
list(REMOVE_ITEM SOURCES ${subSources})
message(STATUS "SOURCES: ${SOURCES}")

# 设置排除链接编译的目录（混在主目录下的子目录）
# add_subdirectory(${PROJECT_SOURCE_DIR}/src/interface/dpi_ota EXCLUDE_FROM_ALL)
# add_subdirectory(${PROJECT_SOURCE_DIR}/src/source/dpi_ota EXCLUDE_FROM_ALL)
  
# 添加头文件目录  
include_directories(${PROJECT_SOURCE_DIR}/src
					${SUBMOUDLES_DIR_LIST}
                    )


# 指定了在安装目标库时，库的运行时依赖项应该被安装到哪个目录
# set(CMAKE_INSTALL_RPATH "$ORIGIN")
# 设置变量来禁用构建目标库时生成运行时搜索路径,与上变量互斥
# set(CMAKE_SKIP_RPATH TRUE)
# 生成动态库文件
add_library(${PROJECT_NAME} SHARED ${SOURCES})
# 链接依赖关系，将子项目的源文件添加到主项目的构建过程中
target_sources(${PROJECT_NAME} PUBLIC ${SubSources})

# 设置需要一并输出依赖的动态库列表
set(DEPENDENCY_LIB "")

# 添加条件需要链接的库文件 
if(OpenSSL_FOUND)
    message(STATUS "OpenSSL found")
	include_directories(${OPENSSL_INCLUDE_DIR}) 
	target_link_libraries(${PROJECT_NAME} ${OPENSSL_LIBRARIES})

	# list(APPEND DEPENDENCY_LIB ${OPENSSL_LIBRARIES})

	# 自定义安装命令，复制库文件  
	copy_system_libs("${INSTALL_LIB_DIR}" "${OPENSSL_LIBRARIES}")
else()
    message(STATUS "OpenSSL not found")
	include_directories(${PROJECT_SOURCE_DIR}/thirdparty/include/openssl_3)
	target_link_libraries(${PROJECT_NAME} ${DEPENDENCY_LIB})

	list(APPEND DEPENDENCY_LIB
		${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/lib/libssl.so.3
		${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/lib/libcrypto.so.3
	)
endif()

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
install(DIRECTORY ${SHARED_ROOT_DIR}/ DESTINATION ${CMAKE_INSTALL_PREFIX})
install(FILES ${DEPENDENCY_LIB} DESTINATION ${CMAKE_INSTALL_PREFIX}/third_lib)
# 确保在make install时运行自定义安装命令  
install(CODE "EXECUTE_PROCESS(COMMAND ${CMAKE_COMMAND} --build . --target install_libs)")


# 设置RPATH, 确保运行时可以找到依赖库
set_target_properties(${PROJECT_NAME} PROPERTIES INSTALL_RPATH "$ORIGIN;$ORIGIN/lib;$ORIGIN/third_lib;$ORIGIN/../third_lib")