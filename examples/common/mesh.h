#pragma once
#include "boundingVolume.h"
#include "common.h"
#include "component.h"

// struct Vertex
//{
//	float position[3];
//	float normal[3];
//	float tangent[4];
//	float texCoord[2];
//	float color[4];
//	float joints[4];
//	float weights[4];
//	static std::vector<Shit::VertexAttributeDescription>
//	getVertexAttributeDescription(uint32_t startLocation = 0, uint32_t
// binding = 0)
//	{
//		return std::vector<Shit::VertexAttributeDescription>{
//			{startLocation, binding, Shit::Format::R32G32B32_SFLOAT,
// offsetof(Vertex, position)}, 			{startLocation + 1, binding,
// Shit::Format::R32G32B32_SFLOAT, offsetof(Vertex, normal)}, 			{startLocation + 2,
// binding, Shit::Format::R32G32B32A32_SFLOAT, offsetof(Vertex, tangent)},
//			{startLocation + 3, binding, Shit::Format::R32G32_SFLOAT,
// offsetof(Vertex, texCoord)}, 			{startLocation + 4, binding,
// Shit::Format::R32G32B32A32_SFLOAT, offsetof(Vertex, color)}, 			{startLocation +
// 5, binding, Shit::Format::R32G32B32A32_SFLOAT, offsetof(Vertex, joints)},
//			{startLocation + 6, binding,
// Shit::Format::R32G32B32A32_SFLOAT, offsetof(Vertex, weights)},
//		};
//	}
// };
// struct InstanceAttribute
//{
//	glm::vec4 colorFactor;
//	glm::mat4 matrix;
//	static std::vector<Shit::VertexAttributeDescription>
//	getVertexAttributeDescription(uint32_t startLocation, uint32_t binding)
//	{
//		return {
//			{startLocation + 0, binding,
// Shit::Format::R32G32B32A32_SFLOAT, 0}, 			{startLocation + 1, binding,
// Shit::Format::R32G32B32A32_SFLOAT, 16}, 			{startLocation + 2, binding,
// Shit::Format::R32G32B32A32_SFLOAT, 32}, 			{startLocation + 3, binding,
// Shit::Format::R32G32B32A32_SFLOAT, 48}, 			{startLocation + 4, binding,
// Shit::Format::R32G32B32A32_SFLOAT, 64},
//		};
//	}
// };
// struct PrimitiveView
//{
//	//uint64_t vertexBufferOffset{};
//	//int64_t indexBufferOffset{-1};
//	//uint32_t firstVertex;
//	//int32_t firstIndex;
//	int32_t material{-1};
//	//Shit::DrawIndirectInfo drawIndirectInfo;

//	std::variant<
//		Shit::DrawIndirectCommand,
//		Shit::DrawIndexedIndirectCommand>
//		drawCommand;
//};

// struct MeshView
//{
//	std::vector<PrimitiveView> primitiveViews;
// };
//=========================================================
class PrimitiveRenderer;
struct Primitive {
    Shit::PrimitiveTopology topology;
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texcoords;
    std::vector<float> distanceAlongNormal;

    std::vector<uint32_t> indices;
    std::vector<int> faceMaterialIds;

    size_t getFaceIndexOffset(size_t faceIndex);
};

class OctreeMesh;
struct Mesh {
    std::vector<std::unique_ptr<Primitive>> primitives;

    BoundingVolume boundingVolume{};

    std::unique_ptr<OctreeMesh> octreePoints;
    std::unique_ptr<OctreeMesh> octreeFaces;

    void calculateDistanceAlongNormal();
    void generateOctreePoints(int maxDepth = -1);
    void generateOctreeFaces(int maxDepth = -1);
};
