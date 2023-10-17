import os
from posixpath import basename
import sys
import getopt
import argparse
import traceback

#
# arguments 
# '-T', 'target environment, values are {vulkan,vulkan1.1,vulkan1.2,opengl}, default is opengl', default="opengl")
#'-S', help='source files, if no files provided, than all shaders in shader path will be compiled', nargs="*", default=None)
# file should be shader file in PATH_SHADER
# example
# py compileshaders.py -T vulkan1.1 -S foldername/shadername
#

parser = argparse.ArgumentParser("call glslc to compile glsl to spirv")
parser.add_argument(
    '-T', help='target environment, values are {vulkan,vulkan1.1,vulkan1.2,opengl}, default is opengl', default="opengl")
parser.add_argument(
	'-S', help='source files, if no files provided, than all shaders in shader path will be compiled', nargs="*", default=None)

curdir = os.path.dirname(__file__)

PATH_SHADER = curdir+"/assets/shaders"
PATH_SHADER_COMPILE_DST = curdir + "/runtime/shaders"


def CompileShader(shader_file_fullpath, target):
	if not os.path.exists(shader_file_fullpath):
		print("========",shader_file_fullpath, " not exist")
		return
	shader_file_dir = PATH_SHADER+"/glsl/"
	shader_file_relpath = os.path.relpath(shader_file_fullpath, shader_file_dir)
	shader_file_reldir=os.path.dirname(shader_file_relpath)

	dst_path = PATH_SHADER_COMPILE_DST
	if target == 'opengl':
		dst_path += "/GL"
	else:
		dst_path += "/Vulkan"
	dst_path+="/"+shader_file_reldir;
	if not os.path.exists(dst_path):
		os.makedirs(dst_path)
	dst_path += "/"+os.path.basename(shader_file_relpath) +".spv"
	os.system('cmd /c "glslangValidator ' + shader_file_fullpath+
	          " -o " + dst_path + '" --target-env '+target)


def ProcessShaderFolder(folder_name, target):
	for item in os.listdir(folder_name):
		if os.path.isdir(folder_name+"/"+item):
			ProcessShaderFolder(folder_name+"/"+item, target)
		else:
			CompileShader(folder_name+"/"+item, target)

def ProcessSrcFiles(filelist, target):
	mypath=PATH_SHADER+"/glsl"
	if filelist == None:
		ProcessShaderFolder(mypath,target)
	else:
		for f in filelist:
			a = mypath+"/"+f
			#print("----",a)
			if os.path.isdir(a):
				ProcessShaderFolder(a,target)
			else:
				CompileShader(a, target)


if __name__ == "__main__":
	try:
		#if not os.path.exists(curdir+"/runtime"):
		#	os.makedirs(curdir+"/runtime")
		if not os.path.exists(PATH_SHADER_COMPILE_DST):
			os.makedirs(PATH_SHADER_COMPILE_DST)
		args = parser.parse_args()
		ProcessSrcFiles(args.S, args.T)
	except Exception as e:
		print(e)
		traceback.print_exc()
