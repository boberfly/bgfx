###### SHADER COMPILER ######
cmake_minimum_required(VERSION 3.1)

# Add shaderc and fcpp source files
file(GLOB_RECURSE SOURCES shaderc/*.cpp shaderc/*.h)

file(GLOB_RECURSE fcpp_sources ${CMAKE_SOURCE_DIR}/3rdparty/fcpp/cpp*.c)


# Add GLSL Optimizer source files
set(GLSL_HOME ${CMAKE_SOURCE_DIR}/3rdparty/glsl-optimizer)
file(GLOB glsl_sources
        ${GLSL_HOME}/src/glsl/glcpp/*.c
        ${GLSL_HOME}/src/glsl/*.cpp
        ${GLSL_HOME}/src/glsl/*.c
        ${GLSL_HOME}/src/mesa/*.c
        ${GLSL_HOME}/src/mesa/main/*.c
        ${GLSL_HOME}/src/mesa/program/*.c
        ${GLSL_HOME}/src/util/*.c		
    )
file(GLOB glsl_sources_remove
	${GLSL_HOME}/src/glsl/glcpp/glcpp.cpp
	${GLSL_HOME}/src/glsl/ir_set_program_inouts.cpp
	${GLSL_HOME}/src/glsl/main.cpp
	${GLSL_HOME}/src/glsl/builtin_stubs.cpp
)
list(REMOVE_ITEM glsl_sources ${glsl_sources_remove})

source_group("shaderc" FILES ${SOURCES})
source_group("fcpp" FILES ${fcpp_sources})
source_group("glsl-optimizer" FILES ${glsl_sources})


add_executable (
   shaderc
   ${SOURCES}   
   ${fcpp_sources}
   ${glsl_sources}
)	

target_include_directories(
	shaderc
	
	PRIVATE ${GLSL_HOME}/include
	PRIVATE ${GLSL_HOME}/src/mesa
	PRIVATE ${GLSL_HOME}/src/mapi
	PRIVATE ${GLSL_HOME}/src/glsl
	PRIVATE ${GLSL_HOME}/src
	PRIVATE ${CMAKE_SOURCE_DIR}/3rdparty/fcpp		
	PRIVATE ${CMAKE_SOURCE_DIR}/3rdparty/glsl-optimizer/src/glsl
	#PRIVATE  ${CMAKE_SOURCE_DIR}/3rdparty/glsl-optimizer/include/c99			
)

target_compile_definitions(
	shaderc 
	PRIVATE -DNINCLUDE=64 -DNWORK=65536 -DNBUFF=65536 -DOLD_PREPROCESSOR=0
)

target_link_libraries(shaderc bgfx)

if(CMAKE_HOST_WINDOWS)
	target_compile_definitions(
		shaderc 
		PRIVATE -D__STDC__ -D__STDC_VERSION__=199901L -Dstrdup=_strdup -Dalloca=_alloca -Disascii=__isascii
	)        
		
	target_include_directories(
		shaderc
		PRIVATE $ENV{DXSDK_DIR}include
	)
	
	target_link_libraries(shaderc d3dx9.lib)
	target_link_libraries(shaderc d3dcompiler.lib)
	target_link_libraries(shaderc dxguid.lib)
	set(DXSDK_LIB "$ENV{DXSDK_DIR}Lib\\x64\\")		
	set_target_properties(shaderc PROPERTIES LINK_FLAGS "/LIBPATH:\"${DXSDK_LIB}\\\"")
	set_target_properties(shaderc PROPERTIES COMPILE_FLAGS "/wd4291 /WX-")
endif()


set_target_properties(shaderc PROPERTIES FOLDER "Tools")

