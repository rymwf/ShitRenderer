#include "octree.h"

// yz xz xy
static int octreeNodeTable[8][3]{
    {4, 2, 1},  // x y z
    {5, 3, 0},  // x y -z
    {6, 0, 3},  // x -y z
    {7, 1, 2},  // x -y -z
    {0, 6, 5},  //-x y z
    {1, 7, 4},  //-x y z
    {2, 4, 7},  //-x -y z
    {3, 5, 6},  //-x -y -z
};

void OctreeNodeBase::getRegionBoudingVolume(OctreeNodeRegion region, glm::vec3 &p0, glm::vec3 &p1) const {
    auto o = (_pmax + _pmin) / glm::vec3(2);
    auto h = (_pmax - _pmin) / glm::vec3(2);

    switch (region) {
        case OCTREE_LEFT_BOTTOM_BACK:
            p0 = {-1, -1, -1};
            p1 = {};
            break;
        case OCTREE_LEFT_BOTTOM_FRONT:
            p0 = {-1, -1, 0};
            p1 = {0, 0, 1};
            break;
        case OCTREE_LEFT_TOP_BACK:
            p0 = {-1, 0, -1};
            p1 = {0, 1, 0};
            break;
        case OCTREE_LEFT_TOP_FRONT:
            p0 = {-1, 0, 0};
            p1 = {0, 1, 1};
            break;
        case OCTREE_RIGHT_BOTTOM_BACK:
            p0 = {0, -1, -1};
            p1 = {1, 0, 0};
            break;
        case OCTREE_RIGHT_BOTTOM_FRONT:
            p0 = {0, -1, 0};
            p1 = {1, 0, 1};
            break;
        case OCTREE_RIGHT_TOP_BACK:
            p0 = {0, 0, -1};
            p1 = {1, 1, 0};
            break;
        case OCTREE_RIGHT_TOP_FRONT:
            p0 = {};
            p1 = {1, 1, 1};
            break;
    }
    float ext = 0.1;
    p0 -= ext;
    p1 += ext;
    p0 *= h;
    p1 *= h;
    p0 += o;
    p1 += o;
}
float OctreeNodeBase::rayNodeIntersect(glm::vec3 const &o, glm::vec3 const &d) const {
    auto ret = rayLeafIntersect(o, d);
    if (ret > 0) return ret;

    auto c = (_pmax + _pmin) / glm::vec3(2);
    auto h = (_pmax - _pmin) / glm::vec3(2);
    auto p = c - o;

    //=================
    float tmin = -FLT_MAX, tmax = FLT_MAX, t0, t1;

    glm::vec3 tm{FLT_MAX};

    int i;
    for (i = 0; i < 3; ++i) {
        if (abs(d[i]) > FLT_EPSILON) {
            t0 = (p[i] + h[i]) / d[i];
            t1 = (p[i] - h[i]) / d[i];

            if (t0 > t1) std::swap(t0, t1);
            if (t0 > tmin) tmin = t0;
            if (t1 < tmax) tmax = t1;
            if (tmin > tmax || tmax < 0) return -1;
        } else if (-p[i] - h[i] > 0 || -p[i] + h[i] < 0)
            return -1;
    }
    // calc mid planes intersection
    char intersectPlanes[4]{tmin < 0 ? -1 : 1};  // 0 mean no intersection
    for (i = 0; i < 3; ++i) {
        if (abs(d[i]) < FLT_EPSILON) continue;
        tm[i] = p[i] / d[i];

        if (tm[i] < 0)
            intersectPlanes[i + 1] = -i - 1;
        else
            intersectPlanes[i + 1] = i + 1;

        if (tm[i] < tmin || tm[i] > tmax) {
            tm[i] = FLT_MAX;
            intersectPlanes[i + 1] = 0;
        }

        for (int j = i; j > 0; --j) {
            if (tm[j] < tm[j - 1]) {
                std::swap(tm[j], tm[j - 1]);
                std::swap(intersectPlanes[j + 1], intersectPlanes[j]);
            }
        }
    }
    // enable last region search
    if (tmin < 0) {
        int j = 1;
        for (; j < 4; ++j) {
            if (intersectPlanes[j] >= 0) {
                intersectPlanes[j - 1] = abs(intersectPlanes[j - 1]);
                break;
            }
        }
        if (j == 4) {
            for (j = 3; j >= 0; --j) {
                if (intersectPlanes[j] != 0) {
                    intersectPlanes[j] = abs(intersectPlanes[j]);
                    break;
                }
            }
        }
    }

    // region
    char intersectRegions[4]{};

    auto inPoint = -p + tmin * d;
    if (inPoint.x > 0) intersectRegions[0] |= 4;
    if (inPoint.y > 0) intersectRegions[0] |= 2;
    if (inPoint.z > 0) intersectRegions[0] |= 1;

    for (i = 1; i < 4; ++i) {
        if (intersectPlanes[i] == 0) break;
        intersectRegions[i] = octreeNodeTable[intersectRegions[i - 1]][abs(intersectPlanes[i]) - 1];
    }
    OctreeNodeBase *pNode;
    for (i = 0; i < 4; ++i) {
        if (intersectPlanes[i] > 0) {
            pNode = getChildNode(OctreeNodeRegion(intersectRegions[i]));
            if (pNode) {
                ret = pNode->rayNodeIntersect(o, d);
                if (ret > 0) return ret;
            }
        }
    }
    return -1;
}
float OctreeNodeBase::rayNodeIntersect2(glm::vec3 const &o, glm::vec3 const &d) const {
    if (RayAABBIntersect(o, d, _pmin, _pmax) < 0) return -1;

    auto ret = rayLeafIntersect(o, d);
    if (ret > 0) return ret;

    ret = FLT_MAX;
    OctreeNodeBase *pNode;
    float t;
    for (int i = 0; i < 8; ++i) {
        pNode = getChildNode(OctreeNodeRegion(i));
        if (pNode) {
            t = pNode->rayNodeIntersect2(o, d);
            if (t > 0) ret = (std::min)(ret, t);
        }
    }
    return ret == FLT_MAX ? -1 : ret;
}

