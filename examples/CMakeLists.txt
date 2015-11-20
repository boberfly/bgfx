###### EXAMPLES ######
cmake_minimum_required(VERSION 3.1)

message("WINDOWS = ${CMAKE_HOST_WINDOWS}")
message("OSX = ${CMAKE_HOST_OSX}")
message("LINUX = ${CMAKE_HOST_LINUX}")

# Add source files for common example project 
file(GLOB_RECURSE SOURCES common/*.cpp common/*.h ../3rdparty/ib-compress/*.cpp ../3rdparty/ib-compress/*.h ../3rdparty/ocornut-imgui/*.cpp ../3rdparty/ocornut-imgui/*.h)
add_library (
   example-common  
   ${SOURCES}
)
set_target_properties(example-common PROPERTIES FOLDER "Examples")

if(${CMAKE_HOST_WINDOWS})
	message("Compiling shaders for dx11")
	set(SHADERC_PLATFORM windows)
	set(SHADERC_VERTEX_PROFILE vs_4_0)
	set(SHADERC_FRAGMENT_PROFILE ps_4_0)
	set(SHADERC_COMPUTE_PROFILE cs_5_0)
	set(SHADERC_HULL_PROFILE hs_5_0)
	set(SHADERC_DOMAIN_PROFILE ds_5_0)
	set(SHADER_PATH shaders/dx11)
	
	#message("Compiling shaders for glsl")
	#set(SHADERC_PLATFORM linux)
	#set(SHADERC_VERTEX_PROFILE 120)
	#set(SHADERC_FRAGMENT_PROFILE 120)
	#set(SHADERC_COMPUTE_PROFILE 430)
	#set(SHADERC_HULL_PROFILE 430)
	#set(SHADERC_DOMAIN_PROFILE 430)
	#set(SHADER_PATH shaders/glsl)
	
elseif(${CMAKE_HOST_OSX})
	message("Compiling shaders for metal")

	set(SHADERC_PLATFORM osx)
	set(SHADERC_VERTEX_PROFILE metal)
	set(SHADERC_FRAGMENT_PROFILE metal)
	set(SHADERC_COMPUTE_PROFILE metal)
	set(SHADERC_HULL_PROFILE metal)
	set(SHADERC_DOMAIN_PROFILE metal)
	set(SHADER_PATH shaders/metal)

else(${CMAKE_HOST_LINUX})
	message("Compiling shaders for glsl")

	set(SHADERC_PLATFORM linux)
	set(SHADERC_VERTEX_PROFILE 120)
	set(SHADERC_FRAGMENT_PROFILE 120)
	set(SHADERC_COMPUTE_PROFILE 430)
	set(SHADERC_HULL_PROFILE 430)
	set(SHADERC_DOMAIN_PROFILE 430)

	set(SHADER_PATH shaders/glsl)
endif()

file(MAKE_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${SHADER_PATH})


# Macro for adding a custom compiler step for .sc files 
macro(execute_shader_compiler target shaderFile profile type platform)	
	get_filename_component(SHADER_FILE_NAME ${shaderFile} NAME_WE)
	add_custom_command(			
			OUTPUT "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${SHADER_PATH}/${SHADER_FILE_NAME}.bin"
			COMMAND "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaderc" 
				-i "${CMAKE_SOURCE_DIR}/src" 
				-f "${shaderFile}" 
				-o "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${SHADER_PATH}/${SHADER_FILE_NAME}.bin" 
				--profile ${profile} 
				--type ${type} 
				--platform ${platform} 
				-O 3
			MAIN_DEPENDENCY ${shaderFile}
			DEPENDS shaderc 
			COMMENT "Building shader ${SHADER_FILE_NAME}.sc"
		)
endmacro()

# Macro for adding an example project by just specifying the folder name 
macro(add_bgfx_example folderName) 
	message("folderName = ${folderName}")
	file(GLOB_RECURSE SOURCES ${folderName}/*.cpp ${folderName}/*.c ${folderName}/*.h ${folderName}/*.sc ${folderName}/*.sh)
	add_executable (
	   ${folderName}
	   ${SOURCES}
	)
	target_link_libraries(${folderName} example-common)
	target_link_libraries(${folderName} bgfx)
	set_target_properties(${folderName} PROPERTIES FOLDER "Examples")
	
	target_include_directories(
		${folderName}
		PRIVATE ${CMAKE_SOURCE_DIR}/examples/common
	)
	
	# Make depdent of shader compiler for letting the IDE or a makefile take care of compiling .sc files during build 
	add_dependencies(${folderName} shaderc)
	
	## VERTEX SHADERS 
	file(GLOB_RECURSE SHADER_SRC ${folderName}/vs_*.sc)	
	foreach(SHADER_FILE_FULL ${SHADER_SRC})
		execute_shader_compiler(${folderName} ${SHADER_FILE_FULL} ${SHADERC_VERTEX_PROFILE} vertex ${SHADERC_PLATFORM}) 		
	endforeach()
	
	## FRAGMENT SHADERS 
	file(GLOB_RECURSE SHADER_SRC ${folderName}/fs_*.sc)	
	foreach(SHADER_FILE_FULL ${SHADER_SRC})
		execute_shader_compiler(${folderName} ${SHADER_FILE_FULL} ${SHADERC_FRAGMENT_PROFILE} fragment ${SHADERC_PLATFORM}) 		
	endforeach()   

	## COMPUTE SHADERS 
	file(GLOB_RECURSE SHADER_SRC ${folderName}/cs_*.sc)	
	foreach(SHADER_FILE_FULL ${SHADER_SRC})
		execute_shader_compiler(${folderName} ${SHADER_FILE_FULL} ${SHADERC_COMPUTE_PROFILE} compute ${SHADERC_PLATFORM}) 		
	endforeach() 	
	
	## HULL SHADERS 
	file(GLOB_RECURSE SHADER_SRC ${folderName}/hs_*.sc)	
	foreach(SHADER_FILE_FULL ${SHADER_SRC})
		execute_shader_compiler(${folderName} ${SHADER_FILE_FULL} ${SHADERC_HULL_PROFILE} hull ${SHADERC_PLATFORM}) 		
	endforeach() 	
	
	## DOMAIN SHADERS 
	file(GLOB_RECURSE SHADER_SRC ${folderName}/ds_*.sc)	
	foreach(SHADER_FILE_FULL ${SHADER_SRC})
		execute_shader_compiler(${folderName} ${SHADER_FILE_FULL} ${SHADERC_DOMAIN_PROFILE} domain ${SHADERC_PLATFORM}) 		
	endforeach() 	
	
endmacro()


add_bgfx_example(00-helloworld)
add_bgfx_example(01-cubes)
add_bgfx_example(02-metaballs)
add_bgfx_example(03-raymarch)
add_bgfx_example(04-mesh)
add_bgfx_example(05-instancing)
add_bgfx_example(06-bump)
add_bgfx_example(07-callback)
add_bgfx_example(08-update)
add_bgfx_example(09-hdr)
add_bgfx_example(10-font)
add_bgfx_example(11-fontsdf)
add_bgfx_example(12-lod)
add_bgfx_example(13-stencil)
add_bgfx_example(14-shadowvolumes)
add_bgfx_example(15-shadowmaps-simple)
add_bgfx_example(16-shadowmaps)
add_bgfx_example(17-drawstress)
add_bgfx_example(18-ibl)
add_bgfx_example(19-oit)
add_bgfx_example(20-nanovg)
add_bgfx_example(21-deferred)
add_bgfx_example(22-windows)
add_bgfx_example(23-vectordisplay)
add_bgfx_example(24-nbody)
add_bgfx_example(25-c99)
add_bgfx_example(26-occlusion)
add_bgfx_example(27-tessellation)


