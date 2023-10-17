#include "renderer.h"

#include "appbase.h"
#include "transform.h"

//=================
PrimitiveRenderer::PrimitiveRenderer(Primitive *primitive) : _primitive(primitive) {
    _positionBuffer =
        g_App->getDevice()->Create(Shit::BufferCreateInfo{{},
                                                          _primitive->positions.size() * sizeof(glm::vec3),
                                                          Shit::BufferUsageFlagBits::VERTEX_BUFFER_BIT |
                                                              Shit::BufferUsageFlagBits::TRANSFER_DST_BIT,
                                                          Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT},
                                   _primitive->positions.data());

    if (!_primitive->normals.empty()) {
        _normalBuffer =
            g_App->getDevice()->Create(Shit::BufferCreateInfo{{},
                                                              _primitive->normals.size() * sizeof(glm::vec3),
                                                              Shit::BufferUsageFlagBits::VERTEX_BUFFER_BIT |
                                                                  Shit::BufferUsageFlagBits::TRANSFER_DST_BIT,
                                                              Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT},
                                       _primitive->normals.data());
    }
    if (!_primitive->texcoords.empty()) {
        _texcoordBuffer =
            g_App->getDevice()->Create(Shit::BufferCreateInfo{{},
                                                              _primitive->texcoords.size() * sizeof(glm::vec2),
                                                              Shit::BufferUsageFlagBits::VERTEX_BUFFER_BIT |
                                                                  Shit::BufferUsageFlagBits::TRANSFER_DST_BIT,
                                                              Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT},
                                       _primitive->texcoords.data());
    }
    if (!_primitive->indices.empty()) {
        _indexBuffer =
            g_App->getDevice()->Create(Shit::BufferCreateInfo{{},
                                                              _primitive->indices.size() * sizeof(uint32_t),
                                                              Shit::BufferUsageFlagBits::INDEX_BUFFER_BIT |
                                                                  Shit::BufferUsageFlagBits::TRANSFER_DST_BIT,
                                                              Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT},
                                       _primitive->indices.data());
    }
    if (!_primitive->distanceAlongNormal.empty()) {
        _distanceAlongNormalBuffer =
            g_App->getDevice()->Create(Shit::BufferCreateInfo{{},
                                                              _primitive->distanceAlongNormal.size() * sizeof(float),
                                                              Shit::BufferUsageFlagBits::VERTEX_BUFFER_BIT |
                                                                  Shit::BufferUsageFlagBits::TRANSFER_DST_BIT,
                                                              Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT},
                                       _primitive->distanceAlongNormal.data());
    }
    MaterialDataBlock *materialDataBlock;
    size_t i = 1, l = 0;
    for (size_t len = _primitive->faceMaterialIds.size(); i < len; ++i) {
        if (_primitive->faceMaterialIds[i] != _primitive->faceMaterialIds[i - 1]) {
            if (g_App->getMaterialDataBlockManager()->containsMaterialDataBlock(_primitive->faceMaterialIds[l]))
                materialDataBlock =
                    g_App->getMaterialDataBlockManager()->getMaterialDataBlockById(_primitive->faceMaterialIds[l]);
            else
                materialDataBlock = g_App->getMaterialDataBlockManager()->getMaterialDataBlockById();
            auto materialDataBlock2 = g_App->getMaterialDataBlockManager()->createMaterialDataBlock(materialDataBlock);
            materialDataBlock2->prepare();
            _drawCommands.emplace_back(materialDataBlock2, (uint32_t)_primitive->getFaceIndexOffset(l),
                                       getTopologyFaceStep(_primitive->topology) * (uint32_t)(i - l));
            // l is face index
            l = i;
        }
    }
    if (g_App->getMaterialDataBlockManager()->containsMaterialDataBlock(_primitive->faceMaterialIds[l]))
        materialDataBlock =
            g_App->getMaterialDataBlockManager()->getMaterialDataBlockById(_primitive->faceMaterialIds[l]);
    else
        materialDataBlock = g_App->getMaterialDataBlockManager()->getMaterialDataBlockById();
    auto materialDataBlock2 = g_App->getMaterialDataBlockManager()->createMaterialDataBlock(materialDataBlock);
    materialDataBlock2->prepare();
    _drawCommands.emplace_back(materialDataBlock2, (uint32_t)_primitive->getFaceIndexOffset(l),
                               getTopologyFaceStep(_primitive->topology) * uint32_t(i - l));
}
PrimitiveRenderer::~PrimitiveRenderer() {
    g_App->getDevice()->Destroy(_positionBuffer);
    g_App->getDevice()->Destroy(_normalBuffer);
    g_App->getDevice()->Destroy(_texcoordBuffer);
    g_App->getDevice()->Destroy(_indexBuffer);
    g_App->getDevice()->Destroy(_distanceAlongNormalBuffer);

    for (auto &&e : _drawCommands) {
        g_App->getMaterialDataBlockManager()->removeMaterialDataBlockById(e.materialDataBlock->getId());
    }
}
void PrimitiveRenderer::update() {
    for (auto &&e : _drawCommands) {
        e.materialDataBlock->update();
    }
}
void PrimitiveRenderer::draw(Shit::CommandBuffer *cmdBuffer, Shit::PipelineLayout const *pipelineLayout,
                             uint32_t materialSetNumber) {
    Shit::Buffer *buffers[]{
        _positionBuffer,
        _normalBuffer,
        _texcoordBuffer,
        _distanceAlongNormalBuffer,
    };
    static uint64_t offsets[4]{};

    cmdBuffer->BindVertexBuffers(Shit::BindVertexBuffersInfo{0, (uint32_t)std::size(buffers), buffers, offsets});

    if (_indexBuffer) {
        cmdBuffer->BindIndexBuffer(Shit::BindIndexBufferInfo{_indexBuffer, 0, Shit::IndexType::UINT32});
    }
    for (auto &&e : _drawCommands) {
        // bind material
        if (pipelineLayout && materialSetNumber != ~(0u)) {
            auto a = e.materialDataBlock->getDescriptorSet();
            cmdBuffer->BindDescriptorSets(Shit::BindDescriptorSetsInfo{Shit::PipelineBindPoint::GRAPHICS,
                                                                       pipelineLayout, materialSetNumber, 1, &a});
        }
        if (_indexBuffer) {
            cmdBuffer->DrawIndexed(Shit::DrawIndexedIndirectCommand{e.vertexCount, 1, e.startVertex, 0, 0});
        } else {
            cmdBuffer->Draw(Shit::DrawIndirectCommand{e.vertexCount, 1, e.startVertex, 0});
        }
    }
}

