#pragma once
#include "resource.h"

struct ImageDescription {
    bool hdr{false};
    int width;
    int height;
    Shit::Format format;
    unsigned char *data;
    size_t size() { return width * height * Shit::GetFormatSize(format); }
};

/**
 * @brief
 *
 * @param path
 * @param imageType  PNG, JPG
 * @param width
 * @param height
 * @param componentNum
 * @param data
 * @return true
 * @return false
 */
bool saveImage(std::string_view path, std::string_view imageType, int width, int height, int componentNum, void *data);

class ImageLoaderSTB : public ResourceLoader {
public:
    // ImageDescription getInfo(std::iostream const &ss) override;
    // ImageDescription load(std::istream *inStream) override;
    // void unload(void *data) override;
    void load(Resource *resource) override;
    void unload(Resource *resource) override;
};

class ImageManager;

// enum class ColorSpace
//{
//	LINEAR,
//	SRGB_NONLINEAR,
// };

class Image : public Resource {
    ImageDescription _desc;
    // ColorSpace _colorSpace;
    bool _isSRGB;
    bool _flipY{false};
    int _requestComponentNum{4};

public:
    Image(std::string_view name, std::string_view path, bool isSRGB, bool flipY, int requestComponentNum,
          ResourceLoader *loader);

    Image(std::string_view name, ImageDescription const &desc, bool isSRGB = false, bool flipY = false,
          std::string_view path = {});

    constexpr void flipY(bool val) { _flipY = val; }
    constexpr bool isflipY() const { return _flipY; }
    constexpr bool isSRGB() const { return _isSRGB; }
    constexpr int getRequestComponentNum() const { return _requestComponentNum; }
    constexpr void _setDescription(ImageDescription const &desc) { _desc = desc; }
    // constexpr void
    constexpr ImageDescription const &getDescription() const { return _desc; }
};

class ImageManager : public ResourceManager {
    Resource *createImpl(std::string_view path, ResourceLoader *loader, ParameterMap const &parameters) override;

public:
    ImageManager();

    Image *createImage(std::string_view name, ImageDescription const &desc, bool isSRGB, bool flipY,
                       std::string_view path);
};
