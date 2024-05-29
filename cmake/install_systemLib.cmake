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


# 添加自定义安装命令，复制库文件  
function(copy_system_libs target_dir origin_list)  
	set(COPY_STAMP_FILES "")
    foreach(lib ${origin_list})  
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