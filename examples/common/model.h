#pragma once
#include "component.h"
#include "glm/glm.hpp"
#include "material.h"
#include "mesh.h"
#include "prerequisites.h"
#include "renderer.h"
#include "resource.h"

class Model;

class ModelLoaderObj : public ResourceLoader {
public:
    void load(Resource *resource) override;
};

// class MyModelLoader : public ResourceLoader
//{
// public:
//	void load(Resource *resource) override;
// };

//======================
class PrimitiveRenderer;

class Model : public Resource {
    std::vector<std::unique_ptr<Mesh>> _meshes;

    bool _isCalcDistanceAlongNormal{false};
    int _octreePointsDepth{-2};  //-2 mean disable, -1 mean auto depth
    int _octreeFacesDepth{-2};   //-2 mean disable, -1 mean auto depth

public:
    Model(std::string_view name, std::string_view path, ResourceLoader *loader) : Resource(name, path, loader) {}
    ~Model();

    Mesh *createMesh() { return _meshes.emplace_back(std::make_unique<Mesh>()).get(); }

    decltype(auto) meshCbegin() const { return _meshes.cbegin(); }
    decltype(auto) meshCend() const { return _meshes.cend(); }

    decltype(auto) meshBegin() { return _meshes.begin(); }
    decltype(auto) meshEnd() { return _meshes.end(); }

    constexpr Model *enableCalcDistanceAlongNormal() {
        _isCalcDistanceAlongNormal = true;
        return this;
    }
    constexpr Model *enableGenerateOctreePoints(int depth = -1) {
        _octreePointsDepth = depth;
        return this;
    }
    constexpr Model *enableGenerateOctreeFaces(int depth = -1) {
        _octreeFacesDepth = depth;
        return this;
    }
    constexpr bool isEnableCalcDistanceAlongNormal() const { return _isCalcDistanceAlongNormal; }

    constexpr int octreePointsDepth() const { return _octreePointsDepth; }
    constexpr int octreeFacesDepth() const { return _octreeFacesDepth; }
};

class ModelManager : public ResourceManager {
    Resource *createImpl(std::string_view path, ResourceLoader *loader, ParameterMap const &parameters) override;

public:
    ModelManager();

    Model *createModel(std::string_view path, std::string_view modelLoaderName = "obj");
};

//============================