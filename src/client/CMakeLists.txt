#把.指定路径下的所有源文件名字放进变量名SRI_LIST里面
aux_source_directory(. SRC_LIST)

#生成可执行文件muu 由SRc_LIST 变量定义的源文件编译而来
add_executable(ChatClient ${SRC_LIST})

target_link_libraries(ChatClient pthread)