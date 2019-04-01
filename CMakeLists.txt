cmake_minimum_required (VERSION 3.4)
project (libnfc-jni)

find_package(JNI)
if (NOT JNI_FOUND)
    message (FATAL_ERROR "JNI Could not be found!")
endif()


set(java_src    "${CMAKE_SOURCE_DIR}/src/main/java")
set(java_bin    "${CMAKE_BINARY_DIR}/../classes")
set(cpp_src     "${CMAKE_SOURCE_DIR}/src/main/cpp")
set(cpp_gen_src "${CMAKE_BINARY_DIR}/../cpp-src")

set(java_nsrc   "${java_src}/uk/co/polarmoment/bacta/libnfc_jni/jni")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE java_nsrc_glob "${java_nsrc}/*.java")
file(GLOB_RECURSE c_src          "${cpp_src}/*.c")
file(GLOB_RECURSE cpp_src        "${cpp_src}/*.cpp")

foreach(loop_var IN LISTS java_nsrc_glob)
	set(original_file "${loop_var}")
	file(RELATIVE_PATH loop_var "${java_src}" "${loop_var}")
	string(REPLACE ".java" "" loop_var "${loop_var}")
	string(REPLACE "/" "." loop_var "${loop_var}")
	
	set(class_name "${loop_var}")
	string(REPLACE "." "_" header_file "${loop_var}")
	set(header_file "${header_file}.h")
	list(APPEND java_classes "${class_name}")
	list(APPEND header_files "${header_file}")
	
	add_custom_command(OUTPUT "${header_file}" COMMAND javah ARGS -d "${cpp_gen_src}" -classpath "${java_bin}" -force "${loop_var}" DEPENDS "${original_file}")                  
endforeach()

include_directories("${cpp_gen_src}")
include_directories("${cpp_src}")
include_directories(${JNI_INCLUDE_DIRS})
link_libraries(${JNI_LIBRARIES})

add_library(libnfc-jni SHARED ${cpp_src} ${c_src})
 
add_custom_target(header_files DEPENDS ${header_files})
add_dependencies("libnfc-jni" header_files)
