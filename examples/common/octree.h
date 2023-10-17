#pragma once
#include "mesh.h"
#include "prerequisites.h"

enum OctreeNodeRegion {
    OCTREE_LEFT_BOTTOM_BACK,
    OCTREE_LEFT_BOTTOM_FRONT,
    OCTREE_LEFT_TOP_BACK,
    OCTREE_LEFT_TOP_FRONT,
    OCTREE_RIGHT_BOTTOM_BACK,
    OCTREE_RIGHT_BOTTOM_FRONT,
    OCTREE_RIGHT_TOP_BACK,
    OCTREE_RIGHT_TOP_FRONT,
};

class OctreeNodeBase {
protected:
    std::unique_ptr<OctreeNodeBase> _childNodes[8];
    int _depth{0};

    glm::vec3 _pmin{};
    glm::vec3 _pmax{};

public:
    OctreeNodeBase(glm::vec3 p0, glm::vec3 p1, int depth) : _pmin(p0), _pmax(p1), _depth(depth) {}
    virtual ~OctreeNodeBase() {}

    void getBoundingBox(glm::vec3 &p0, glm::vec3 &p1) const {
        p0 = _pmin;
        p1 = _pmax;
    }
    OctreeNodeBase *getChildNode(OctreeNodeRegion region) const {
        if (_childNodes[region]) return _childNodes[region].get();
        return nullptr;
    }
    void getRegionBoudingVolume(OctreeNodeRegion region, glm::vec3 &p0, glm::vec3 &p1) const;

    float rayNodeIntersect(glm::vec3 const &o, glm::vec3 const &d) const;
    float rayNodeIntersect2(glm::vec3 const &o, glm::vec3 const &d) const;

    virtual float rayLeafIntersect(glm::vec3 const &o, glm::vec3 const &d) const = 0;
};

template <typename T>
class OctreeNode : public OctreeNodeBase {
protected:
    std::vector<T> _vals{};

    // every four bits stand for a region ,value [0,7], if bit 3 == 0 mean no
    // intersection
    virtual uint32_t getOccupyRegions(T const &p) const = 0;

public:
    OctreeNode(glm::vec3 p0, glm::vec3 p1, int depth) : OctreeNodeBase(p0, p1, depth) {}

    virtual void insert(T const &p, int maxDepth) = 0;

    constexpr decltype(auto) childCBegin() const { return _vals.cbegin(); }
    constexpr decltype(auto) childCEnd() const { return _vals.cend(); }
    size_t getValueCount() const { return _vals.size(); }

    void getRegionValCount(std::vector<int> &countVec) const {
        auto a = getValueCount();
        if (a > 0) {
            countVec.resize((std::max)(countVec.size(), a + 1), 0);
            ++countVec[a];
        }

        for (int i = 0; i < 8; ++i) {
            if (_childNodes[(OctreeNodeRegion)i])
                static_cast<OctreeNode<T> const *>(_childNodes[(OctreeNodeRegion)i].get())->getRegionValCount(countVec);
        }
    }
};

class OctreeNodePoint : public OctreeNode<glm::vec3> {
    uint32_t getOccupyRegions(glm::vec3 const &p) const override;

public:
    OctreeNodePoint(glm::vec3 p0, glm::vec3 p1, int depth) : OctreeNode(p0, p1, depth) {}

    void insert(glm::vec3 const &p, int maxDepth) override;

    float rayLeafIntersect(glm::vec3 const &o, glm::vec3 const &d) const override { return -1; }
};

class OctreeNodeFace : public OctreeNode<size_t> {
    uint32_t getOccupyRegions(Primitive const *pPrimitive, std::vector<size_t> const &vertexIndices) const;

    uint32_t getOccupyRegions(size_t const &faceIndex) const override;

    Mesh const *_pMesh;

public:
    OctreeNodeFace(Mesh const *pMesh, glm::vec3 p0, glm::vec3 p1, int depth)
        : OctreeNode(p0, p1, depth), _pMesh(pMesh) {}

    void getFaceInfo(size_t faceIndex, Primitive const *&pPrimitive, std::vector<size_t> &vertexOffset) const;

    void insertFace(size_t faceIndex, Primitive const *pPrimitive, std::vector<size_t> const &vertexIndices,
                    int maxDepth);

    void insert(size_t const &faceIndex, int maxDepth) override;

    float rayTriIntersect(glm::vec3 const &o, glm::vec3 const &d) const;

    float rayLeafIntersect(glm::vec3 const &o, glm::vec3 const &d) const override;
};

class OctreeMesh {
    Mesh *_pMesh;

    std::unique_ptr<OctreeNodeBase> _rootNode;

    void initOctreePoints(int maxDepth);
    void initOctreeFaces(int maxDepth);

public:
    OctreeMesh(Mesh *pMesh, bool isPoints);
    OctreeMesh(Mesh *pMesh, bool isPoints, int maxdepth);

    OctreeNodeBase const *getRootNode() const { return _rootNode.get(); }

    float rayIntersect(glm::vec3 const &o, glm::vec3 const &d) const;
    float rayIntersect2(glm::vec3 const &o, glm::vec3 const &d) const;
};
