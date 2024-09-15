set(project_name ipc)
cmake_minimum_required (VERSION 3.18)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-D_WIN32_WINNT=0x0601)

if(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}" )
	set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS}")
	set(CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS}")
else()
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}" )
	set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS}")
	set(CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS}")
endif()

project(${project_name} LANGUAGES C CXX CUDA)

include(conan)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_VERBOSE_MAKEFILE TRUE)

conan_check(VERSION 1.16.0 REQUIRED)
conan_cmake_run(CONANFILE conanfile.py BASIC_SETUP CMAKE_TARGETS KEEP_RPATHS UPDATE)

set(include_files
	include/ipc/SharedMemory.h
	include/ipc/SharedMemoryWindows.h
)

set(source_files
	src/SharedMemory.cpp
)

set(meggitt-cli-files
	src/cli/meggitt/Meggitt.cpp
	src/cli/meggitt/MeggittData.cpp
	src/cli/meggitt/MeggittMessages.h
	src/cli/meggitt/MeggittData.h
)

set(sumer-cli-files
	src/cli/sumer/Sumer.cpp
	src/cli/sumer/SumerData.cpp
	src/cli/sumer/SumerMessages.h
	src/cli/sumer/SumerData.h
)

add_library(${project_name} ${include_files} ${source_files})
target_include_directories(${project_name} PUBLIC include)

set_target_properties(${project_name} PROPERTIES LINK_FLAGS "/INCREMENTAL:NO")
set_target_properties(${project_name} PROPERTIES LINK_FLAGS "/ignore:4099")
set_target_properties(${project_name} PROPERTIES LINK_FLAGS "/ignore:2005")

file(GLOB_RECURSE lib_files "${PROJECT_BINARY_DIR}/lib/*")
install(TARGETS ${project_name} RUNTIME DESTINATION bin)
install(FILES ${lib_files} DESTINATION lib)

add_executable(${project_name}-meggitt-cli ${meggitt-cli-files})
target_link_libraries(${project_name}-meggitt-cli ${project_name})

add_executable(${project_name}-sumer-cli ${sumer-cli-files})
target_link_libraries(${project_name}-sumer-cli ${project_name})