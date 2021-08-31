

cmake_minimum_required(VERSION 3.10)

set(OBJECT_DRM_LIBRARY_NAME object-drm)

project(${OBJECT_DRM_LIBRARY_NAME})

find_package(PkgConfig REQUIRED)
pkg_check_modules(DRM REQUIRED libdrm)

find_package(fmt REQUIRED)

add_library(${OBJECT_DRM_LIBRARY_NAME} ./src/odrm.cpp)

target_compile_features(${OBJECT_DRM_LIBRARY_NAME} PUBLIC cxx_std_17)

target_link_libraries(${OBJECT_DRM_LIBRARY_NAME} PUBLIC ${DRM_LIBRARIES} fmt::fmt
	raii)

target_include_directories(${OBJECT_DRM_LIBRARY_NAME} PUBLIC ${DRM_INCLUDE_DIRS}
	PUBLIC ./include)
