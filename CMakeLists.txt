cmake_minimum_required(VERSION 3.16)

project(ShitRenderer)
find_package(GLEW CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

set(CMAKE_CXX_STANDARD 20)
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
	add_compile_options(/W4)

else()
    # lots of warnings and all warnings as errors -Weffc++
	add_compile_options(
		-Wall 
		-Wextra 
		-pedantic 
		-Werror 
		-Wshadow 
		-ansi 
		-Wno-missing-field-initializers
		-Wnon-virtual-dtor 	
	)
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS on)

set(GLRENDERER on CACHE BOOL "gl")
set(VKRENDERER on CACHE BOOL "vukan")
set(EXAMPLES on CACHE BOOL "examples")

if(VKRENDERER)
set(SHIT_VK_VERSION_MAJOR 1 CACHE STRING 1)
set(SHIT_VK_VERSION_MINOR 1 CACHE STRING 1)
set(SHIT_VK_VERSION_PATCH 0 CACHE STRING 0)
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
set_target_properties(ShitRenderer PROPERTIES
	COMPILE_FLAGS "/WX"
)

#========================
#add GL lib
if(GLRENDERER)
file(GLOB glfile 
renderer/GL/*.hpp
renderer/GL/*.cpp 
#renderer/GL/Platform/${CMAKE_HOST_SYSTEM_NAME}/*
)
add_library(GLRenderer SHARED ${glfile} )
target_link_libraries(GLRenderer PRIVATE
GLEW::GLEW
#Opengl32.lib
ShitRenderer
)
target_include_directories(GLRenderer PRIVATE	
${CMAKE_SOURCE_DIR}/
${CMAKE_SOURCE_DIR}/renderer/GL
${CMAKE_BINARY_DIR}
)
set_target_properties(GLRenderer PROPERTIES
	COMPILE_FLAGS "-DSHIT_DLL /WX"
)
add_dependencies(GLRenderer ShitRenderer)

endif()

#=====================================
#add Vulkan lib
if(VKRENDERER)

set(VULKAN_INCLUDE_DIR $ENV{VULKAN_SDK}/include)
if(MSVC)
#TODO: x64 x86?
    set(VULKAN_LIB_DIR $ENV{VULKAN_SDK}/lib)
#    set(VULKAN_LIB_DIR  $ENV{VULKAN_SDK}/lib32)
endif()

file(GLOB vkfile 
renderer/Vulkan/*.hpp
renderer/Vulkan/*.cpp 
#renderer/Vulkan/Platform/${CMAKE_HOST_SYSTEM_NAME}/*
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
set_target_properties(VKRenderer PROPERTIES
	COMPILE_FLAGS "-DSHIT_DLL /WX"
)

add_dependencies(VKRenderer ShitRenderer)
endif()

#===================
if(EXAMPLES)
#find_package(Boost REQUIRED COMPONENTS program_options)
find_path(TINYGLTF_INCLUDE_DIRS "tiny_gltf.h")
find_package(RapidJSON CONFIG REQUIRED)
#add examples
function(newexample examplename)
message(${examplename})

add_executable(${examplename} examples/${examplename}.cpp examples/common.cpp examples/model.cpp)

set_target_properties(${examplename} PROPERTIES FOLDER examples
#DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX}
COMPILE_FLAGS "/wd5054"
)

target_include_directories(${examplename} PRIVATE 
${CMAKE_SOURCE_DIR}
${CMAKE_SOURCE_DIR}/examples
${CMAKE_BINARY_DIR} 
${TINYGLTF_INCLUDE_DIRS}
${RAPIDJSON_INCLUDE_DIRS}/rapidjson
)

if(MSVC)
target_link_libraries(${examplename} PRIVATE
glm
)
else()
target_link_libraries(${examplename} PRIVATE
glm::glm
)
endif()

target_link_libraries(${examplename} PRIVATE
GLEW::GLEW
ShitRenderer
#Boost::program_options
)


add_dependencies(${examplename} ShitRenderer)
#string(REGEX REPLACE "/" "\\\\" STR0 ${CMAKE_SOURCE_DIR}/3rdparty/assimp/build/bin/$(Configuration)/assimp-vc142-mt$<$<CONFIG:Debug>:d>.dll)
#string(REGEX REPLACE "/" "\\\\" STR1 ${SHIT_OUTPUT_DIR}/)
#add_custom_command(TARGET ${examplename} PRE_BUILD 
#COMMAND copy /Y ${STR0} ${STR1}) 
endfunction()

#======================
newexample(00-init)
newexample(01-triangle)
newexample(02-triangle-vbo)
newexample(03-image)
newexample(04-ubo)
newexample(05-depth)
newexample(06-mipmap)
newexample(07-multisample)
newexample(08-screenshot)
newexample(09-model-pbr)
newexample(10-tessllation)
endif()





