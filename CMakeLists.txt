cmake_minimum_required(VERSION 2.8.4)
project(emimg C)
include(appversion.cmake)
set(DEBUG 1)

# software version

# image format version
set(EMI_FORMAT_V_MAJOR 2)
set(EMI_FORMAT_V_MINOR 0)

include_directories(${CMAKE_SOURCE_DIR}/include)

add_definitions(-Wall -std=c99)
add_definitions(-DEMIMG_VERSION_MAJOR=${APP_VERSION_MAJOR} -DEMIMG_VERSION_MINOR=${APP_VERSION_MINOR} -DEMIMG_VERSION_PATCH=${APP_VERSION_PATCH})
add_definitions(-DEMI_FORMAT_V_MAJOR=${EMI_FORMAT_V_MAJOR} -DEMI_FORMAT_V_MINOR=${EMI_FORMAT_V_MINOR})

if(DEBUG)
	add_definitions(-DDEBUG -g -O0)
else(DEBUG)
	add_definitions(-DNDEBUG -O3)
endif(DEBUG)

configure_file(
	emimg-config.cmake.in
	${PROJECT_BINARY_DIR}/emimg-config.cmake
	@ONLY
)

configure_file(
	emimg-config-version.cmake.in
	${PROJECT_BINARY_DIR}/emimg-config-version.cmake
	@ONLY
)

install(FILES
	${PROJECT_BINARY_DIR}/emimg-config.cmake
	${PROJECT_BINARY_DIR}/emimg-config-version.cmake
	DESTINATION lib/emimg
	COMPONENT dev
)

add_subdirectory(src)

# vim: tabstop=4
