cmake_minimum_required(VERSION 3.5)
project(macs_io C CXX)

set(MACS_IO_VERSION_MAJOR 2)
set(MACS_IO_VERSION_MINOR 0)
set(MACS_IO_VERSION ${MACS_IO_VERSION_MAJOR}.${MACS_IO_VERSION_MINOR} )

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(Qt6Core REQUIRED)
find_package(Qt6Gui REQUIRED)
find_package(OpenCV 4.0 REQUIRED COMPONENTS core imgproc imgcodecs)

option(BUILD_PYTHON "Build Python bindings" OFF)
if (BUILD_PYTHON)
	find_package(pybind11 REQUIRED)
	find_package (Python3 COMPONENTS Interpreter Development REQUIRED)
endif()

add_library(macs_io STATIC 
	include/macs_io.h 
	src/macs_io.cpp
)

if (BUILD_PYTHON)
	Python3_add_library(macs_io_python src/macs_io_python.cpp)
	target_link_libraries(macs_io_python PRIVATE pybind11::headers)
	set_target_properties(macs_io_python PROPERTIES
			VISIBILITY_INLINES_HIDDEN ON
			OUTPUT_NAME macs_io_python)


	#target_link_libraries(macs_io_python P pybind11::headers)
	target_link_libraries(macs_io_python PUBLIC Python3::Python)
	target_link_libraries(macs_io_python PUBLIC macs_io)
endif()

target_include_directories(macs_io 
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
	${OpenCV_INCLUDE_DIRS} 
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ DESTINATION include)
target_link_libraries(macs_io "Qt6::Core;Qt6::Gui;${OpenCV_LIBS}")

# Install targets:
install(TARGETS macs_io 
        EXPORT macs_ioExport
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION lib )

#option(BUILD_TESTING "Build the tests" ON)

option(BUILD_EXAMPLES "Build the examples" ON)
if (BUILD_EXAMPLES)
	add_executable(macs_info examples/macs_info.cpp)
	target_link_libraries(macs_info macs_io)

	add_executable(macs_decode examples/macs_decode.cpp)
	target_link_libraries(macs_decode macs_io)

	add_executable(macs_encode examples/macs_encode.cpp)
	target_link_libraries(macs_encode macs_io)
	
	install(TARGETS 
		macs_info 
		macs_decode
		macs_encode
		RUNTIME DESTINATION bin)
endif()

option(BUILD_DOCUMENTATION "Build html documentation with Sphinx." OFF)
if (BUILD_DOCUMENTATION)
  add_subdirectory(docs)
endif()

configure_file(cmake/macs_ioConfig.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/macs_ioConfig.cmake @ONLY )
configure_file(cmake/macs_ioConfigVersion.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/macs_ioConfigVersion.cmake @ONLY )

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/macs_ioConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/macs_ioConfigVersion.cmake
        DESTINATION cmake )

install(EXPORT macs_ioExport DESTINATION cmake FILE macs_ioTargets.cmake )