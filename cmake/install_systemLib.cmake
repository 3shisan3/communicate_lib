# 使用这个函数来获取SONAME  
function(get_library_soname libpath VARIABLE_NAME) 
    # 使用execute_process来执行shell命令  
    # 注意：我们使用sh -c来在shell中执行命令  
    execute_process(  
        COMMAND sh -c "readelf -d ${libpath} | grep 'soname'"  
        OUTPUT_VARIABLE SONAME_OUTPUT  
        RESULT_VARIABLE RESULT_CODE  
        ERROR_QUIET  
        OUTPUT_STRIP_TRAILING_WHITESPACE  
    )  

    # 检查命令是否成功执行  
    if(NOT RESULT_CODE EQUAL 0)  
        message(WARNING "Failed to get SONAME from ${libpath}")  
    endif()
  
    # 解析SONAME_OUTPUT以获取SONAME，取决于readelf的输出格式  
    string(REGEX REPLACE ".*soname: \\[(.*)\\].*" "\\1" SONAME_VALUE "${SONAME_OUTPUT}")
  
    # 将SONAME存储在提供的VARIABLE_NAME变量中  
    set(${VARIABLE_NAME} "${SONAME_VALUE}" PARENT_SCOPE)  
endfunction() 

# 判断库信息，是库名，还是具体路径
function(is_library_name_or_path VAR_NAME RESULT_VAR)  
    # 假设VAR_NAME是我们要检查的变量名  
    # RESULT_VAR是存储结果的变量名（1表示库名，0表示路径）  
  
    # 简单的检查：看变量是否包含路径分隔符  
    # 注意：这取决于你的操作系统。Windows上可能是'\'或'/'，Linux/macOS上通常是'/'  
    string(FIND "${VAR_NAME}" "/" PATH_SEPARATOR_FOUND)  
    if(PATH_SEPARATOR_FOUND GREATER -1)  
        # 如果找到了路径分隔符，我们假设这是一个路径  
        set(${RESULT_VAR} 0 PARENT_SCOPE)  
    else()  
        # 否则，我们假设这是一个库名  
        set(${RESULT_VAR} 1 PARENT_SCOPE)  
    endif()  
endfunction() 

# 获取真正的库路径
function(get_library_path LIB_NAME LIB_PATH_VAR)  
    # 假设${LIB_NAME}是库名（如yaml-cpp），并且我们要找到它的路径  
    # 尝试使用find_library来找到库文件  
    find_library(${LIB_PATH_VAR} NAMES ${LIB_NAME}  
        PATHS  
            # 在这里添加可能的库路径，例如标准库路径、第三方库的安装路径等  
            /usr/lib  
            /usr/local/lib  
            /opt/local/lib  
            # 如果yaml-cpp是通过某种方式安装的，你也可以添加它的安装路径  
            # 例如，如果它是通过vcpkg安装的，路径可能是/usr/local/vcpkg/installed/x64-linux/lib  
            # 注意：这里需要根据实际情况修改  
        NO_DEFAULT_PATH # 只在指定的路径中搜索  
        NO_CMAKE_FIND_ROOT_PATH # 不使用CMAKE_FIND_ROOT_PATH变量  
    )  
  
    # 检查是否找到了库  
    if(NOT ${LIB_PATH_VAR})  
        # 如果没有找到，打印错误消息  
        message(FATAL_ERROR "Could not find library ${LIB_NAME}. Please ensure it is installed and its path is included in the search paths.")  
    else()  
        # 如果找到了，打印找到的路径（可选）  
        message(STATUS "Found library ${LIB_NAME} at ${${LIB_PATH_VAR}}")  
    endif()  
endfunction() 

# 添加自定义安装命令，复制库文件  
function(copy_system_libs target_dir origin_list)  
	set(COPY_STAMP_FILES "")
    foreach(lib ${origin_list})  
        is_library_name_or_path(${lib} IS_LIBRARY_NAME)
        if(IS_LIBRARY_NAME)  
            get_library_path(${lib} lib)
        endif()

        # 构建目标文件的完整路径  
        get_library_soname("${lib}" lib_name)
        set(target_file "${target_dir}/${lib_name}")  
          
        # 添加自定义命令来复制单个库文件  
        add_custom_command(  
            OUTPUT ${CMAKE_BINARY_DIR}/install_libs_${lib_name}.stamp  
            COMMAND ${CMAKE_COMMAND} -E copy "${lib}" "${target_file}"     # 直接覆盖
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