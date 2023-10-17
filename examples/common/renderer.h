#pragma once
#include "material.h"
#include "mesh.h"
#include "octree.h"

//
class PrimitiveRenderer {
    Primitive *_primitive;

    Shit::Buffer *_positionBuffer{};  // locatin 0
    Shit::Buffer *_normalBuffer{};    // locatin 1
    Shit::Buffer *_texcoordBuffer{};  // locatin 2
    Shit::Buffer *_indexBuffer{};
    Shit::Buffer *_distanceAlongNormalBuffer{};

    struct DrawCommand {
        MaterialDataBlock *materialDataBlock{};
        uint32_t startVertex;
        uint32_t vertexCount;
    };
    std::vector<DrawCommand> _drawCommands;

public:
    PrimitiveRenderer(Primitive *primitive);
    ~PrimitiveRenderer();

    void update();

    MaterialDataBlock *getMaterialDataBlock(uint32_t index = 0) const {
        return _drawCommands.at(index).materialDataBlock;
    }

    void draw(Shit::CommandBuffer *cmdBuffer, Shit::PipelineLayout const *pipelineLayout, uint32_t materialSetNumber);
};

//
class MeshRenderer : public Component {
    Mesh *_mesh;
    std::vector<std::unique_ptr<PrimitiveRenderer>> _renderers;

    void updateImpl() override;
    void prepareImpl() override;

public:
    MeshRenderer(GameObject *parent, Mesh *mesh);
    ~MeshRenderer();
    constexpr Mesh *getMesh() const { return _mesh; }

    size_t getRenderersCount() const { return _renderers.size(); }
    PrimitiveRenderer *getPrimitiveRenderer(size_t index) const { return _renderers.at(index).get(); }

    // constexpr void setMaterialDataBlock(MaterialDataBlock *materialDataBlock)
    //{
    // }

    void draw(Shit::CommandBuffer *cmdBuffer, Shit::PipelineLayout const *pipelineLayout = nullptr,
              uint32_t materialSetNumber = ~(0u));
};

//=============================
class PrimitivePointRenderer {
    Primitive *_primitive;

    Shit::Buffer *_positionBuffer{};  // locatin 0

    struct DrawCommand {
        uint32_t startVertex;
        uint32_t vertexCount;
    };
    std::vector<DrawCommand> _drawCommands;

public:
    PrimitivePointRenderer(Primitive *primitive);
    ~PrimitivePointRenderer();

    void draw(Shit::CommandBuffer *cmdBuffer);
};

//
class MeshPointRenderer : public Component {
    Mesh *_mesh;
    std::vector<std::unique_ptr<PrimitivePointRenderer>> _renderers;

public:
    MeshPointRenderer(GameObject *parent, Mesh *mesh);
    virtual ~MeshPointRenderer();
    constexpr Mesh *getMesh() const { return _mesh; }

    size_t getRenderersCount() const { return _renderers.size(); }
    PrimitivePointRenderer *getPrimitiveRenderer(size_t index) const { return _renderers.at(index).get(); }

    void draw(Shit::CommandBuffer *cmdBuffer);
};

class OctreeMeshRenderer : public Component {
    OctreeMesh *_pOctreeMesh;

    std::vector<glm::vec3> _positions;

    Shit::Buffer *_positionBuffer{};  // locatin 0
    struct DrawCommand {
        uint32_t startVertex;
        uint32_t vertexCount;
    };
    DrawCommand _drawCommand;

    void getOctreeMeshPositions(OctreeNodeBase const *pNode, std::vector<glm::vec3> &pos);

public:
    OctreeMeshRenderer(GameObject *parent, OctreeMesh *pOctreeMesh);
    virtual ~OctreeMeshRenderer() {}

    void draw(Shit::CommandBuffer *cmdBuffer);
};