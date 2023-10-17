#include "shader.h"
ShaderSpvLoader::ShaderSpvLoader(Shit::Device *device) : _device(device) {}
void ShaderSpvLoader ::load(Resource *resource) {
    auto shaderRes = static_cast<Shader *>(resource);

    auto iStream = shaderRes->open();
    iStream->seekg(0, std::ios::end);
    auto len = iStream->tellg();
    iStream->seekg(0, std::ios::beg);

    std::string code;
    code.resize(len);
    iStream->read(code.data(), len);

    shaderRes->_setHandle(
        _device->Create(Shit::ShaderCreateInfo{Shit::ShadingLanguage::SPIRV, code.size(), code.data()}));

    shaderRes->close();
}
void ShaderSpvLoader::unload(Resource *resource) { _device->Destroy(static_cast<Shader *>(resource)->getHandle()); }
//=========================
Shader::Shader(std::string_view name, std::string_view path, ResourceLoader *loader, Shit::ShaderStageFlagBits stage)
    : Resource(name, path, loader), _stage(stage) {}
ShaderManager::ShaderManager() {}
Resource *ShaderManager::createImpl(std::string_view path, ResourceLoader *loader, ParameterMap const &parameters) {
    auto str = parameters.at("stage");
    Shit::ShaderStageFlagBits stage;
    if (str == "VERT")
        stage = Shit::ShaderStageFlagBits::VERTEX_BIT;
    else if (str == "FRAG")
        stage = Shit::ShaderStageFlagBits::FRAGMENT_BIT;
    else if (str == "GEOM")
        stage = Shit::ShaderStageFlagBits::GEOMETRY_BIT;
    else if (str == "TESC")
        stage = Shit::ShaderStageFlagBits::TESSELLATION_CONTROL_BIT;
    else if (str == "TESE")
        stage = Shit::ShaderStageFlagBits::TESSELLATION_EVALUATION_BIT;
    else if (str == "COMP")
        stage = Shit::ShaderStageFlagBits::COMPUTE_BIT;
    return new Shader(path, path, loader, stage);
}