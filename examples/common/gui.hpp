#pragma once
#include "common.hpp"
#include "imgui-impl.hpp"
#include "resourcebase.hpp"
#include "ImFileDialog.h"
#include "scene.hpp"

class GUI
{
	Scene* _scene;
	CommandPool *_shortLiveCommandPool;
	std::vector<CommandBuffer *> _commandBuffers;	//secondary commandbuffers

	using SelectedObject_T = std::variant<
		Node *,
		std::unique_ptr<Material> *,
		std::unique_ptr<Model2> *>;

	SelectedObject_T _ppSelectedObject;

public:
	GUI(Scene *pScene);
	virtual ~GUI();

	void update(uint32_t imageIndex);

	void setSelectedObject(SelectedObject_T ppObject)
	{
		_ppSelectedObject= ppObject;
	}

	void showMenuBar();
	void showInspector();

	void showProjectWindow();
	void showNodeWindow();
	void showNodeWindowHelper(Node *node, int& index, uint64_t& selection_mask);

	void nodeAttribute(Node *pNode);

	void showMaterialWindow(std::unique_ptr<Material> *ppMaterial);
	void materialStandard(Material* pMaterial);
	void materialColor(Material* pMaterial);
	void materialSkyboxProcedural(Material* pMaterial);
	void materialSkyboxEquirectangular(Material* pMaterial);

	decltype(auto) getCommandBuffer(uint32_t imageIndex) const
	{
		return _commandBuffers[imageIndex];
	}
};
