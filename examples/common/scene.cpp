#include "scene.hpp"
#include "appbase.hpp"

glm::vec3 ambientColor = glm::vec3(0.2);

std::unique_ptr<Material> *g_ppDefaultMaterial;

Scene::Scene() : _name("new scene")
{
	auto graphicsQueueFamilyIndex = g_App->getDevice()->GetQueueFamilyIndexByFlag(QueueFlagBits::GRAPHICS_BIT, {});

	_defaultCommandPool = g_App->getDevice()->Create(CommandPoolCreateInfo{{}, graphicsQueueFamilyIndex->index});
	_defaultResetableCommandPool = g_App->getDevice()->Create(CommandPoolCreateInfo{CommandPoolCreateFlagBits::RESET_COMMAND_BUFFER_BIT, graphicsQueueFamilyIndex->index});

	//create secondary command buffers
	_defaultResetableCommandPool->CreateCommandBuffers(
		CommandBufferCreateInfo{
			CommandBufferLevel::SECONDARY,
			g_App->getImageCount()
			},
		_commandBuffers);
	//

	//create default descriptors
	std::vector<DescriptorSetLayoutBinding> PVBindings{
		DescriptorSetLayoutBinding{UNIFORM_BINDING_FRAME, DescriptorType::UNIFORM_BUFFER, 1, ShaderStageFlagBits::VERTEX_BIT | ShaderStageFlagBits::FRAGMENT_BIT},
	};

	_defaultDescriptorSetLayouts = {g_App->getDevice()->Create(DescriptorSetLayoutCreateInfo{PVBindings})};

	std::vector<DescriptorPoolSize> poolSizes(PVBindings.size());
	std::vector<DescriptorSetLayout *> setLayouts(g_App->getImageCount(), _defaultDescriptorSetLayouts[0]);
	std::transform(PVBindings.begin(), PVBindings.end(), poolSizes.begin(), [](auto &&e) {
		return DescriptorPoolSize{e.descriptorType, e.descriptorCount};
	});
	_defaultDescriptorPool = g_App->getDevice()->Create(DescriptorPoolCreateInfo{
		g_App->getImageCount(),
		poolSizes});
	_defaultDescriptorPool->Allocate({setLayouts}, _defaultDescriptorSets);

	//create default ubo buffers
	BufferCreateInfo bufferCreateInfo{
		.size = sizeof(UBOFrame),
		.usage = BufferUsageFlagBits::UNIFORM_BUFFER_BIT,
		.memoryPropertyFlags = MemoryPropertyFlagBits::HOST_COHERENT_BIT | MemoryPropertyFlagBits::HOST_VISIBLE_BIT};
	_defaultUBOBuffers.resize(g_App->getImageCount());

	UBOFrame uboFrame{
		{},
		{},
		UBOLight{
			{},
			glm::vec4(1),
			glm::vec3(-1),
		},
		ambientColor};

	for (auto &&e : _defaultUBOBuffers)
		e = g_App->getDevice()->Create(bufferCreateInfo, &uboFrame);

	//write to desriptor sets
	std::vector<WriteDescriptorSet> writes(g_App->getImageCount());
	for (size_t i = 0; i < g_App->getImageCount(); ++i)
	{
		writes[i] = WriteDescriptorSet{
			_defaultDescriptorSets[i],
			UNIFORM_BINDING_FRAME,
			0,
			DescriptorType::UNIFORM_BUFFER,
			std::vector<DescriptorBufferInfo>{{_defaultUBOBuffers[i],
											   0,
											   sizeof(UBOFrame)}}};
	}
	g_App->getDevice()->UpdateDescriptorSets(writes, {});

	//create default pipelinelayout
	_defaultPipelineLayout = g_App->getDevice()->Create(PipelineLayoutCreateInfo{_defaultDescriptorSetLayouts});

	//===================================================
	//                                                   
	//===================================================
	//prepare default scene
	//create root node
	_nodes.emplace_back(std::make_unique<Node>());
	_rootNode = _nodes.back().get();
	_rootNode->_name = "root";

	//add default material
	g_ppDefaultMaterial = AddMaterial(MATERIAL_STANDARD);
	(*g_ppDefaultMaterial)->_name = "default";

	//add default axis
	auto axisView = CreateModelView(ModelType::AXIS, nullptr, "axis");
	auto axisMaterial = AddMaterial(MaterialType::MATERIAL_AXIS);
	(*axisMaterial)->_name = "axis";
	ApplyMaterialToNode(axisMaterial, axisView);

	//add default skybox
	auto skyboxView= CreateModelView(ModelType::SKYBOX,nullptr,"skybox");

	//create skybox material
	_skyboxMaterial = AddMaterial(MaterialType::MATERIAL_SKYBOX_PROCEDURAL);
	(*_skyboxMaterial)->_name = "skybox";
	ApplyMaterialToNode(_skyboxMaterial, skyboxView);

	//add default main camera
	_mainCamera = std::make_unique<Camera>(CameraType::PERSPECTIVE);
	uint32_t w, h;
	g_App->getWindow()->GetFramebufferSize(w, h);
	_mainCamera->frustumDescription.extraDesc = PerspectiveProjectionDescription{60., double(w) / h};
	_mainCamera->UpdateFrustum();
}
Scene::~Scene()
{
	for (auto &&e : _defaultUBOBuffers)
		g_App->getDevice()->Destroy(e);
	g_App->getDevice()->Destroy(_defaultPipelineLayout);
	for (auto &&e : _defaultDescriptorSetLayouts)
		g_App->getDevice()->Destroy(e);

	g_App->getDevice()->Destroy(_defaultCommandPool);
	g_App->getDevice()->Destroy(_defaultResetableCommandPool);
}
Node *Scene::CreateNode(Node *parent,const char* pName)
{
	_nodes.emplace_back(std::make_unique<Node>());
	auto ret=_nodes.back().get();
	if(parent)
		parent->AddChild(ret);
	else
		_rootNode->AddChild(ret);
	return ret;
}
Light *Scene::CreateLight(LightType type, Node *parent,const char* pName)
{
	_nodes.emplace_back(std::make_unique<Light>(type));
	auto ret = _nodes.back().get();
	if (parent)
		parent->AddChild(ret);
	else
		_rootNode->AddChild(ret);
	return static_cast<Light *>(ret);
}
Camera *Scene::CreateCamera(CameraType type, Node *parent,const char* pName)
{
	_nodes.emplace_back(std::make_unique<Camera>(type));
	auto ret = _nodes.back().get();
	if (parent)
		parent->AddChild(ret);
	else
		_rootNode->AddChild(ret);
	return static_cast<Camera *>(ret);
}
ModelView *Scene::CreateModelView(ModelType type, Node *parent,const char* pName)
{
	_nodes.emplace_back(std::make_unique<ModelView>(type,pName));
	auto ret = _nodes.back().get();
	if (parent)
		parent->AddChild(ret);
	else
		_rootNode->AddChild(ret);
	return static_cast<ModelView *>(ret);
}
void Scene::UpdateDefaultUBOBuffers(Camera *camera, uint32_t imageIndex)
{
	static UBOFrame uboFrame{};
	static std::vector<bool> flags(g_App->getImageCount(), true);
	if (camera->updated || camera->frustumUpdated)
	{
		uboFrame.PV = camera->GetProjectionMatrix();
		uboFrame.eyePosition = glm::vec3(camera->eye.globalMatrix[3]);
		flags.assign(g_App->getImageCount(), true);
		camera->updated = false;
		camera->frustumUpdated = false;
	}
	if (flags[imageIndex])
	{
		void *pData;
		auto size = sizeof(glm::mat4) + sizeof(glm::vec3);
		_defaultUBOBuffers[imageIndex]->MapMemory(0, size, &pData);
		memcpy(pData, &uboFrame, size);
		_defaultUBOBuffers[imageIndex]->UnMapMemory();
		flags[imageIndex] = false;
	}
}
void Scene::Update(uint32_t imageIndex, Framebuffer *pFramebuffer)
{
	UpdateDefaultUBOBuffers(_mainCamera.get(), imageIndex);
	_commandBuffers[imageIndex]->Begin(CommandBufferBeginInfo{
		CommandBufferUsageFlagBits::ONE_TIME_SUBMIT_BIT,
		CommandBufferInheritanceInfo{
			g_App->getDefaultRenderPass(),
			g_App->getDefaultSubpass(),
			pFramebuffer}});

	// Setup viewport:
	Viewport viewport{
		0, 0,
		(float)pFramebuffer->GetCreateInfoPtr()->extent.width, (float)pFramebuffer->GetCreateInfoPtr()->extent.height,
		0.f, 1.f};
	_commandBuffers[imageIndex]->SetViewport({SetViewPortInfo{
		0, 1, &viewport}});

	//set scissor
	Rect2D scissor{{}, {pFramebuffer->GetCreateInfoPtr()->extent.width, pFramebuffer->GetCreateInfoPtr()->extent.height}};
	_commandBuffers[imageIndex]->SetScissor({SetScissorInfo{
		0, 1, &scissor}});

	_commandBuffers[imageIndex]->BindDescriptorSets(
		BindDescriptorSetsInfo{
			PipelineBindPoint::GRAPHICS,
			_defaultPipelineLayout,
			DESCRIPTORSET_ID_FRAME,
			1,
			&_defaultDescriptorSets[imageIndex],
		});

	for (auto &&material : _materials)
	{
		material->bind(_commandBuffers[imageIndex], imageIndex);
		for (auto &&e : material->_nodes)
		{
			if (e->enable)
				e->pModel->draw(_commandBuffers[imageIndex], imageIndex);
		}
	}
	_commandBuffers[imageIndex]->End();
}

