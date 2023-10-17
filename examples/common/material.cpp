#include "material.h"

#include "appbase.h"

MaterialDataBlock::~MaterialDataBlock() {
    g_App->getDescriptorPool()->Free(1, &_descriptorSet);
    g_App->getDevice()->Destroy(_uboBuffer);
    for (auto p : _textures) {
        g_App->getTextureManager()->removeTextureById(p->getId());
    }
}
MaterialDataBlock::MaterialDataBlock(MaterialDataBlock const &other) {
    _name = other._name;
    _uboMaterial = other._uboMaterial;
    _textures = other._textures;
}
MaterialDataBlock &MaterialDataBlock::operator=(MaterialDataBlock const &other) {
    _name = other._name;
    _uboMaterial = other._uboMaterial;
    _textures = other._textures;
    return *this;
}
void MaterialDataBlock::prepare() {
    //=============
    _uboBuffer = g_App->getDevice()->Create(
        Shit::BufferCreateInfo{
            {},
            sizeof(UBO),
            Shit::BufferUsageFlagBits::UNIFORM_BUFFER_BIT | Shit::BufferUsageFlagBits::TRANSFER_DST_BIT,
            Shit::MemoryPropertyFlagBits::HOST_VISIBLE_BIT | Shit::MemoryPropertyFlagBits::HOST_COHERENT_BIT},
        &_uboMaterial);

    //=====================
    auto descriptorSetLayout = g_App->getMaterialDescriptorSetLayout();

    _textures.resize(descriptorSetLayout->GetCreateInfoPtr()->pDescriptorSetLayoutBindings[0].descriptorCount,
                     g_App->getTextureManager()->getWhiteTexture());

    g_App->getDescriptorPool()->Allocate(Shit::DescriptorSetAllocateInfo{1, &descriptorSetLayout}, &_descriptorSet);

    Shit::DescriptorBufferInfo bufferInfo{_uboBuffer, 0, sizeof(UBO)};

    auto sampler = g_App->getSampler("linear");

    std::vector<Shit::DescriptorImageInfo> imageInfos;
    for (auto e : _textures) {
        e->prepare();
        imageInfos.emplace_back(sampler, e->getImageView(), Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL);
    }
    Shit::WriteDescriptorSet writeSets[]{
        {_descriptorSet, 1, 0, 1, Shit::DescriptorType::UNIFORM_BUFFER, 0, &bufferInfo},
        {_descriptorSet, 0, 0, (uint32_t)imageInfos.size(), Shit::DescriptorType::COMBINED_IMAGE_SAMPLER,
         imageInfos.data()},
    };
    g_App->getDevice()->UpdateDescriptorSets(writeSets);
}
void MaterialDataBlock::update() {
    void *data;
    _uboBuffer->MapMemory(0, sizeof(UBO), &data);
    memcpy(data, &_uboMaterial, sizeof(UBO));
    _uboBuffer->UnMapMemory();
}