void OctreeNodePoint::insert(glm::vec3 const &p, int maxDepth) {
    //// check if out of bounding volume
    // if (p[0] < _pmin[0] ||
    //	p[1] < _pmin[1] ||
    //	p[2] < _pmin[2] ||
    //	p[0] > _pmax[0] ||
    //	p[1] > _pmax[1] ||
    //	p[2] > _pmax[2])
    //	return;

    bool flag = false;
    if (_vals.empty()) {
        flag = true;
        for (int i = 0; i < 8; ++i) {
            if (_childNodes[i]) {
                flag = false;
                break;
            }
        }
    }
    flag |= _depth == maxDepth;

    //
    if (flag) {
        _vals.emplace_back(p);
        return;
    }
    // leaf node, divide node
    auto region = getOccupyRegions(p);
    glm::vec3 p0, p1;
    getRegionBoudingVolume(OctreeNodeRegion(region), p0, p1);

    OctreeNodePoint *pNode;
    if (_childNodes[region])
        pNode = static_cast<OctreeNodePoint *>(_childNodes[region].get());
    else {
        pNode = static_cast<OctreeNodePoint *>(
            (_childNodes[region] = std::make_unique<OctreeNodePoint>(p0, p1, _depth + 1)).get());
    }
    pNode->insert(p, maxDepth);

    for (auto &&e : _vals) {
        region = getOccupyRegions(e);
        getRegionBoudingVolume(OctreeNodeRegion(region), p0, p1);
        if (_childNodes[region])
            pNode = static_cast<OctreeNodePoint *>(_childNodes[region].get());
        else {
            pNode = static_cast<OctreeNodePoint *>(
                (_childNodes[region] = std::make_unique<OctreeNodePoint>(p0, p1, _depth + 1)).get());
        }
        pNode->insert(e, maxDepth);
    }
    _vals.clear();
}

