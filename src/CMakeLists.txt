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
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
	PUBLIC_HEADER DESTINATION include
)

add_executable(emimg
	emimg-tool.c
)

target_link_libraries(emimg emimg-lib)

install(TARGETS emimg
	RUNTIME DESTINATION bin
)

# vim: tabstop=4
