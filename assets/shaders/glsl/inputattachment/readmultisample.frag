#version 460

layout (binding = 2) uniform UBO {
	vec2 range;
	int attachmentIndex;
	int sampleCount;
} ubo;
layout (location = 0) out vec4 outColor;

#ifdef VULKAN
layout (input_attachment_index = 0, binding = 0) uniform subpassInputMS inputColor;
layout (input_attachment_index = 1, binding = 1) uniform subpassInputMS inputDepth;
void main() 
{
	//is there any method like texturesamples or imagesamples to get samplecount?
	// Apply brightness and contrast filer to color input
	if (ubo.attachmentIndex == 0) {
		// Read color from previous color input attachment
		vec3 color =vec3(0);
		for(int i=0;i<ubo.sampleCount;++i)
		{
			color +=subpassLoad(inputColor,i).rgb;
		}
		color/=ubo.sampleCount;
		outColor.rgb = color;
	}

	// Visualize depth input range
	if (ubo.attachmentIndex == 1) {
		// Read depth from previous depth input attachment
		float depth = subpassLoad(inputDepth,0).r;
		outColor.rgb = vec3((depth - ubo.range[0]) * 1.0 / (ubo.range[1] - ubo.range[0]));
	}
}
#else

layout (binding = 0) uniform sampler2DMS inputColor;
layout (binding = 1) uniform sampler2DMS inputDepth;

void main() 
{
	//can use texturesamples to get sample count
	ivec2 uv=ivec2(gl_FragCoord.xy);
	// Apply brightness and contrast filer to color input
	if (ubo.attachmentIndex == 0) {
		// Read color from previous color input attachment
		vec3 color =vec3(0);
		for(int i=0;i<ubo.sampleCount;++i)
		{
			color += texelFetch(inputColor,uv,i).rgb;
		}
		color/=ubo.sampleCount;
		outColor.rgb = color;
	}

	// Visualize depth input range
	if (ubo.attachmentIndex == 1) {
		// Read depth from previous depth input attachment
		float depth = texelFetch(inputDepth,uv,0).r;
		outColor.rgb = vec3((depth - ubo.range[0]) * 1.0 / (ubo.range[1] - ubo.range[0]));
	}
}


#endif