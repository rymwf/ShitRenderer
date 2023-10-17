#include "image.h"

#include "appbase.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_write.h>

bool saveImage(std::string_view path, std::string_view imageType, int width, int height, int componentNum, void *data) {
    if (imageType == "PNG")
        stbi_write_png(path.data(), width, height, componentNum, data, componentNum);
    else if (imageType == "BMP")
        stbi_write_bmp(path.data(), width, height, componentNum, data);
    else if (imageType == "TGA")
        stbi_write_tga(path.data(), width, height, componentNum, data);
    else if (imageType == "JPG")
        stbi_write_jpg(path.data(), width, height, componentNum, data, 90);
    else if (imageType == "HDR")
        stbi_write_hdr(path.data(), width, height, componentNum, (float *)data);
    throw std::runtime_error("failed to save image");
    return false;
}

// ImageDescription ImageLoaderSTB::load(std::istream *inStream)
void ImageLoaderSTB::load(Resource *resource) {
    auto inStream = resource->open();
    if (!inStream) return;

    inStream->seekg(0, std::ios::end);
    size_t len = inStream->tellg();
    inStream->seekg(0, std::ios::beg);
    std::vector<unsigned char> data(len);
    inStream->read((char *)data.data(), len);

    auto pImage = static_cast<Image *>(resource);
    auto requestComponentNum = pImage->getRequestComponentNum();

    ImageDescription ret;
    int componentNum;

    static bool flipY = (g_App->getRendererVersion() & Shit::RendererVersion::TypeBitmask) == Shit::RendererVersion::GL;
    stbi_set_flip_vertically_on_load(flipY);
    if (stbi_is_hdr(pImage->getPath().data())) {
        ret.data = (unsigned char *)stbi_loadf(pImage->getPath().data(), &ret.width, &ret.height, &componentNum,
                                               requestComponentNum);
        assert(ret.data);
        if (!ret.data) THROW("failed to load image", pImage->getPath())
        ret.hdr = true;
        switch (requestComponentNum) {
            case 1:
                ret.format = Shit::Format::R32_SFLOAT;  // linear space
                break;
            case 2:
                ret.format = Shit::Format::R32G32_SFLOAT;  // linear space
                break;
            case 3:
                ret.format = Shit::Format::R32G32B32_SFLOAT;  // linear space
                break;
            case 4:
            default:
                ret.format = Shit::Format::R32G32B32A32_SFLOAT;  // linear space
                break;
        }
    } else {
        assert(ret.data);
        ret.data = (unsigned char *)stbi_load(pImage->getPath().data(), &ret.width, &ret.height, &componentNum,
                                              requestComponentNum);

        if (!ret.data) THROW("failed to load image", pImage->getPath())
        ret.hdr = false;
        if (static_cast<Image *>(resource)->isSRGB()) {
            switch (requestComponentNum) {
                case 1:
                    ret.format = Shit::Format::R8_SRGB;
                    break;
                case 2:
                    ret.format = Shit::Format::R8G8_SRGB;
                    break;
                case 3:
                    ret.format = Shit::Format::R8G8B8_SRGB;
                    break;
                case 4:
                default:
                    ret.format = Shit::Format::R8G8B8A8_SRGB;
                    break;
            }
        } else {
            switch (requestComponentNum) {
                case 1:
                    ret.format = Shit::Format::R8_UNORM;
                    break;
                case 2:
                    ret.format = Shit::Format::R8G8_UNORM;
                    break;
                case 3:
                    ret.format = Shit::Format::R8G8B8_UNORM;
                    break;
                case 4:
                default:
                    ret.format = Shit::Format::R8G8B8A8_UNORM;
                    break;
            }
        }
    }
    static_cast<Image *>(resource)->_setDescription(ret);
}
void ImageLoaderSTB::unload(Resource *resource) {
    stbi_image_free(static_cast<Image *>(resource)->getDescription().data);
}

//========================
Image::Image(std::string_view name, std::string_view path, bool isSRGB, bool flipY, int requestComponentNum,
             ResourceLoader *loader)
    : Resource(name, path, loader), _isSRGB(isSRGB), _flipY(flipY), _requestComponentNum(requestComponentNum) {}
Image::Image(std::string_view name, ImageDescription const &desc, bool isSRGB, bool flipY, std::string_view path)
    : Resource(name, path, nullptr), _desc(desc), _isSRGB(isSRGB), _flipY(flipY) {
    _status = Status::LOADED;
}
//==================================
ImageManager::ImageManager() { registerResourceLoader<ImageLoaderSTB>(DEFAULT_LOADER_NAME); }
Resource *ImageManager::createImpl(std::string_view path, ResourceLoader *loader, ParameterMap const &parameters) {
    bool isSRGB = 1;
    bool flipY = false;
    int requestComponentNum = 4;

    static const char *keys[]{"srgb", "flipy", "request_component_num"};
    auto it = parameters.find(keys[0]);
    if (it != parameters.end()) isSRGB = (it->second == "1");

    it = parameters.find(keys[1]);
    if (it != parameters.end()) flipY = (it->second == "1");

    it = parameters.find(keys[2]);
    if (it != parameters.end()) requestComponentNum = std::atoi(it->second.c_str());

    return new Image(path, path, isSRGB, flipY, requestComponentNum, loader);
}
Image *ImageManager::createImage(std::string_view name, ImageDescription const &desc, bool isSRGB, bool flipY,
                                 std::string_view path) {
    auto a = new Image(name, desc, isSRGB, flipY, path);
    addResource(a);
    return a;
}