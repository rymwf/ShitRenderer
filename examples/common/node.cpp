#include "node.hpp"

Node::Node()
{
	_name="new node";
	_type = ResourceType::NODE;

	static bool flag = true;
	if (flag)
	{
		for (int i = 1; i < int(ModelType::NUM); ++i)
		{
			models[i] = std::make_unique<Model2>(static_cast<ModelType>(i));
		}
		flag = false;
	}
}
void Node::Update()
{
	globalMatrix = (pParent ? pParent->globalMatrix : glm::mat4(1)) *
				   glm::scale(glm::translate(glm::mat4(1), translation) * glm::mat4_cast(rotation), scale);
	updated = true;
	for (auto &&child : children)
		child->Update();
}
void Node::AddChild(Node *pNode)
{
	children.emplace_back(pNode);
	pNode->pParent = this;
}
void Node::RemoveChild(Node *pNode)
{
	auto it = std::find_if(children.begin(), children.end(), [pNode](auto &&e) {
		return e == pNode;
	});
	if (it != children.end())
		children.erase(it);
}
Camera::Camera(CameraType type,const char* pName):cameraType(type)
{
	_name=pName;
	nodeType = NodeType::CAMERA;
	eye.translation = glm::vec3(0, 0, 10);
	AddChild(&eye);
	switch (type)
	{
	case CameraType::PERSPECTIVE:
	default:
		frustumDescription = {0.1, 0, PerspectiveProjectionDescription{60, 1}};
		break;
	case CameraType::ORTHO:
		frustumDescription = {-100, 100, OrthogonalProjectionDescription{100, 100}};
		break;
	}
	UpdateFrustum();
	Update();
}
Camera::~Camera()
{
}
void Camera::UpdateFrustum()
{
	frustumUpdated = true;
	std::visit(
		[&](auto &&arg) {
			using T = std::decay_t<decltype(arg)>;
			if constexpr (std::is_same_v<T, PerspectiveProjectionDescription>)
			{
				if (frustumDescription.far == 0)
					frustumMatrix = glm::infinitePerspective(glm::radians(arg.fovy), arg.aspect, frustumDescription.near);
				else
					frustumMatrix = glm::perspective(glm::radians(arg.fovy), arg.aspect, frustumDescription.near, frustumDescription.far);
			}
			else if constexpr (std::is_same_v<T, OrthogonalProjectionDescription>)
			{
				frustumMatrix = glm::ortho(-arg.xmag / 2, arg.xmag / 2, -arg.ymag / 2, arg.ymag / 2, frustumDescription.near, frustumDescription.far);
			}
			frustumMatrix[1][1] *= -1;
		},
		frustumDescription.extraDesc);
}
glm::mat4 Camera::GetProjectionMatrix()
{
	return frustumMatrix * glm::mat4(glm::translate(glm::transpose(glm::mat4(glm::mat3(eye.globalMatrix))), -glm::vec3(eye.globalMatrix[3])));
}
Light::Light(LightType type, const char *pName) : lightType(type)
{
	nodeType = NodeType::LIGHT;
	_name = pName;
}
ModelView::ModelView(ModelType modelType, const char *pName)
{
	_name = pName;
	nodeType = NodeType::MODEL_VIEW;

	pModel = models[static_cast<size_t>(modelType)].get();
	pModel->addInstance(InstanceAttribute{glm::vec4(1), glm::mat4(1)});

	(*g_ppDefaultMaterial)->_nodes.emplace_back(this);
	pModel->useModelMaterial(false);
	ppMaterial = g_ppDefaultMaterial;
}