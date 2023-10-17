
#===================
## create common library 
file(GLOB exampleCommonFiles
${CMAKE_SOURCE_DIR}/examples/imguiimpl/*.hpp
${CMAKE_SOURCE_DIR}/examples/imguiimpl/*.cpp
common/*.hpp
common/*.cpp
)
add_library(exampleCommon ${exampleCommonFiles})
set_target_properties(exampleCommon PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})

target_include_directories(exampleCommon PRIVATE 
${CMAKE_SOURCE_DIR}
${CMAKE_SOURCE_DIR}/examples/dependencies
${CMAKE_SOURCE_DIR}/examples/dependencies/imgui
${CMAKE_SOURCE_DIR}/examples/dependencies/stb
${CMAKE_SOURCE_DIR}/examples
${CMAKE_BINARY_DIR} 
${VULKAN_INCLUDE_DIR}
)

target_link_libraries(exampleCommon PRIVATE
ShitRenderer
${CMAKE_SOURCE_DIR}/examples/dependencies/lib/imgui.lib
${CMAKE_SOURCE_DIR}/examples/dependencies/lib/freetype.lib
)
target_link_libraries(exampleCommon PRIVATE
debug
${VULKAN_LIB_DIR}/shaderc_combinedd.lib
)
target_link_libraries(exampleCommon PRIVATE
optimized
${VULKAN_LIB_DIR}/shaderc_combined.lib
)

add_custom_command(TARGET exampleCommon POST_BUILD COMMAND ${CMAKE_COMMAND} -E
copy \"${CMAKE_SOURCE_DIR}/examples/dependencies/lib/brotlicommon.dll\"
\"${CMAKE_SOURCE_DIR}/examples/dependencies/lib/brotlidec.dll\"
\"${CMAKE_SOURCE_DIR}/examples/dependencies/lib/bz2.dll\"
\"${CMAKE_SOURCE_DIR}/examples/dependencies/lib/freetype.dll\"
\"${CMAKE_SOURCE_DIR}/examples/dependencies/lib/libpng16.dll\"
\"${CMAKE_SOURCE_DIR}/examples/dependencies/lib/zlib1.dll\"
\"${SHIT_OUTPUT_DIR}\\$<IF:$<CONFIG:Debug>,Debug,Release>\")

configure_file(common/exampleconfig.hpp.in ${CMAKE_BINARY_DIR}/exampleconfig.h)

#===================
#add examples
function(newexample examplename)
message(${examplename})

add_executable(${examplename} ${examplename}.cpp)

if(MSVC)
set_target_properties(${examplename} PROPERTIES FOLDER examples
DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX}
#COMPILE_FLAGS "/wd5054 /wd4100"
)

target_link_options(${examplename} PRIVATE	
	"/subsystem:windows"
)
endif()

target_include_directories(${examplename} PRIVATE 
${CMAKE_SOURCE_DIR}
${CMAKE_SOURCE_DIR}/examples/dependencies
${CMAKE_SOURCE_DIR}/examples/dependencies/stb
${CMAKE_SOURCE_DIR}/examples/dependencies/imgui
${CMAKE_BINARY_DIR} 
${IMGUI_INCLUDE_DIRS}
)

target_link_libraries(${examplename} PRIVATE
ShitRenderer
exampleCommon
${CMAKE_SOURCE_DIR}/examples/dependencies/lib/freetype.lib
${CMAKE_SOURCE_DIR}/examples/dependencies/lib/glfw3.lib
)

add_dependencies(${examplename} ShitRenderer exampleCommon)

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
#newexample(10-tessllation)

newexample(offscreen)
newexample(appbasetest)
newexample(loadmodelobj)
newexample(envBRDF)
newexample(imgui)
newexample(multiview)
newexample(multiviewport)
newexample(skybox)
newexample(prefilteredEnv)
newexample(pushconstant)
newexample(pbr)
newexample(arealight)
newexample(imageprocess)
newexample(inputattachment)
newexample(irradianceSH)
newexample(IBL)
newexample(shadowcascade)
newexample(shadowcube)
newexample(shadow)
newexample(fxaa)
newexample(ao)
newexample(deferred)
newexample(deferredmultisample)
newexample(bloom)
newexample(sheencloth)
#newexample(proceduralSky)
newexample(depthpeeling)
newexample(Abuffer)
newexample(kbuffers)
newexample(translucency)
newexample(octreetest)
newexample(SSLarge)
# newexample(cartoon)
#newexample(raytracing)
#newexample(skin)
# newexample(multiwindow)

newexample(renderInTriangle)
#########################