uint32_t OctreeNodePoint::getOccupyRegions(glm::vec3 const &p) const {
    glm::vec3 pmin, pmax;
    for (uint32_t i = 0; i < 8; ++i) {
        getRegionBoudingVolume(OctreeNodeRegion(i), pmin, pmax);
        if (glm::all(glm::greaterThanEqual(p, pmin)) && glm::all(glm::lessThanEqual(p, pmax))) return i;
    }
    return 0;
}
//================================
void OctreeNodeFace::getFaceInfo(size_t faceIndex, Primitive const *&pPrimitive,
                                 std::vector<size_t> &vertexOffset) const {
    size_t primitiveIndex = 0;
    size_t offset = 0;
    size_t tempIndex;
    size_t indexOffset;
    for (auto &&e : _pMesh->primitives) {
        tempIndex = faceIndex - offset;
        indexOffset = e->getFaceIndexOffset(tempIndex);
        if (indexOffset < e->indices.size()) break;
        offset += e->indices.size() / getTopologyFaceStep(e->topology);
        ++primitiveIndex;
    }
    pPrimitive = _pMesh->primitives.at(primitiveIndex).get();
    auto faceVertexCount = getTopologyFaceStep(pPrimitive->topology);
    vertexOffset.resize(faceVertexCount);
    for (uint32_t i = 0; i < faceVertexCount; ++i) {
        vertexOffset[i] = pPrimitive->indices[indexOffset + i];
    }
}

uint32_t OctreeNodeFace::getOccupyRegions(Primitive const *pPrimitive, std::vector<size_t> const &vertexIndices) const {
    uint32_t ret{};

    // get triangle aabb box
    glm::vec3 faceMinVal{FLT_MAX}, faceMaxVal{-FLT_MAX};

    for (auto e : vertexIndices) {
        auto &&vertex = pPrimitive->positions[e];
        faceMinVal = (glm::min)(faceMinVal, vertex);
        faceMaxVal = (glm::max)(faceMaxVal, vertex);
    }

    glm::vec3 p0, p1;

    for (int i = 0; i < 8; ++i) {
        getRegionBoudingVolume(OctreeNodeRegion(i), p0, p1);
        if (AABB_intersect(faceMinVal, faceMaxVal, p0, p1)) {
            ret <<= 4;
            ret |= 8;
            ret |= i;
        }
    }
    return ret;
}
uint32_t OctreeNodeFace::getOccupyRegions(size_t const &faceIndex) const {
    uint32_t ret{};
    Primitive const *pPrimitive;
    std::vector<size_t> vertexIndices;
    getFaceInfo(faceIndex, pPrimitive, vertexIndices);
    return getOccupyRegions(pPrimitive, vertexIndices);
}

