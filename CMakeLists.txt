###### BGFX Engine ######
cmake_minimum_required(VERSION 3.1)

project (bgfx)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY  				"${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG  			"${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE  		"${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO  	"${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL  		"${CMAKE_SOURCE_DIR}/bin")


# set host system variables
set (CMAKE_HOST_WINDOWS 0)
set (CMAKE_HOST_OSX 0)
set (CMAKE_HOST_LINUX 0)
if (${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Windows")
	set(CMAKE_HOST_WINDOWS 1)
	message("Host system: Windows")
elseif (${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Darwin")
	set(CMAKE_HOST_OSX 1)
	message("Host system: OSX")
elseif (${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Linux")
	set(CMAKE_HOST_LINUX 1)
	message("Host system: Linux")
else()
	message(WARNING "Host system not recognized, setting to 'Linux'")
	set(CMAKE_HOST_LINUX 1)
endif()

# detect compiler    
if (${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
	set(CMAKE_COMPILER_CLANG 1)
	message("Detected C++ Compiler: Clang (CMAKE_COMPILER_CLANG)")
elseif (${CMAKE_CXX_COMPILER_ID} MATCHES "GNU")
	set(CMAKE_COMPILER_GCC 1)
	message("Detected C++ Compiler: GCC (CMAKE_COMPILER_GCC)")
elseif (MSVC)
	set(CMAKE_COMPILER_MSVC 1)
	message("Detected C++ Compiler: VStudio (CMAKE_COMPILER_MSVC)")
else()
	message("Detected C++ Compiler: Unknown")
endif()


# TODO: Make platform dependent
if(${CMAKE_COMPILER_MSVC})
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
	add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
elseif(${CMAKE_COMPILER_CLANG})
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
endif()


add_definitions(-D__STDC_LIMIT_MACROS)
add_definitions(-D__STDC_FORMAT_MACROS)
add_definitions(-D__STDC_CONSTANT_MACROS)

set_property(GLOBAL PROPERTY USE_FOLDERS ON) 

if(${CMAKE_HOST_WINDOWS})
	include_directories(${CMAKE_SOURCE_DIR}/../bx/include/compat/msvc)
elseif(${CMAKE_HOST_OSX})
	include_directories(${CMAKE_SOURCE_DIR}/../bx/include/compat/osx)
endif()

# Global include directories
include_directories(
	${CMAKE_SOURCE_DIR}/../bx/include	
	${CMAKE_SOURCE_DIR}/include
	${CMAKE_SOURCE_DIR}/3rdparty
	${CMAKE_SOURCE_DIR}/3rdparty/khronos	
)

# Engine sources 
file(GLOB_RECURSE SOURCES src/*.cpp src/*.h include/*.h)
file(GLOB to_remove src/amalgamated.*)
list(REMOVE_ITEM SOURCES ${to_remove})

# Engine library 
add_library (
   bgfx   
   ${SOURCES}
)

if(${CMAKE_HOST_LINUX})
	find_package(OpenGL)	
	
	target_link_libraries(
		bgfx
		PRIVATE X11 Xrandr Xi Xinerama Xxf86vm Xcursor GL dl
		PRIVATE pthread
		PRIVATE ${OPENGL_LIBRARIES}
	)
	
	target_include_directories(
		bgfx
		PRIVATE ${OPENGL_INCLUDE_DIR}
		
	)
endif()

target_include_directories(
	bgfx
	PRIVATE ${CMAKE_SOURCE_DIR}/3rdparty/dxsdk/include
)

#if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    target_compile_definitions(bgfx PRIVATE "BGFX_CONFIG_DEBUG=1")
#endif()

# TODO: Is this needed? 
#target_compile_definitions(bgfx PRIVATE "BX_CONFIG_ENABLE_MSVC_LEVEL4_WARNINGS")

add_subdirectory(examples)
add_subdirectory(tools)