std::unique_ptr<Material>* Scene::AddMaterial(MaterialType type)
{
	switch (type)
	{
	case MATERIAL_CUSTOM:
	default:
		_materials.emplace_back(std::make_unique<Material>());
		break;
	case MATERIAL_STANDARD:
		_materials.emplace_back(std::make_unique<MaterialStandard>());
		break;
	case MATERIAL_COLOR:
		_materials.emplace_back(std::make_unique<MaterialColor>());
		break;
	case MATERIAL_SKYBOX_EQUIRECTANGULAR:
		_materials.emplace_back(std::make_unique<MaterialSkyboxEquirectangular>());
		break;
	case MATERIAL_SKYBOX_PROCEDURAL:
		_materials.emplace_back(std::make_unique<MaterialSkyboxProcedural>());
		break;
	case MATERIAL_AXIS:
		_materials.emplace_back(std::make_unique<MaterialAxis>());
		break;
	}
	return &_materials.back();
}
void Scene::ApplyMaterialToNode(std::unique_ptr<Material> *ppMaterial, Node *pNode)
{
	RemoveNodeMaterial(pNode);
	(*ppMaterial)->_nodes.emplace_back(pNode);
	pNode->ppMaterial = ppMaterial;
	if (pNode->pModel)
		pNode->pModel->useModelMaterial(false);
}
void Scene::RemoveNodeMaterial(Node *pNode)
{
	if (pNode->ppMaterial)
	{
		auto &&nodes = (*pNode->ppMaterial)->_nodes;
		auto &&it = std::find_if(nodes.begin(), nodes.end(), [pNode](auto &&e) { return e == pNode; });
		if (it != nodes.end())
			nodes.erase(it);
		pNode->ppMaterial = nullptr;
		if (pNode->pModel)
			pNode->pModel->useModelMaterial(true);
	}
}
void Scene::RemoveNode(Node *pNode)
{
	//for (auto &&e : pNode->children)
	//{
	//	RemoveNode(e);
	//}
	//for (auto it = nodes.begin(); it != nodes.end(); ++it)
	//{
	//	if (it->get() == pNode)
	//	{
	//		nodes.erase(it);
	//		break;
	//	}
	//}
}

Model2 *Scene::LoadModel(const char *pPath)
{
	//auto model = std::make_unique<Model>(pPath);
	//if (model->LoadSucceed())
	//{
	//	models.emplace_back(std::move(model));
	//	return models.back().get();
	//}
	//else
	//{
	//	LOG("model load failed");
	//	return nullptr;
	//}
	return nullptr;
}
void Scene::Destroy(Node *pNode)
{
	//for (auto &&e : pNode->children)
	//	Destroy(e);
	//nodes.erase(std::find_if(nodes.begin(), nodes.end(), [pNode](auto &&e) {
	//	return e.get() == pNode;
	//}));
}
void Scene::Destroy(Model2 *pModel)
{
	//models.erase(std::find_if(models.begin(), models.end(), [pModel](auto &&e) {
	//	return e.get() == pModel;
	//}));
}