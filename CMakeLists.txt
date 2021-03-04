cmake_minimum_required(VERSION 3.16)

message("-- CMAKE_SYSTEM_INFO_FILE:" ${CMAKE_SYSTEM_INFO_FILE})
message("-- CMAKE_SYSTEM_NAME:     " ${CMAKE_SYSTEM_NAME})
message("-- CMAKE_SYSTEM_PROCESSOR:" ${CMAKE_SYSTEM_PROCESSOR})
message("-- CMAKE_SYSTEM:          " ${CMAKE_SYSTEM})
message("-- CMAKE_HOST_SYSTEM:     " ${CMAKE_HOST_SYSTEM})
message("-- CCMAKE_HOST_SYSTEM_PROCESSOR:" ${CMAKE_HOST_SYSTEM_PROCESSOR})
message("-- CMAKE_GENERATOR:       " ${CMAKE_GENERATOR})
message("-- CMAKE_GENERATOR_INSTANCE:" ${CMAKE_GENERATOR_INSTANCE})
message("-- CMAKE_GENERATOR_PLATFORM:" ${CMAKE_GENERATOR_PLATFORM})

project(ShitRenderer)
find_package(GLEW CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (MSVC)
    # warning level 4 and all warnings as errors
    #how to deal with these warns??
    #add_compile_options(/W4 /WX /wd4201 /wd4996 /wd4100 /wd4189 /wd4819) 
    #add_compile_options(/Wall /wd4514 /wd4625 /wd4626 /wd4668 /wd5045)
	add_compile_options(/W4)

else()
    # lots of warnings and all warnings as errors
#    add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS on)

set(GLRENDERER on CACHE BOOL "gl")
set(VKRENDERER on CACHE BOOL "vukan")

set(SHIT_OUTPUT_DIR ${CMAKE_BINARY_DIR}/bin)

foreach(VAR ${CMAKE_CONFIGURATION_TYPES})
string(TOUPPER ${VAR} VAR2)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${VAR2} ${SHIT_OUTPUT_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${VAR2} ${SHIT_OUTPUT_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${VAR2} ${SHIT_OUTPUT_DIR})
endforeach()
set(CMAKE_DEBUG_POSTFIX d)

configure_file(renderer/config.h.in ${CMAKE_BINARY_DIR}/config.h)

#========================
#add GL lib
if(GLRENDERER)
file(GLOB glfile 
renderer/GL/*.h 
renderer/GL/*.cpp 
#renderer/GL/Platform/${CMAKE_HOST_SYSTEM_NAME}/*
)
add_library(GLRenderer SHARED ${glfile} )
target_link_libraries(GLRenderer PRIVATE
GLEW::GLEW
Opengl32.lib
)
target_include_directories(GLRenderer PRIVATE	
${CMAKE_SOURCE_DIR}/
${CMAKE_SOURCE_DIR}/renderer/GL
${CMAKE_BINARY_DIR} 
)
set_target_properties(GLRenderer PROPERTIES
	COMPILE_FLAGS -DSHIT_DLL
)
endif()

#=====================================
#add Vulkan lib
if(VKRENDERER)

set(VULKAN_INCLUDE_DIR $ENV{VULKAN_SDK}/include)
if(MSVC)
if(${CMAKE_VS_PLATFORM_NAME} STREQUAL "x64")
    set(VULKAN_LIB_DIR $ENV{VULKAN_SDK}/lib)
else()
    set(VULKAN_LIB_DIR  $ENV{VULKAN_SDK}/lib32)
endif()
endif()

file(GLOB vkfile 
renderer/Vulkan/*.h 
renderer/Vulkan/*.cpp 
#renderer/Vulkan/Platform/${CMAKE_HOST_SYSTEM_NAME}/*
)
add_library(VKRenderer SHARED ${vkfile} )
target_link_libraries(VKRenderer
${VULKAN_LIB_DIR}/vulkan-1.lib
)
target_include_directories(VKRenderer PRIVATE	
${CMAKE_SOURCE_DIR}/
${CMAKE_SOURCE_DIR}/renderer/Vulkan
${CMAKE_BINARY_DIR} 
${VULKAN_INCLUDE_DIR}
)
set_target_properties(VKRenderer PROPERTIES
	COMPILE_FLAGS -DSHIT_DLL
)
endif()


#===========================
#add shit renderer lib
file(GLOB rendererfile 
renderer/*.h 
renderer/sources/*.cpp 
renderer/Platform/${CMAKE_HOST_SYSTEM_NAME}/*
)
add_library(ShitRenderer ${rendererfile} )

target_include_directories(ShitRenderer PRIVATE
${CMAKE_SOURCE_DIR}/renderer
${CMAKE_BINARY_DIR} 
)
add_dependencies(ShitRenderer GLRenderer VKRenderer)

#===================
#add examples
function(newexample examplename)
message(${examplename})

add_executable(${examplename} examples/${examplename}.cpp)

#set_target_properties(${examplename} PROPERTIES FOLDER examples)

target_include_directories(${examplename} PRIVATE 
${CMAKE_SOURCE_DIR}
${CMAKE_SOURCE_DIR}/examples
${CMAKE_BINARY_DIR} 
)
target_link_libraries(${examplename} PRIVATE
${SHIT_OUTPUT_DIR}/ShitRenderer$<$<CONFIG:Debug>:d>.lib
glm
GLEW::GLEW
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






