#pragma once
#include "common.hpp"
#include "resourcebase.hpp"

enum MaterialType
{
	MATERIAL_CUSTOM,
	MATERIAL_STANDARD,
	MATERIAL_COLOR,
	MATERIAL_SKYBOX_PROCEDURAL,
	MATERIAL_SKYBOX_EQUIRECTANGULAR,
	MATERIAL_AXIS,
};
struct UBOMaterial
{
	alignas(MIN_UNIFORM_BUFFER_OFFSET_ALIGNMENT) float emissiveFactor[3]{};
	float alphaCutoff{0.5};
	alignas(16) float baseColorFactor[4]{1.f, 1.f, 1.f, 1.f};
	float metallic{1.};
	float roughness{0.};
	float normalTextureFactor{0};
};

struct Node;
struct ModelView;

struct Vertex
{
	float position[3];
	float normal[3];
	float tangent[4];
	float texCoord[2];
	float color[4];
	float joints[4];
	float weights[4];
	static VertexInputStateCreateInfo getVertexInputStateCreateInfo();
};


//standar material
struct Material : public ResourceBase
{
	MaterialType _materialType = MATERIAL_CUSTOM;

	std::string _vertShaderName = "color.vert.spv";
	std::string _fragShaderName = "color.frag.spv";

	PipelineLayout *_pipelineLayout;
	Pipeline *_pipeline;

	std::vector<uint32_t> _descriptorSetIDs;
	std::vector<DescriptorSet *> _descriptorSets; //bindings and descriptor set

	std::vector<Node*> _nodes; //nodes use this material

	Sampler* _linearSampler;

	Material();
	virtual ~Material()
	{
	}

	void bind(CommandBuffer *pCommandBuffer, uint32_t imageIndex);
};

struct MaterialStandard : Material
{
	Image *_whiteImage;
	ImageView *_whiteImageView;
	Image *_blackImage;
	ImageView *_blackImageView;

	Buffer* _materialBuffer;

	MaterialStandard();
};

struct MaterialColor : Material
{
	MaterialColor() 
	{
		_materialType = MATERIAL_COLOR;
		_vertShaderName = "color.vert.spv";
		_fragShaderName = "color.frag.spv";
	}
};

//skybox
struct MaterialSkybox : Material
{
	struct UBOSkybox
	{
		float tintColor[4]{0.5, 0.5, 0.5, 1.};
		float exposure{1.};
	} uboSkybox;

	Sampler *sampler;
	Buffer *uboBuffer;

	Image *image2D;
	ImageView *imageView2D;

	Image *imageCube;
	ImageView *imageViewCube;

	Image *irradianceImageCube;
	ImageView *irradianceImageViewCube;

	Image *reflectionEnvMap;
	ImageView *reflectionEnvMapView;

	MaterialSkybox();
	virtual ~MaterialSkybox();
	void setParameters(const std::array<float, 4> tintColor, float exposure);
};

struct MaterialSkyboxProcedural : MaterialSkybox
{
	bool hasSun{true};
	float sunSize{1.};
	float sunSizeConvergence{1.};
	float atmosphereThickness{1.};
	float groundColor[4]{0.5, 0.5, 0.5, 0.5};
	MaterialSkyboxProcedural();
	~MaterialSkyboxProcedural() override
	{
	}
	void setLightDirection(const std::array<float, 3> &lightDirection);
};

struct MaterialSkyboxEquirectangular : MaterialSkybox
{
	MaterialSkyboxEquirectangular(const char *pImagePath = nullptr);
	void setImage(const char *pImagePath);
	~MaterialSkyboxEquirectangular() override
	{
	}
};

struct MaterialAxis : Material
{
	MaterialAxis();
};