void OctreeNodeFace::insertFace(size_t faceIndex, Primitive const *pPrimitive, std::vector<size_t> const &vertexIndices,
                                int maxDepth) {
    bool flag = false;
    if (_vals.size() < 10) {
        flag = true;
        for (int i = 0; i < 8; ++i) {
            if (_childNodes[i]) {
                flag = false;
                break;
            }
        }
    }
    flag |= _depth == maxDepth;

    if (flag) {
        _vals.emplace_back(faceIndex);
        return;
    }
    auto regions = getOccupyRegions(pPrimitive, vertexIndices);
    glm::vec3 p0, p1;
    char region = 0;
    while (regions != 0) {
        region = regions & 7;
        getRegionBoudingVolume(OctreeNodeRegion(region), p0, p1);
        OctreeNodeFace *pNode = static_cast<OctreeNodeFace *>(getChildNode(OctreeNodeRegion(region)));
        if (!pNode)
            pNode = static_cast<OctreeNodeFace *>(
                (_childNodes[region] = std::make_unique<OctreeNodeFace>(_pMesh, p0, p1, _depth + 1)).get());
        pNode->insertFace(faceIndex, pPrimitive, vertexIndices, maxDepth);
        regions >>= 4;
    }

    for (auto e : _vals) {
        std::vector<size_t> tempVertexIndices;
        getFaceInfo(e, pPrimitive, tempVertexIndices);
        regions = getOccupyRegions(pPrimitive, tempVertexIndices);

        while (regions != 0) {
            region = regions & 7;
            getRegionBoudingVolume(OctreeNodeRegion(region), p0, p1);
            OctreeNodeFace *pNode = static_cast<OctreeNodeFace *>(getChildNode(OctreeNodeRegion(region)));
            if (!pNode)
                pNode = static_cast<OctreeNodeFace *>(
                    (_childNodes[region] = std::make_unique<OctreeNodeFace>(_pMesh, p0, p1, _depth + 1)).get());
            pNode->insertFace(e, pPrimitive, tempVertexIndices, maxDepth);
            regions >>= 4;
        }
    }
    _vals.clear();
}
void OctreeNodeFace::insert(size_t const &faceIndex, int maxDepth) {
    Primitive const *pPrimitive;
    std::vector<size_t> vertexIndices;
    getFaceInfo(faceIndex, pPrimitive, vertexIndices);
    insertFace(faceIndex, pPrimitive, vertexIndices, maxDepth);
}
float OctreeNodeFace::rayTriIntersect(glm::vec3 const &o, glm::vec3 const &d) const {
    Primitive const *pPrimitive;
    std::vector<size_t> vertexOffset;
    float ret(FLT_MAX);
    float t;
    for (auto e : _vals) {
        getFaceInfo(e, pPrimitive, vertexOffset);

        t = RayTriIntersect(o, d, pPrimitive->positions[vertexOffset[0]], pPrimitive->positions[vertexOffset[1]],
                            pPrimitive->positions[vertexOffset[2]])
                .z;
        if (t > 0) ret = (std::min)(ret, t);
    }
    return ret == FLT_MAX ? -1 : ret;
}
float OctreeNodeFace::rayLeafIntersect(glm::vec3 const &o, glm::vec3 const &d) const { return rayTriIntersect(o, d); }

//================================
OctreeMesh::OctreeMesh(Mesh *pMesh, bool isPoints) : _pMesh(pMesh) {
    if (isPoints)
        initOctreePoints(9);
    else
        initOctreeFaces(9);
}
OctreeMesh::OctreeMesh(Mesh *pMesh, bool isPoints, int maxdepth) : _pMesh(pMesh) {
    if (isPoints)
        initOctreePoints(maxdepth);
    else
        initOctreeFaces(maxdepth);
}
void OctreeMesh::initOctreePoints(int maxDepth) {
    _rootNode = std::make_unique<OctreeNodePoint>(_pMesh->boundingVolume.box.aabb.minValue,
                                                  _pMesh->boundingVolume.box.aabb.maxValue, 0);

    auto pNode = static_cast<OctreeNodePoint *>(_rootNode.get());

    for (auto &&e : _pMesh->primitives) {
        for (auto &&e2 : e->positions) {
            pNode->insert(e2, maxDepth);
        }
    }
    // check region point count
    std::vector<int> pointCount;
    pNode->getRegionValCount(pointCount);
    for (size_t i = 0; i < pointCount.size(); ++i) {
        LOG(i, " points region count ", pointCount[i]);
    }
}
void OctreeMesh::initOctreeFaces(int maxDepth) {
    _rootNode = std::make_unique<OctreeNodeFace>(_pMesh, _pMesh->boundingVolume.box.aabb.minValue,
                                                 _pMesh->boundingVolume.box.aabb.maxValue, 0);
    auto pNode = static_cast<OctreeNodeFace *>(_rootNode.get());

    size_t faceIndex = 0;
    for (auto &&e : _pMesh->primitives) {
        auto l = e->faceMaterialIds.size();
        for (size_t i = 0; i < l; ++i) {
            pNode->insert(i + faceIndex, maxDepth);
        }
        faceIndex += l;
    }

    // check region point count
    std::vector<int> pointCount;
    pNode->getRegionValCount(pointCount);
    for (size_t i = 0; i < pointCount.size(); ++i) {
        LOG(i, " faces region count ", pointCount[i]);
    }
}
float OctreeMesh::rayIntersect(glm::vec3 const &o, glm::vec3 const &d) const {
    return _rootNode->rayNodeIntersect(o, d);
}
float OctreeMesh::rayIntersect2(glm::vec3 const &o, glm::vec3 const &d) const {
    return _rootNode->rayNodeIntersect2(o, d);
}