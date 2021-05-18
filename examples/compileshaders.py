import os
import sys
import getopt
import argparse


parser = argparse.ArgumentParser("call glslc to compile glsl to spirv")
parser.add_argument(
    '-T', help='target environment, values are {vulkan,vulkan1.1,vulkan1.2,opengl}, default is opengl', default="opengl")
parser.add_argument(
	'-S', help='source files, if no files provided, than all shaders in shader path will be compiled', nargs="*", default=None)

curdir = os.path.dirname(__file__)

PATH_SHADER = curdir+"/assets/shaders"
PATH_SHADER_COMPILE_DST = curdir + "/runtime/shaders"


def compileShader(srcfileFullName, target):
	dstPath = PATH_SHADER_COMPILE_DST
	if target == 'opengl':
		dstPath += "/GL"
	else:
		dstPath += "/Vulkan"
	if not os.path.exists(dstPath):
		os.mkdir(dstPath)
	os.system('cmd /c "glslangValidator -Os' + " -o " + dstPath+"/" +
	          os.path.basename(srcfileFullName)+".spv" + '" --target-env '+target + " " + srcfileFullName)

def processSrcFiles(filelist, target):
	subpath = "glsl/"
	if filelist == None:
		shaders = os.listdir(PATH_SHADER+"/"+subpath)
		for f in shaders:
			compileShader(PATH_SHADER+"/" +subpath+ f, target)
	else:
		for f in filelist:
			a = PATH_SHADER+"/"+subpath+os.path.basename(f)
			compileShader(a, target)


if __name__ == "__main__":
	args = parser.parse_args()
	processSrcFiles(args.S, args.T)
