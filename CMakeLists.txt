cmake_minimum_required(VERSION 3.16)

project(ShitRenderer)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

message("-- CMAKE_SYSTEM_INFO_FILE:" ${CMAKE_SYSTEM_INFO_FILE})
message("-- CMAKE_SYSTEM_NAME:     " ${CMAKE_SYSTEM_NAME})
message("-- CMAKE_SYSTEM_PROCESSOR:" ${CMAKE_SYSTEM_PROCESSOR})
message("-- CMAKE_SYSTEM:          " ${CMAKE_SYSTEM})
message("-- CMAKE_HOST_SYSTEM:     " ${CMAKE_HOST_SYSTEM})
message("-- CCMAKE_HOST_SYSTEM_PROCESSOR:" ${CMAKE_HOST_SYSTEM_PROCESSOR})
message("-- CMAKE_GENERATOR:       " ${CMAKE_GENERATOR})
message("-- CMAKE_GENERATOR_INSTANCE:" ${CMAKE_GENERATOR_INSTANCE})
message("-- CMAKE_GENERATOR_PLATFORM:" ${CMAKE_GENERATOR_PLATFORM})

if (MSVC)
    # warning level 4 and all warnings as errors
    #how to deal with these warns??
    #add_compile_options(/W4 /WX /wd4201 /wd4996 /wd4100 /wd4189 /wd4819) 
    #add_compile_options(/Wall /wd4514 /wd4625 /wd4626 /wd4668 /wd5045)
	#add_compile_options(/W4)
else()
    # lots of warnings and all warnings as errors -Weffc++
	add_compile_options(
		-Wall 
		-Wextra 
		-pedantic 
		-Wshadow 
		-ansi 
		-Wno-missing-field-initializers
		-Wnon-virtual-dtor 	
		-Wno-cast-function-type
	)
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS on)

set(GLRENDERER on CACHE BOOL "gl")
set(VKRENDERER on CACHE BOOL "vukan")

set(EXAMPLES on CACHE BOOL "examples")
set(ENGINE on CACHE BOOL "engine")

set(X86 off CACHE BOOL "x86")

if(VKRENDERER)
set(SHIT_VK_VERSION_MAJOR 1)
set(SHIT_VK_VERSION_MINOR 3)
set(SHIT_VK_VERSION_PATCH 0)
endif()

set(SHIT_OUTPUT_DIR ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${SHIT_OUTPUT_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${SHIT_OUTPUT_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${SHIT_OUTPUT_DIR})

set(CMAKE_DEBUG_POSTFIX d)

configure_file(renderer/config.hpp.in ${CMAKE_BINARY_DIR}/config.hpp)

#===========================
#add shit renderer lib
file(GLOB rendererfile 
renderer/*.hpp
renderer/sources/*.cpp 
)
add_library(ShitRenderer ${rendererfile} )

target_include_directories(ShitRenderer PRIVATE
${CMAKE_SOURCE_DIR}
${CMAKE_BINARY_DIR}
${CMAKE_SOURCE_DIR}/renderer
)
if(MSVC)
set_target_properties(ShitRenderer PROPERTIES COMPILE_FLAGS "/W4 /WX"
)
else()
set_target_properties(ShitRenderer PROPERTIES
	COMPILE_FLAGS "-Werror "
)
endif()

#========================
#add GL lib
if(GLRENDERER)
file(GLOB glfile 
renderer/GL/*.cpp 
#renderer/GL/Platform/${CMAKE_HOST_SYSTEM_NAME}/*
)
add_library(GLRenderer SHARED ${glfile} )
target_link_libraries(GLRenderer PRIVATE
ShitRenderer
${CMAKE_SOURCE_DIR}/renderer/GL/lib/glew32.lib
opengl32.lib
)
target_include_directories(GLRenderer PRIVATE	
${CMAKE_SOURCE_DIR}/
${CMAKE_SOURCE_DIR}/renderer/GL
${CMAKE_SOURCE_DIR}/renderer
${CMAKE_BINARY_DIR}
)
if(MSVC)
set_target_properties(GLRenderer PROPERTIES
	COMPILE_FLAGS "-DSHIT_DLL /W4 /WX"
)
else()
set_target_properties(GLRenderer PROPERTIES
	COMPILE_FLAGS "-W4 -Werror "
)

endif()

add_custom_command(TARGET GLRenderer POST_BUILD COMMAND ${CMAKE_COMMAND} -E
copy \"${CMAKE_SOURCE_DIR}/renderer/GL/lib/glew32.dll\"
\"${SHIT_OUTPUT_DIR}/$<IF:$<CONFIG:Debug>,Debug,Release>\")

add_dependencies(GLRenderer ShitRenderer)

endif()

#=====================================
#add Vulkan lib
if(VKRENDERER)

set(VULKAN_INCLUDE_DIR $ENV{VULKAN_SDK}/include)
if(WIN32)
if(X86)
   set(VULKAN_LIB_DIR $ENV{VULKAN_SDK}/lib32)
else()
   set(VULKAN_LIB_DIR $ENV{VULKAN_SDK}/lib)
endif()
endif()

file(GLOB vkfile 
renderer/Vulkan/*.hpp
renderer/Vulkan/*.cpp 
)
add_library(VKRenderer SHARED ${vkfile} )
target_link_libraries(VKRenderer
${VULKAN_LIB_DIR}/vulkan-1.lib
ShitRenderer
)
target_include_directories(VKRenderer PRIVATE	
${CMAKE_SOURCE_DIR}/
${CMAKE_SOURCE_DIR}/renderer/Vulkan
${CMAKE_BINARY_DIR} 
${VULKAN_INCLUDE_DIR}
)
if(MSVC)
set_target_properties(VKRenderer PROPERTIES
	COMPILE_FLAGS "-DSHIT_DLL /W4 /WX"
)
else()
set_target_properties(VKRenderer PROPERTIES
	COMPILE_FLAGS "-W4 -Werror "
)
endif()

add_dependencies(VKRenderer ShitRenderer)
endif()

#===================
if(EXAMPLES)
add_subdirectory(examples)
endif()