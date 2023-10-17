#pragma once
#include "resource.h"

class ShaderSpvLoader : public ResourceLoader {
    Shit::Device *_device;

public:
    ShaderSpvLoader(Shit::Device *device);
    void load(Resource *resource) override;
    void unload(Resource *resource);
};

class Shader : public Resource {
    // std::string _srcCode;
    // Shit::ShadingLanguage _shadingLanguage;
    Shit::ShaderStageFlagBits _stage;
    Shit::Shader *_shader;
    std::string _entryName = "main";

public:
    Shader(std::string_view name, std::string_view path, ResourceLoader *loader, Shit::ShaderStageFlagBits stage);
    ~Shader() {}

    // constexpr void _setSrcCode(std::string_view code) { _srcCode = code; }
    // constexpr void _setShadingLanguage(Shit::ShadingLanguage shadingLanguage) {
    // _shadingLanguage = shadingLanguage; }
    constexpr void _setHandle(Shit::Shader *handle) { _shader = handle; }
    constexpr void _setEntryName(std::string_view name) { _entryName = name; }

    constexpr Shit::Shader *getHandle() const { return _shader; }
    constexpr Shit::ShaderStageFlagBits getStage() const { return _stage; }
    constexpr std::string_view getEntryName() const { return _entryName; }
};

class ShaderManager : public ResourceManager {
    Resource *createImpl(std::string_view path, ResourceLoader *loader, ParameterMap const &parameters) override;

public:
    ShaderManager();
    ~ShaderManager() {}
};