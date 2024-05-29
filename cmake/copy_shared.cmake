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