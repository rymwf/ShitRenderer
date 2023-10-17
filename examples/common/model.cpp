#include "model.h"

#include <array>

#include "appbase.h"
#include "common.h"
#include "glm/gtx/hash.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

struct TempVertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 texCoord;
};
inline bool operator==(const TempVertex &lhv, const TempVertex &rhv) {
    return lhv.pos == rhv.pos && lhv.normal == rhv.normal && lhv.texCoord == rhv.texCoord;
}

namespace std {
template <>
struct hash<TempVertex> {
    std::size_t operator()(TempVertex const &vertex) const noexcept {
        return ((hash<glm::vec3>{}(vertex.pos) ^ hash<glm::vec3>{}(vertex.normal) << 1) >> 1) ^
               (hash<glm::vec2>{}(vertex.texCoord) << 1);
    }
};
}  // namespace std

void ModelLoaderObj::load(Resource *resource) {
    Model *dstModel = static_cast<Model *>(resource);
    auto parentPath = std::filesystem::path(dstModel->getPath()).parent_path();

    //===============
    tinyobj::ObjReaderConfig reader_config;
    reader_config.mtl_search_path = parentPath.string();  // Path to material files

    tinyobj::ObjReader reader;

    if (!reader.ParseFromFile(std::string(dstModel->getPath()), reader_config)) {
        if (!reader.Error().empty()) {
            std::cerr << "TinyObjReader: " << reader.Error();
        }
        exit(1);
    }

    if (!reader.Warning().empty()) {
        std::cout << "TinyObjReader: " << reader.Warning();
    }

    auto &attrib = reader.GetAttrib();
    auto &shapes = reader.GetShapes();
    auto &materials = reader.GetMaterials();

    std::vector<uint32_t> materialIds;

    for (auto &&material : materials) {
        auto m = g_App->getMaterialDataBlockManager()->createMaterialDataBlock();
        materialIds.emplace_back(m->getId());
        auto pUboData = m->getUBOData();
        for (int i = 0; i < 3; ++i) {
            pUboData->ambient[i] = material.ambient[i];
            pUboData->diffuse[i] = material.diffuse[i];
            pUboData->specular[i] = material.specular[i];
            pUboData->transmittance[i] = material.transmittance[i];
            pUboData->emission[i] = material.emission[i];
        }
        pUboData->shininess = material.shininess;
        pUboData->ior = material.ior;
        pUboData->dissolve = material.dissolve;

        pUboData->roughness = material.roughness;
        pUboData->metallic = material.metallic;
        pUboData->sheen = material.sheen;
        pUboData->clearcoat_thickness = material.clearcoat_thickness;
        pUboData->clearcoat_roughness = material.clearcoat_roughness;
        pUboData->anisotropy = material.anisotropy;
        pUboData->anisotropy_rotation = material.anisotropy_rotation;

        if (!material.ambient_texname.empty()) {
            m->setAmbientTex(reader_config.mtl_search_path + "/" + material.ambient_texname);
        }
        if (!material.diffuse_texname.empty()) {
            m->setDiffuseTex(reader_config.mtl_search_path + "/" + material.diffuse_texname);
            m->setTextureOffset(material.diffuse_texopt.origin_offset[0], material.diffuse_texopt.origin_offset[1],
                                material.diffuse_texopt.origin_offset[2]);
            m->setTextureScale(material.diffuse_texopt.scale[0], material.diffuse_texopt.scale[1],
                               material.diffuse_texopt.scale[2]);
        }
        if (!material.specular_texname.empty()) {
            m->setSpecularTex(reader_config.mtl_search_path + "/" + material.specular_texname);
        }
        if (!material.specular_highlight_texname.empty()) {
            m->setSpecularHighlightTex(reader_config.mtl_search_path + "/" + material.specular_highlight_texname);
        }
        if (!material.bump_texname.empty()) {
            m->setBumpTex(reader_config.mtl_search_path + "/" + material.bump_texname);
        }
        if (!material.displacement_texname.empty()) {
            m->setDisplacementTex(reader_config.mtl_search_path + "/" + material.displacement_texname);
        }
        if (!material.alpha_texname.empty()) {
            m->setAlphaTex(reader_config.mtl_search_path + "/" + material.alpha_texname);
        }
        if (!material.reflection_texname.empty()) {
            m->setRelectionTex(reader_config.mtl_search_path + "/" + material.reflection_texname);
        }
        // pbr
        if (!material.roughness_texname.empty()) {
            m->setRoughnessTex(reader_config.mtl_search_path + "/" + material.roughness_texname);
        }
        if (!material.metallic_texname.empty()) {
            m->setMetallicTex(reader_config.mtl_search_path + "/" + material.metallic_texname);
        }
        if (!material.sheen_texname.empty()) {
            m->setSheenTex(reader_config.mtl_search_path + "/" + material.sheen_texname);
        }
        if (!material.emissive_texname.empty()) {
            m->setEmissiveTex(reader_config.mtl_search_path + "/" + material.emissive_texname);
        }
    }

    static bool flipY =
        (g_App->getRendererVersion() & Shit::RendererVersion::TypeBitmask) == Shit::RendererVersion::VULKAN;

    for (auto &&shape : shapes) {
        auto newMesh = dstModel->createMesh();
        auto trianglePrimitive =
            newMesh->primitives.emplace_back(std::make_unique<Primitive>(Shit::PrimitiveTopology::TRIANGLE_LIST)).get();

        size_t indexOffset = 0;

        std::unordered_map<TempVertex, uint32_t> vertexIndexMap;
        TempVertex tempVertex;

        // trangulated , fv is 3
        for (size_t f = 0, lenf = shape.mesh.num_face_vertices.size(); f < lenf; ++f) {
            size_t fv = size_t(shape.mesh.num_face_vertices[f]);
            for (unsigned char v = 0; v < fv; ++v, ++indexOffset) {
                auto index = shape.mesh.indices[indexOffset];
                tempVertex = {};

                auto vx = attrib.vertices[3 * index.vertex_index + 0];
                auto vy = attrib.vertices[3 * index.vertex_index + 1];
                auto vz = attrib.vertices[3 * index.vertex_index + 2];
                tempVertex.pos = {vx, vy, vz};
                newMesh->boundingVolume.box.aabb.merge(tempVertex.pos);

                if (index.normal_index >= 0) {
                    auto nx = attrib.normals[3 * index.normal_index + 0];
                    auto ny = attrib.normals[3 * index.normal_index + 1];
                    auto nz = attrib.normals[3 * index.normal_index + 2];
                    tempVertex.normal = {nx, ny, nz};
                }
                if (index.texcoord_index >= 0) {
                    auto tx = attrib.texcoords[2 * index.texcoord_index + 0];
                    auto ty = attrib.texcoords[2 * index.texcoord_index + 1];
                    if (flipY) ty = 1. - ty;
                    tempVertex.texCoord = {tx, ty};
                }
                auto it = vertexIndexMap.find(tempVertex);
                if (it == vertexIndexMap.end()) {
                    trianglePrimitive->indices.emplace_back(vertexIndexMap[tempVertex] =
                                                                (uint32_t)trianglePrimitive->positions.size());
                    trianglePrimitive->positions.emplace_back(tempVertex.pos);
                    trianglePrimitive->normals.emplace_back(tempVertex.normal);
                    trianglePrimitive->texcoords.emplace_back(tempVertex.texCoord);
                } else {
                    trianglePrimitive->indices.emplace_back(it->second);
                }
            }
            // in obj, faces with same material were usually put together
            if (shape.mesh.material_ids[f] == -1)
                trianglePrimitive->faceMaterialIds.emplace_back(-1);
            else
                trianglePrimitive->faceMaterialIds.emplace_back(materialIds[shape.mesh.material_ids[f]]);
        }
        trianglePrimitive->positions.shrink_to_fit();
        trianglePrimitive->normals.shrink_to_fit();
        trianglePrimitive->texcoords.shrink_to_fit();
        trianglePrimitive->indices.shrink_to_fit();
        newMesh->generateOctreePoints(dstModel->octreePointsDepth());
        newMesh->generateOctreeFaces(dstModel->octreeFacesDepth());
        if (dstModel->isEnableCalcDistanceAlongNormal()) newMesh->calculateDistanceAlongNormal();
    }
}
Model::~Model() {}
//=======================================
ModelManager::ModelManager() { registerResourceLoader<ModelLoaderObj>(DEFAULT_LOADER_NAME); }
Resource *ModelManager::createImpl(std::string_view path, ResourceLoader *loader, ParameterMap const &parameters) {
    return new Model(path, path, loader);
}