# ShitRenderer 📝  
deprecated

ShitRenderer is vulkan-like graphic api rendering library, aiming to handle all graphics api through 
the same interface.

Supported graphic api:
* vulkan1.1
* opengl4.6

Supported platform :
* windows

## Get Started 🚀  
The interface of this library is almost the same as vulkan, it is easy for you to use this lib if you are
familiar with vulkan.

## build
use compileshaders.py to compile glsl to spirv
It can compile shader files in assets folder "assets/shaders/glsl" to spirv

usage: call glslc to compile glsl to spirv [-h] [-T T] [-S [S [S ...]]]

optional arguments:

  -h, --help      show this help message and exit

  -T T            target environment, values are {vulkan,vulkan1.1,vulkan1.2,opengl}, default is opengl

  -S [S [S ...]]  source files, if no files provided, than all shaders in shader path will be compiled

example

    py compileshaders.py -T vulkan1.1
    py compileshaders.py -T vulkan1.1 -S [folder or file in assets/shaders/glsl]

build project
    
    cmake -B build

## examples
Here are some examples, you can easily switch between vulkan and opengl by passing argument "GL" or "VK".
More render results can be found in folder ./screenshot

*	00-init.cpp
*	01-triangle.cpp
*	02-triangle-vbo.cpp
*	03-image.cpp
*	04-ubo.cpp
*	05-depth.cpp
*	06-mipmap.cpp
*	07-multisample.cpp
*	08-screenshot.cpp
*	Abuffer.cpp
*	IBL.cpp

![image](screenshot/20220918014301-0.jpg)

![image](screenshot/20220918014927-0.jpg)

*	SSLarge.cpp

![image](screenshot/20221115093958-0.jpg)

*	ao.cpp

![image](screenshot/20221010115239-0.jpg)

*	appbasetest.cpp
*	arealight.cpp

![image](screenshot/20220908013448-0.jpg)
![image](screenshot/20220908014324-0.jpg)
![image](screenshot/20220908010239-0.jpg)

*	bloom.cpp

![image](screenshot/20221011110433-0.jpg)

*	deferred.cpp
*	deferredmultisample.cpp

![image](screenshot/20220925214256-0.jpg)

*	depthpeeling.cpp
*	envBRDF.cpp
*	fxaa.cpp
*	imageprocess.cpp
*	imgui.cpp
*	inputattachment.cpp
*	irradianceSH.cpp
*	kbuffers.cpp
*	loadmodelobj.cpp
*	multiview.cpp
*	multiviewport.cpp
*	octreetest.cpp

![image](screenshot/20221109004339-0.jpg)

*	offscreen.cpp
*	pbr.cpp

![image](screenshot/20221101103934-0.jpg)
![image](screenshot/20221101104004-0.jpg)

*	prefilteredEnv.cpp
*	pushconstant.cpp
*	renderInTriangle.cpp

BSpline

![image](screenshot/20230106220145-0.jpg)

BoundingVolume

![image](screenshot/20230106223116-0.jpg)

glass

![image](screenshot/20230106222234-0.jpg)

*	shadow.cpp

![image](screenshot/20220928214932-0.jpg)

*	shadowcascade.cpp

![image](screenshot/20220928235554-0.jpg)

*	shadowcube.cpp

![image](screenshot/20221005003943-0.jpg)

*	sheencloth.cpp

![image](screenshot/20221101112739-0.jpg)

*	skybox.cpp
*	translucency.cpp

![image](screenshot/20221113034150-0.jpg)


