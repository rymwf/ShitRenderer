#pragma once
#include <fstream>

#include "idObject.h"
#include "prerequisites.h"
#include "singleton.h"

class Resource;
class ResourceLoader {
public:
    virtual ~ResourceLoader() {}
    virtual void load(Resource *resource) = 0;
    virtual void unload(Resource *resource) {}
};

using ParameterMap = std::unordered_map<std::string, std::string>;

class Resource : public IdObject<Resource> {
public:
    enum class Status {
        UNLOADED,
        LOADED,
    };

    Resource(std::string_view name, std::string_view path, ResourceLoader *loader)
        : _name(name), _path(path), _loader(loader) {}
    virtual ~Resource() {}
    void load() {
        if (_status == Status::UNLOADED) {
            if (_loader) _loader->load(this);
            loadImpl();
            _status = Status::LOADED;
            updateSignal();
        }
    }
    void unload() {
        if (_status == Status::LOADED) {
            if (_loader) _loader->unload(this);
            unloadImpl();
            _status = Status::LOADED;
            updateSignal();
        }
    }

    std::iostream *open(std::ios_base::openmode openmode = std::ios::in | std::ios::binary) {
        _fs = std::make_unique<std::fstream>(_path, openmode);
        if (_fs->is_open()) {
            return _fs.get();
        }
        THROW("failed to open resource ", _name, "path: ", _path)
        return nullptr;
    }
    void close() { _fs->close(); }
    constexpr Status getStatus() const { return _status; }
    constexpr std::string_view getName() const { return _name; }
    constexpr std::string_view getPath() const { return _path; }

    Shit::Signal<void()> updateSignal;

protected:
    std::string _name;
    std::string _path;
    Status _status{Status::UNLOADED};

    ResourceLoader *_loader;

    std::unique_ptr<std::fstream> _fs;

    virtual void loadImpl() {}
    virtual void unloadImpl() {}
};

class ResourceManager {
protected:
    std::unordered_map<uint32_t, std::unique_ptr<Resource>> _resources;
    std::unordered_map<std::string, Resource *> _resourcesNameMap;
    std::unordered_map<std::string, std::unique_ptr<ResourceLoader>> _loaders;

    bool addResource(Resource *res) {
        _resources.emplace(res->getId(), std::unique_ptr<Resource>(res));
        _resourcesNameMap.emplace(res->getName(), res);
        return true;
    }

    virtual Resource *createImpl(std::string_view path, ResourceLoader *loader, ParameterMap const &parameters) = 0;

public:
    inline static const char *DEFAULT_LOADER_NAME = "default";

    ResourceManager() {}

    Resource *create(std::string_view path, std::string_view loaderName = DEFAULT_LOADER_NAME,
                     ParameterMap const &parameters = {}) {
        // if (_resources.contains(std::string(name)))
        //{
        //	LOG("resource ", name, " already exists")
        //	return nullptr;
        // }
        auto loader = getResourceLoader(loaderName);
        if (!loader) {
            LOG("loader ", loaderName, " does not exist")
            return nullptr;
        }
        auto ret = createImpl(path, loader, parameters);
        addResource(ret);
        return ret;
    }
    Resource *createOrRetrieve(std::string_view path, std::string_view loaderName = DEFAULT_LOADER_NAME,
                               ParameterMap const &parameters = {}) {
        if (auto ret = getResourceByName(path)) return ret;
        return create(path, loaderName, parameters);
    }
    Resource *getResourceByName(std::string_view name) const {
        auto it = _resourcesNameMap.find(std::string(name));
        if (it != _resourcesNameMap.end()) return it->second;
        return nullptr;
    }
    Resource *getResourceById(uint32_t id) const { return _resources.at(id).get(); }
    void removeResourceById(uint32_t id) { _resources.erase(id); }

    ResourceLoader *getResourceLoader(std::string_view name) const {
        auto it = _loaders.find(std::string(name));
        if (it != _loaders.cend()) return it->second.get();
        return nullptr;
    }

    // template <typename Loader_T, typename
    // std::enable_if_t<std::is_base_of_v<ResourceLoader, Loader_T>, bool> = true>
    // template <typename Loader_T>
    // void registerResourceLoader(std::string_view loaderName)
    //{
    //	_loaders.emplace(loaderName, std::make_unique<Loader_T>());
    // }

    template <typename Loader_T, typename... Args>
    void registerResourceLoader(std::string_view loaderName, Args... args) {
        _loaders.emplace(loaderName, std::make_unique<Loader_T>(args...));
    }
};
