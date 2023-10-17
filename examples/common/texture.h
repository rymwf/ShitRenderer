#pragma once
#include "idObject.h"
#include "image.h"

// template <>
// struct std::hash<Shit::ImageViewCreateInfo>
//{
//	std::size_t operator()(Shit::ImageViewCreateInfo const &s) const
// noexcept
//	{
//		size_t h = std::hash<Shit::ImageViewType>{}(s.viewType);
//		hashCombine(h, std::hash<Shit::Format>{}(s.format));
//		hashCombine(h,
// std::hash<Shit::ComponentSwizzle>{}(s.components.r)); 		hashCombine(h,
// std::hash<Shit::ComponentSwizzle>{}(s.components.g)); 		hashCombine(h,
// std::hash<Shit::ComponentSwizzle>{}(s.components.b)); 		hashCombine(h,
// std::hash<Shit::ComponentSwizzle>{}(s.components.a)); 		hashCombine(h,
// std::hash<uint32_t>{}(s.subresourceRange.baseMipLevel)); 		hashCombine(h,
// std::hash<uint32_t>{}(s.subresourceRange.levelCount)); 		hashCombine(h,
// std::hash<uint32_t>{}(s.subresourceRange.baseArrayLayer)); 		hashCombine(h,
// std::hash<uint32_t>{}(s.subresourceRange.layerCount)); 		return h;
//	}
// };
// namespace Shit{
//	constexpr bool operator==(const ImageViewCreateInfo &lhs, const
// ImageViewCreateInfo &rhs) noexcept
//	{
//		return lhs.pImage == rhs.pImage &&
//			   lhs.viewType == rhs.viewType &&
//			   lhs.format == rhs.format &&
//			   lhs.components.r == rhs.components.r &&
//			   lhs.components.g == rhs.components.g &&
//			   lhs.components.b == rhs.components.b &&
//			   lhs.components.a == rhs.components.a &&
//			   lhs.subresourceRange.baseArrayLayer ==
// rhs.subresourceRange.baseArrayLayer && 			   lhs.subresourceRange.baseMipLevel ==
// rhs.subresourceRange.baseMipLevel && 			   lhs.subresourceRange.layerCount ==
// rhs.subresourceRange.layerCount && 			   lhs.subresourceRange.levelCount ==
// rhs.subresourceRange.levelCount;
//	}
// }
class Texture : public IdObject<Texture> {
    std::string _name;
    Image const *_imageSrc{};

    Shit::ImageLayout _imageLayout;
    Shit::ImageUsageFlagBits _usage;

    Shit::Image *_image{};
    Shit::ImageView *_imageView;

    // std::unordered_map<Shit::ImageViewCreateInfo, Shit::ImageView *>
    // _imageViews{};

public:
    enum class Status {
        CREATED,
        PREPARED,
    };

    Texture(Image const *image,
            Shit::ImageUsageFlagBits usage = Shit::ImageUsageFlagBits::TRANSFER_DST_BIT |
                                             Shit::ImageUsageFlagBits::SAMPLED_BIT,
            Shit::ImageLayout imageLayout = Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL);
    Texture(Shit::ImageCreateInfo const &imageCI, Shit::ImageViewType imageViewType, const void *data);
    ~Texture();

    void prepare();

    constexpr Image const *getImageSrc() const { return _imageSrc; }
    constexpr Shit::Image *getImage() const { return _image; }
    constexpr Shit::ImageView *getImageView() const { return _imageView; }
    // Shit::ImageView *getImageView(
    //	Shit::ImageViewType viewType,
    //	const Shit::ImageSubresourceRange &subresourceRange,
    //	Shit::Format format,
    //	const Shit::ComponentMapping &components = {});

private:
    Status _status{Status::CREATED};
};

class TextureManager {
    std::unordered_map<uint32_t, std::unique_ptr<Texture>> _textures;
    std::unordered_map<std::string, Texture *> _texturesNameMap;

    std::unique_ptr<Texture> _whiteTexture;
    std::unique_ptr<Texture> _blackTexture;

public:
    TextureManager();
    Texture *createTexture(Image const *image, Shit::ImageUsageFlagBits usage = Shit::ImageUsageFlagBits::SAMPLED_BIT,
                           Shit::ImageLayout imageLayout = Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL);
    Texture *createOrRetrieveTexture(Image const *image,
                                     Shit::ImageUsageFlagBits usage = Shit::ImageUsageFlagBits::SAMPLED_BIT,
                                     Shit::ImageLayout imageLayout = Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL);
    Texture *getTexture(std::string_view imageName);
    Texture *createTexture(Shit::ImageCreateInfo const &imageCI, Shit::ImageViewType imageViewType, const void *data);

    void removeTextureById(uint32_t id) { _textures.erase(id); }

    Texture *getTextureById(uint32_t id) const { return _textures.at(id).get(); }
    Texture *getWhiteTexture() const { return _whiteTexture.get(); }
    Texture *getBlackTexture() const { return _blackTexture.get(); }
};