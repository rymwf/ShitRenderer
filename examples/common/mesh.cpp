#include "mesh.h"

#include "octree.h"
#include "renderer.h"
// return u v t
// std::optional<glm::vec3> rayTriIntesect(
//	glm::vec3 const &o,
//	glm::vec3 const &d,
//	glm::vec3 const &p0,
//	glm::vec3 const &p1,
//	glm::vec3 const &p2)
//{
//	auto e1=p1-p0;

//}

std::optional<float> intersectTriangle(glm::vec3 const &p, glm::vec3 const &dir, glm::vec3 const *pVertices) {
    auto v0 = pVertices[1] - pVertices[0];
    auto v1 = pVertices[2] - pVertices[0];

    auto m = glm::mat3(v0, v1, -dir);
    if (glm::determinant(m) == 0) return {};
    auto k = glm::inverse(m) * (p - pVertices[0]);
    if (k.x + k.y > 1 || k.z < 0) return {};
    return k.z;
}
size_t Primitive::getFaceIndexOffset(size_t faceIndex) { return getTopologyFaceStep(topology) * faceIndex; }
void Mesh::calculateDistanceAlongNormal() {
    static const float dt = 0.01;
    for (auto &&e : primitives) {
        auto l = e->positions.size();
        e->distanceAlongNormal.resize(l, 0);

        for (size_t i = 0; i < l; ++i) {
            e->distanceAlongNormal[i] =
                octreeFaces->rayIntersect(e->positions[i] - glm::vec3(dt) * e->normals[i], -e->normals[i]) + dt;
        }
    }
}
void Mesh::generateOctreePoints(int maxDepth) {
    if (maxDepth < -1)
        return;
    else if (maxDepth == -1)
        octreePoints = std::make_unique<OctreeMesh>(this, true);
    else
        octreePoints = std::make_unique<OctreeMesh>(this, true, maxDepth);
}
void Mesh::generateOctreeFaces(int maxDepth) {
    if (maxDepth < -1)
        return;
    else if (maxDepth == -1)
        octreeFaces = std::make_unique<OctreeMesh>(this, false);
    else
        octreeFaces = std::make_unique<OctreeMesh>(this, false, maxDepth);
}