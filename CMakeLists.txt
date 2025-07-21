set(MODULE_NAME "mod-server-buffs")
set(MODULE_SRCS
    src/mod-server-buffs.cpp
)

AC_ADD_SCRIPT("${CMAKE_CURRENT_LIST_DIR}/src/mod-server-buffs.cpp")

# 설정 파일 설치
install(FILES
    conf/mod-server-buffs.conf.dist
    DESTINATION "${CMAKE_INSTALL_PREFIX}/conf"
)