MeshRenderer::MeshRenderer(GameObject *parent, Mesh *mesh) : Component(parent), _mesh(mesh) {
    for (auto &&e : _mesh->primitives) {
        _renderers.emplace_back(std::make_unique<PrimitiveRenderer>(e.get()));
    }
}
void MeshRenderer::prepareImpl() {}
void MeshRenderer::updateImpl() {
    for (auto &&e : _renderers) {
        e->update();
    }
}
MeshRenderer::~MeshRenderer() {}
void MeshRenderer::draw(Shit::CommandBuffer *cmdBuffer, Shit::PipelineLayout const *pipelineLayout,
                        uint32_t materialSetNumber) {
    for (auto &&e : _renderers) {
        e->draw(cmdBuffer, pipelineLayout, materialSetNumber);
    }
}
//=========================================
PrimitivePointRenderer::PrimitivePointRenderer(Primitive *primitive) : _primitive(primitive) {
    _positionBuffer =
        g_App->getDevice()->Create(Shit::BufferCreateInfo{{},
                                                          _primitive->positions.size() * sizeof(glm::vec3),
                                                          Shit::BufferUsageFlagBits::VERTEX_BUFFER_BIT |
                                                              Shit::BufferUsageFlagBits::TRANSFER_DST_BIT,
                                                          Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT},
                                   _primitive->positions.data());

    _drawCommands.emplace_back(0u, (uint32_t)_primitive->positions.size());
}
PrimitivePointRenderer::~PrimitivePointRenderer() { g_App->getDevice()->Destroy(_positionBuffer); }
void PrimitivePointRenderer::draw(Shit::CommandBuffer *cmdBuffer) {
    static uint64_t offsets[1]{};
    cmdBuffer->BindVertexBuffers(Shit::BindVertexBuffersInfo{0, 1, &_positionBuffer, offsets});
    for (auto &&e : _drawCommands) {
        cmdBuffer->Draw(Shit::DrawIndirectCommand{e.vertexCount, 1, e.startVertex, 0});
    }
}
//==================
MeshPointRenderer::MeshPointRenderer(GameObject *parent, Mesh *mesh) : Component(parent), _mesh(mesh) {
    for (auto &&e : _mesh->primitives) {
        _renderers.emplace_back(std::make_unique<PrimitivePointRenderer>(e.get()));
    }
}
MeshPointRenderer::~MeshPointRenderer() {}
void MeshPointRenderer::draw(Shit::CommandBuffer *cmdBuffer) {
    for (auto &&e : _renderers) {
        e->draw(cmdBuffer);
    }
}

