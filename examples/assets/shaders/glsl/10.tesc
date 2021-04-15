#version 460
layout(vertices=3) out;	//triangle
void main()
{
	//int a=gl_InvocationID%3;
	//float c=1./(gl_InvocationID/3+1);
	//gl_out[gl_InvocationID].gl_Position=gl_in[a].gl_Position*c ;
	gl_out[gl_InvocationID].gl_Position=gl_in[gl_InvocationID].gl_Position ;

	gl_TessLevelInner[0]=3;

	gl_TessLevelOuter[0]=4;
	gl_TessLevelOuter[1]=4;
	gl_TessLevelOuter[2]=4;
}