void MaterialDataBlock::setTexture(int &index, std::string_view imagePath, bool isSRGB) {
    ParameterMap params;
    if (isSRGB)
        params.emplace("srgb", "1");
    else
        params.emplace("srgb", "0");
    if ((g_App->getRendererVersion() & Shit::RendererVersion::TypeBitmask) == Shit::RendererVersion::GL)
        params.emplace("flipy", "1");

    auto pImageSrc = static_cast<Image *>(
        g_App->getImageManager()->createOrRetrieve(imagePath, ResourceManager::DEFAULT_LOADER_NAME, params));
    pImageSrc->load();
    auto texture = g_App->getTextureManager()->createOrRetrieveTexture(pImageSrc);

    if (index == -1) {
        index = _textures.size();
        _textures.emplace_back(texture);
    } else {
        g_App->getTextureManager()->removeTextureById(_textures[index]->getId());
        _textures[index] = texture;
    }
}
void MaterialDataBlock::setAmbientTex(std::string_view imagePath, bool isSRGB) {
    setTexture(_uboMaterial.ambient_tex_index, imagePath, isSRGB);
}
void MaterialDataBlock::setDiffuseTex(std::string_view imagePath, bool isSRGB) {
    setTexture(_uboMaterial.diffuse_tex_index, imagePath, isSRGB);
}
void MaterialDataBlock::setSpecularTex(std::string_view imagePath, bool isSRGB) {
    setTexture(_uboMaterial.specular_tex_index, imagePath, isSRGB);
}
void MaterialDataBlock::setSpecularHighlightTex(std::string_view imagePath, bool isSRGB) {
    setTexture(_uboMaterial.specular_highlight_tex_index, imagePath, isSRGB);
}
void MaterialDataBlock::setBumpTex(std::string_view imagePath, bool isSRGB) {
    setTexture(_uboMaterial.bump_tex_index, imagePath, isSRGB);
}
void MaterialDataBlock::setDisplacementTex(std::string_view imagePath, bool isSRGB) {
    setTexture(_uboMaterial.displacement_tex_index, imagePath, isSRGB);
}
void MaterialDataBlock::setAlphaTex(std::string_view imagePath, bool isSRGB) {
    setTexture(_uboMaterial.alpha_tex_index, imagePath, isSRGB);
}
void MaterialDataBlock::setRelectionTex(std::string_view imagePath, bool isSRGB) {
    setTexture(_uboMaterial.reflection_tex_index, imagePath, isSRGB);
}
void MaterialDataBlock::setEmissiveTex(std::string_view imagePath, bool isSRGB) {
    setTexture(_uboMaterial.emissive_tex_index, imagePath, isSRGB);
}
void MaterialDataBlock::setMetallicTex(std::string_view imagePath, bool isSRGB) {
    setTexture(_uboMaterial.metallic_tex_index, imagePath, isSRGB);
}
void MaterialDataBlock::setRoughnessTex(std::string_view imagePath, bool isSRGB) {
    setTexture(_uboMaterial.roughness_tex_index, imagePath, isSRGB);
}
void MaterialDataBlock::setSheenTex(std::string_view imagePath, bool isSRGB) {
    setTexture(_uboMaterial.sheen_tex_index, imagePath, isSRGB);
}
void MaterialDataBlock::setTextureOffset(float offsetx, float offsety, float offsetz) {
    _uboMaterial.offset[0] = offsetx;
    _uboMaterial.offset[1] = offsety;
    _uboMaterial.offset[2] = offsetz;
}
void MaterialDataBlock::setTextureScale(float scalex, float scaley, float scalez) {
    _uboMaterial.scale[0] = scalex;
    _uboMaterial.scale[1] = scaley;
    _uboMaterial.scale[2] = scalez;
}
void MaterialDataBlock::setTextureRotateAngle(float rad) { _uboMaterial.rotateAngle = rad; }
//=============================
MaterialDataBlockManager::MaterialDataBlockManager() {
    // create default datablock
    _defaultMaterialId = createMaterialDataBlock("default")->getId();
}
MaterialDataBlockManager::~MaterialDataBlockManager() {}
MaterialDataBlock *MaterialDataBlockManager::createMaterialDataBlock(std::string_view name) {
    MaterialDataBlock *ret;
    if (name.empty())
        ret = new MaterialDataBlock(this);
    else
        ret = new MaterialDataBlock(this, name);
    _materials.emplace(ret->getId(), std::unique_ptr<MaterialDataBlock>(ret));
    return ret;
}
MaterialDataBlock *MaterialDataBlockManager::createMaterialDataBlock(MaterialDataBlock const *other) {
    MaterialDataBlock *ret = new MaterialDataBlock(*other);
    _materials.emplace(ret->getId(), std::unique_ptr<MaterialDataBlock>(ret));
    return ret;
}