//=========================

void OctreeMeshRenderer::getOctreeMeshPositions(OctreeNodeBase const *pNode, std::vector<glm::vec3> &pos) {
    if (!pNode) return;
    glm::vec3 p0, p1;
    pNode->getBoundingBox(p0, p1);
    auto o = (p0 + p1) / glm::vec3(2);
    // x
    pos.emplace_back(o.x, p0.y, p0.z);
    pos.emplace_back(o.x, p1.y, p0.z);

    pos.emplace_back(o.x, p1.y, p0.z);
    pos.emplace_back(o.x, p1.y, p1.z);

    pos.emplace_back(o.x, p1.y, p1.z);
    pos.emplace_back(o.x, p0.y, p1.z);

    pos.emplace_back(o.x, p0.y, p1.z);
    pos.emplace_back(o.x, p0.y, p0.z);

    // y

    pos.emplace_back(p0.x, o.y, p0.z);
    pos.emplace_back(p0.x, o.y, p1.z);

    pos.emplace_back(p0.x, o.y, p1.z);
    pos.emplace_back(p1.x, o.y, p1.z);

    pos.emplace_back(p1.x, o.y, p1.z);
    pos.emplace_back(p1.x, o.y, p0.z);

    pos.emplace_back(p1.x, o.y, p0.z);
    pos.emplace_back(p0.x, o.y, p0.z);

    // z
    pos.emplace_back(p0.x, p0.y, o.z);
    pos.emplace_back(p1.x, p0.y, o.z);

    pos.emplace_back(p1.x, p0.y, o.z);
    pos.emplace_back(p1.x, p1.y, o.z);

    pos.emplace_back(p1.x, p1.y, o.z);
    pos.emplace_back(p0.x, p1.y, o.z);

    pos.emplace_back(p0.x, p1.y, o.z);
    pos.emplace_back(p0.x, p0.y, o.z);

    for (int i = 0; i < 8; ++i) {
        getOctreeMeshPositions(pNode->getChildNode(OctreeNodeRegion(i)), pos);
    }
}

OctreeMeshRenderer::OctreeMeshRenderer(GameObject *parent, OctreeMesh *pOctreeMesh)
    : Component(parent), _pOctreeMesh(pOctreeMesh) {
    getOctreeMeshPositions(_pOctreeMesh->getRootNode(), _positions);

    _positionBuffer = g_App->getDevice()->Create(Shit::BufferCreateInfo{{},
                                                                        _positions.size() * sizeof(glm::vec3),
                                                                        Shit::BufferUsageFlagBits::VERTEX_BUFFER_BIT |
                                                                            Shit::BufferUsageFlagBits::TRANSFER_DST_BIT,
                                                                        Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT},
                                                 _positions.data());

    _drawCommand = {0u, (uint32_t)_positions.size()};
}
void OctreeMeshRenderer::draw(Shit::CommandBuffer *cmdBuffer) {
    static uint64_t offsets[1]{};
    cmdBuffer->BindVertexBuffers(Shit::BindVertexBuffersInfo{0, 1, &_positionBuffer, offsets});
    cmdBuffer->Draw(Shit::DrawIndirectCommand{_drawCommand.vertexCount, 1, _drawCommand.startVertex, 0});
}