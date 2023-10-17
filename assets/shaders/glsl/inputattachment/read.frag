#version 460

layout (binding = 2) uniform UBO {
	vec2 range;
	int attachmentIndex;
} ubo;
layout (location = 0) out vec4 outColor;

#ifdef VULKAN
layout (input_attachment_index = 0, binding = 0) uniform subpassInput inputColor;
layout (input_attachment_index = 1, binding = 1) uniform subpassInput inputDepth;
void main() 
{
	// Apply brightness and contrast filer to color input
	if (ubo.attachmentIndex == 0) {
		// Read color from previous color input attachment
		outColor.rgb = subpassLoad(inputColor).rgb;
	}

	// Visualize depth input range
	if (ubo.attachmentIndex == 1) {
		// Read depth from previous depth input attachment
		float depth = subpassLoad(inputDepth).r;
		//outColor.rgb = vec3((depth - ubo.range[0]) * 1.0 / (ubo.range[1] - ubo.range[0]));
		outColor.rgb = vec3((depth - ubo.range[0]) * 1.0 / (ubo.range[1] - ubo.range[0]));
	}
}
#else

layout (binding = 0) uniform sampler2D inputColor;
layout (binding = 1) uniform sampler2D inputDepth;

void main() 
{
	ivec2 uv=ivec2(gl_FragCoord.xy);
	// Apply brightness and contrast filer to color input
	if (ubo.attachmentIndex == 0) {
		// Read color from previous color input attachment
		outColor.rgb = texelFetch(inputColor,uv,0).rgb;
	}

	// Visualize depth input range
	if (ubo.attachmentIndex == 1) {
		// Read depth from previous depth input attachment
		float depth = texelFetch(inputDepth,uv,0).r;
		outColor.rgb = vec3((depth - ubo.range[0]) * 1.0 / (ubo.range[1] - ubo.range[0]));
	}
}


#endif