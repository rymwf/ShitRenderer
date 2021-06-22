#include "gui.hpp"
#include "appbase.hpp"

GUI::GUI(Scene *pScene) : _scene(pScene)
{
	auto graphicsQueueFamilyIndex = g_App->getDevice()->GetQueueFamilyIndexByFlag(QueueFlagBits::GRAPHICS_BIT, {});
	_shortLiveCommandPool = g_App->getDevice()->Create(
		CommandPoolCreateInfo{CommandPoolCreateFlagBits::RESET_COMMAND_BUFFER_BIT, graphicsQueueFamilyIndex->index});

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	uint32_t windowWidth, windowHeight;
	g_App->getWindow()->GetFramebufferSize(windowWidth, windowHeight);
	io.DisplaySize = ImVec2{float(windowWidth), float(windowHeight)};

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	_shortLiveCommandPool->CreateCommandBuffers(
		CommandBufferCreateInfo{
			CommandBufferLevel::SECONDARY,
			static_cast<uint32_t>(g_App->getImageCount())},
		_commandBuffers);

	ImGui_ImplShitRenderer_InitInfo imguiInitInfo{
		g_RendererVersion,
		g_App->getDevice(),
		_commandBuffers,
		g_App->getImageCount(),
		SAMPLE_COUNT};
	ImGui_ImplShitRenderer_Init(&imguiInitInfo, g_App->getDefaultRenderPass(), g_App->getDefaultSubpass());

	executeOneTimeCommands(g_App->getDevice(), QueueFlagBits::TRANSFER_BIT, 0, [](CommandBuffer *commandBuffer) {
		ImGui_ImplShitRenderer_CreateFontsTexture(commandBuffer);
	});
	ImGui_ImplShitRenderer_DestroyFontUploadObjects();

	//init ImFileDialog
	//ifd::FileDialog::Instance().CreateTexture=[]();
}
GUI::~GUI()
{
	g_App->getDevice()->Destroy(_shortLiveCommandPool);
}
void GUI::update(uint32_t imageIndex)
{
	bool commandBufferNeedUpdated = false;
	ImGui_ImplShitRenderer_NewFrame();
	ImGui::NewFrame();
	//=======================================

	showMenuBar();
	showNodeWindow();
	showInspector();
	showProjectWindow();

	//=======================================
	ImGui::Render();

	ImGui_ImplShitRenderer_RecordCommandBuffer(imageIndex, g_App->getDefaultRenderPass(), g_App->getDefaultSubpass(), g_App->getFramebuffer(imageIndex));
}
void GUI::showMenuBar()
{
	static bool demoWindow = false;
	static bool metricsWindow = false;
	//main menu bar
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Exit"))
			{
				g_App->getWindow()->Close();
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Edit"))
		{
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("GameObject"))
		{
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Component"))
		{
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Window"))
		{
			if (ImGui::MenuItem("Demo Window"))
			{
				demoWindow = true;
			}
			if (ImGui::MenuItem("Metrics Window"))
			{
				metricsWindow = true;
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Help"))
		{
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}
	//==========================
	if (demoWindow)
		ImGui::ShowDemoWindow(&demoWindow);

	if (metricsWindow)
		ImGui::ShowMetricsWindow();
}
void GUI::showInspector()
{
	ImGui::Begin("inspector");
	std::visit(overloaded{
				   [&](Node *pNode) {
					   if (!pNode)
						   return;
					   ImGui::Text(pNode->_name.c_str());
					   ImGui::Checkbox("enable", &pNode->enable);

					   ImGui::Separator();
					   nodeAttribute(pNode);
					   ImGui::Separator();
					   showMaterialWindow(pNode->ppMaterial);
				   },
				   [&](std::unique_ptr<Material> *ppMaterial) {
					   showMaterialWindow(ppMaterial);
				   },
				   [&](std::unique_ptr<Model2> *ppModel) {
					   ImGui::Text((*ppModel)->_name.c_str());
				   },
			   },
			   _ppSelectedObject);

	ImGui::End();
}
void GUI::showMaterialWindow(std::unique_ptr<Material> *ppMaterial)
{
	ImGui::Text((*ppMaterial)->_name.c_str());

	static const char *items[] = {
		"Custom",
		"Standard",
		"Color",
		"Procedural",
		"Equirectangular",
		"Axis",
	};

	// Expose flags as checkbox for the demo
	static ImGuiComboFlags flags = 0;

	// Using the generic BeginCombo() API, you have full control over how to display the combo contents.
	// (your selection data could be an index, a pointer to the object, an id for the object, a flag intrusively
	// stored in the object itself, etc.)
	int item_current_idx = (*ppMaterial)->_materialType;
	const char *combo_label = items[item_current_idx]; // Label to preview before opening the combo (technically it could be anything)
	if (ImGui::BeginCombo("material type", combo_label, flags))
	{
		for (int n = 0; n < IM_ARRAYSIZE(items); n++)
		{
			const bool is_selected = (item_current_idx == n);
			if (ImGui::Selectable(items[n], is_selected))
			{
				if (item_current_idx != n)
				{
					g_App->getDevice()->WaitIdle();

					auto name = (*ppMaterial)->_name;
					auto nodes = (*ppMaterial)->_nodes;
					switch (n)
					{
					case MATERIAL_COLOR:
						ppMaterial->reset(new MaterialColor());
						break;
					case MATERIAL_STANDARD:
					default:
						ppMaterial->reset(new MaterialStandard());
						break;
					case MATERIAL_SKYBOX_PROCEDURAL:
						ppMaterial->reset(new MaterialSkyboxProcedural());
						break;
					case MATERIAL_SKYBOX_EQUIRECTANGULAR:
						//*_ppSelectedResource = std::make_unique<MaterialSkyboxEquirectangular>((*_pSelectedResource)->_name.c_str(), _appBase->device, _appBase->defaultRenderPass);
						static const char *equirectangularImagePath = IMAGE_PATH "Mt-Washington-Cave-Room_Ref.hdr";
						ppMaterial->reset(new MaterialSkyboxEquirectangular(equirectangularImagePath));
						break;
					}
					(*ppMaterial)->_name = name;
					(*ppMaterial)->_nodes = nodes;
					item_current_idx = n;
				}
			}
			// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
			if (is_selected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}

	ImGui::Separator();
	switch (item_current_idx)
	{
	case MATERIAL_COLOR:
		materialColor(ppMaterial->get());
		break;
	case MATERIAL_STANDARD:
		materialStandard(ppMaterial->get());
		break;
	case MATERIAL_SKYBOX_PROCEDURAL:
		materialSkyboxProcedural(ppMaterial->get());
		break;
	case MATERIAL_SKYBOX_EQUIRECTANGULAR:
		materialSkyboxEquirectangular(ppMaterial->get());
		break;
	default:
		break;
	}
}
void GUI::materialStandard(Material *pMaterial)
{
	if (!pMaterial)
		return;
	auto p = static_cast<MaterialStandard *>(pMaterial);

	static UBOMaterial uboMaterial{};

	auto a0 = ImGui::DragFloat3("emissiveFactor", uboMaterial.emissiveFactor, 0.01f, -10.f, 10.f);
	auto a1 = ImGui::DragFloat("alphaCutOff", &uboMaterial.alphaCutoff, 0.01f, 0.f, 1.f);
	auto a2 = ImGui::DragFloat4("baseColorFactor", uboMaterial.baseColorFactor, 0.01f, -10.f, 10.f);
	auto a3 = ImGui::DragFloat("metallic", &uboMaterial.metallic, 0.01f, 0.f, 1.f);
	auto a4 = ImGui::DragFloat("roughness", &uboMaterial.roughness, 0.01f, 0.f, 1.f);
	auto a5 = ImGui::DragFloat("normalScale", &uboMaterial.normalTextureFactor, 0.01f, -10.f, 10.f);
	if (a0 || a1 || a2 || a3 || a4 || a5)
	{
		void *data;
		p->_materialBuffer->MapMemory(0, sizeof(UBOMaterial), &data);
		UBOMaterial *p2 = reinterpret_cast<UBOMaterial *>(data);
		memcpy(data, &uboMaterial, sizeof(UBOMaterial));
		p->_materialBuffer->UnMapMemory();
	}
}
void GUI::materialSkyboxProcedural(Material *pMaterial)
{
	if (!pMaterial)
		return;
	auto p = static_cast<MaterialSkyboxProcedural *>(pMaterial);
	ImGui::Image((ImTextureID)(p->imageView2D), ImVec2(320, 150));
}
void GUI::materialSkyboxEquirectangular(Material *pMaterial)
{
	auto p = static_cast<MaterialSkyboxProcedural *>(pMaterial);

	static float exposure = 1.f, preExposure = 1.f;
	static std::array<float, 4> tintColor{1.f, 1.f, 1.f, 1.f};
	static std::array<float, 4> preTintColor = tintColor;

	ImGui::SliderFloat("slider float", &exposure, 0.0f, 5.0f, "exposure= %.3f");

	//tint color
	ImGuiColorEditFlags misc_flags =
		ImGuiColorEditFlags_NoDragDrop |
		ImGuiColorEditFlags_AlphaPreviewHalf |
		ImGuiColorEditFlags_NoOptions |
		ImGuiColorEditFlags_AlphaBar;

	ImGui::Text("TintColor:");
	ImGui::ColorEdit4("MyColor##3", tintColor.data(), ImGuiColorEditFlags_NoLabel | misc_flags);

	if (tintColor != preTintColor || exposure != preExposure)
	{
		static_cast<MaterialSkyboxEquirectangular *>(pMaterial)->setParameters(tintColor, exposure);
		preTintColor = tintColor;
		preExposure = exposure;
	}
	ImGui::Image((ImTextureID)(p->imageView2D), ImVec2(320, 150));
}
void GUI::showProjectWindow()
{
	ImGui::Begin("Project");

	if (ImGui::TreeNode("Models"))
	{
		static int selected = -1;
		int i = 0;
		for (auto it = _scene->_models.begin(), end = _scene->_models.end(); it != end; ++it, ++i)
		{
			if (ImGui::Selectable((*it)->_name.c_str(), selected == i))
			{
				selected = i;
				_ppSelectedObject = &(*it);
			}
		}
		ImGui::TreePop();
	}
	if (ImGui::TreeNode("Materials"))
	{
		static int selected = -1;
		int i = 0;
		for (auto it = _scene->_materials.begin(), end = _scene->_materials.end(); it != end; ++it, ++i)
		{
			if (ImGui::Selectable((*it)->_name.c_str(), selected == i))
				selected = i;
			if (ImGui::IsItemClicked())
				_ppSelectedObject = &(*it);
		}
		ImGui::TreePop();
	}
	//ImGui::Separator();

	ImGui::End();
}
void GUI::materialColor(Material *pMaterial)
{
}
void GUI::showNodeWindow()
{
	ImGui::Begin("Hierarchy");

	static uint64_t selection_mask = 0;
	int index = 0;
	for (auto &&e : _scene->_rootNode->children)
	{
		showNodeWindowHelper(e, index, selection_mask);
		++index;
	}
	ImGui::End();
}
void GUI::showNodeWindowHelper(Node *node, int &index, uint64_t &selection_mask)
{
	ImGuiTreeNodeFlags base_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
	static bool test_drag_and_drop = true;

	if (node->children.empty())
		base_flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

	const bool is_selected = (selection_mask & (1 << index)) != 0;
	if (is_selected)
	{
		base_flags |= ImGuiTreeNodeFlags_Selected;
	}

	bool node_open = ImGui::TreeNodeEx((void *)(intptr_t)index, base_flags, "%s", node->_name.c_str());
	// 'selection_mask' is dumb representation of what may be user-side selection state.
	//  You may retain selection state inside or outside your objects in whatever format you see fit.
	// 'node_clicked' is temporary storage of what node we have clicked to process selection at the end
	/// of the loop. May be a pointer to your own node type, etc.
	int node_clicked = -1;

	if (ImGui::IsItemClicked())
	{
		node_clicked = index;
		_ppSelectedObject = node;
	}

	if (test_drag_and_drop && ImGui::BeginDragDropSource())
	{
		ImGui::SetDragDropPayload("_TREENODE", NULL, 0);
		ImGui::Text("This is a drag and drop source");
		ImGui::EndDragDropSource();
	}

	if (node_clicked != -1)
	{
		// Update selection state
		// (process outside of tree loop to avoid visual inconsistencies during the clicking frame)
		if (ImGui::GetIO().KeyCtrl)
			selection_mask ^= (1 << node_clicked); // CTRL+click to toggle
		else									   //if (!(selection_mask & (1 << node_clicked))) // Depending on selection behavior you want, may want to preserve selection when clicking on item that is part of the selection
			selection_mask = (1 << node_clicked);  // Click to single-select
	}

	for (auto &&e : node->children)
		showNodeWindowHelper(e, ++index, selection_mask);

	if (node_open && !(base_flags & ImGuiTreeNodeFlags_Leaf))
		ImGui::TreePop();
}
void GUI::nodeAttribute(Node *pNode)
{
	ImGui::DragFloat3("position", reinterpret_cast<float *>(&pNode->translation), 0.1f);
	ImGui::DragFloat3("rotation", reinterpret_cast<float *>(&pNode->rotation), 0.1f);
	ImGui::DragFloat3("scale", reinterpret_cast<float *>(&pNode->scale), 0.1f);
	pNode->Update();
	auto instanceAttribute = pNode->pModel->getInstanceAttribute(pNode->instanceId);
	instanceAttribute.matrix = pNode->globalMatrix;
	for (uint32_t i = 0; i < g_App->getImageCount(); ++i)
		pNode->pModel->updateInstanceAttributes(i, pNode->instanceId, instanceAttribute);
}