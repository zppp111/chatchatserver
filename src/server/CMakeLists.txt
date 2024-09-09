

#把.指定路径下的所有源文件名字放进变量名SRI_LIST里面
aux_source_directory(. SRC_LIST)
#把db源文件名字放进变量名SRI_LIST里面
aux_source_directory(./db DB_LIST)
aux_source_directory(./model MODEL_LIST)
aux_source_directory(./redis MODEL_LIST)
#生成可执行文件muu 由SRc_LIST 变量定义的源文件编译而来
add_executable(ChatServer ${SRC_LIST} ${DB_LIST} ${MODEL_LIST} ${REDIS_LIST})

#执行可执行文件链接时可依赖的库文件
#target_link_libraries(ChatServer /usr/lib64/mysql/libmysqlclient.so )
#target_link_libraries(ChatServer muduo_net muduo_base pthread  ${MYSQLCLIENT_LIBRARY})

target_link_libraries(ChatServer /usr/local/lib/libhiredis.so )
target_link_libraries(ChatServer /usr/lib64/mysql/libmysqlclient.so )
target_link_libraries(ChatServer muduo_net muduo_base pthread )