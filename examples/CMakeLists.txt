###### EXAMPLES ######
cmake_minimum_required(VERSION 3.1)

# Add source files for common example project 
file(GLOB_RECURSE SOURCES common/*.cpp common/*.h ../3rdparty/ib-compress/*.cpp ../3rdparty/ib-compress/*.h ../3rdparty/ocornut-imgui/*.cpp ../3rdparty/ocornut-imgui/*.h)
add_library (
   example-common  
   ${SOURCES}
)
set_target_properties(example-common PROPERTIES FOLDER "Examples")

# Macro for adding a custom compiler step for .sc files 
macro(execute_shader_compiler target shaderFile profile type platform)	
	get_filename_component(SHADER_FILE_NAME ${shaderFile} NAME_WE)
	add_custom_command(			
			OUTPUT "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders/dx11/${SHADER_FILE_NAME}.bin"
			COMMAND "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaderc" 
				-i "${CMAKE_SOURCE_DIR}/src" 
				-f "${shaderFile}" 
				-o "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders/dx11/${SHADER_FILE_NAME}.bin" 
				--profile ${profile} 
				--type ${type} 
				--platform ${platform} 
				-O 3
			MAIN_DEPENDENCY ${shaderFile}
			COMMENT "Building shader ${SHADER_FILE_NAME}.sc"
		)
endmacro()

# Macro for adding an example project by just specifying the folder name 
macro(add_bgfx_example folderName) 
	message("folderName = ${folderName}")
	file(GLOB_RECURSE SOURCES ${folderName}/*.cpp ${folderName}/*.c ${folderName}/*.h ${folderName}/*.sc)
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
		execute_shader_compiler(${folderName} ${SHADER_FILE_FULL} vs_4_0 vertex windows) 		
	endforeach()
	
	## FRAGMENT SHADERS 
	file(GLOB_RECURSE SHADER_SRC ${folderName}/fs_*.sc)	
	foreach(SHADER_FILE_FULL ${SHADER_SRC})
		execute_shader_compiler(${folderName} ${SHADER_FILE_FULL} ps_4_0 fragment windows) 		
	endforeach()   

	## COMPUTE SHADERS 
	file(GLOB_RECURSE SHADER_SRC ${folderName}/cs_*.sc)	
	foreach(SHADER_FILE_FULL ${SHADER_SRC})
		execute_shader_compiler(${folderName} ${SHADER_FILE_FULL} cs_5_0 compute windows) 		
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

