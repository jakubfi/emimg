add_library(emimg-lib SHARED
	emimg.c
	disk.c
	mtape.c
	ptape.c
)

set_target_properties(emimg-lib PROPERTIES
	OUTPUT_NAME "emimg"
	SOVERSION ${EMIMG_VERSION_MAJOR}.${EMIMG_VERSION_MINOR}
	PUBLIC_HEADER ${CMAKE_SOURCE_DIR}/include/emimg.h
)

install(TARGETS emimg-lib
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

add_executable(emimg
	emimg-tool.c
)

target_link_libraries(emimg emimg-lib)

install(TARGETS emimg
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# vim: tabstop=4
