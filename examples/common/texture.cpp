#include "texture.h"

#include "appbase.h"

Texture::Texture(Image const *image, Shit::ImageUsageFlagBits usage, Shit::ImageLayout imageLayout)
    : _imageSrc(image), _imageLayout(imageLayout), _usage(usage) {}
Texture::Texture(Shit::ImageCreateInfo const &imageCI, Shit::ImageViewType imageViewType, const void *data) {
    _image = g_App->getDevice()->Create(imageCI, Shit::ImageAspectFlagBits::COLOR_BIT, data);
    _image->GenerateMipmap(Shit::Filter::LINEAR, imageCI.initialLayout, imageCI.initialLayout);
    _imageView = g_App->getDevice()->Create(
        Shit::ImageViewCreateInfo{_image,
                                  imageViewType,
                                  _image->GetCreateInfoPtr()->format,
                                  {},
                                  {Shit::ImageAspectFlagBits::COLOR_BIT, 0, _image->GetCreateInfoPtr()->mipLevels, 0,
                                   _image->GetCreateInfoPtr()->arrayLayers}});
}
Texture::~Texture() {
    g_App->getDevice()->Destroy(_image);
    g_App->getDevice()->Destroy(_imageView);
    // for (auto &&e : _imageViews)
    //	g_App->getDevice()->Destroy(e.second);
    if (_imageSrc) {
        g_App->getImageManager()->removeResourceById(_imageSrc->getId());
    }
}
void Texture::prepare() {
    if (_status != Status::CREATED || !_imageSrc) return;
    _status = Status::PREPARED;
    const_cast<Image *>(_imageSrc)->load();

    auto &&desc = _imageSrc->getDescription();

    auto imageType = Shit::ImageType::TYPE_2D;
    auto imageViewType = Shit::ImageViewType::TYPE_2D;
    if (desc.height == 1) {
        imageType = Shit::ImageType::TYPE_1D;
        imageViewType = Shit::ImageViewType::TYPE_1D;
    }

    _image = g_App->getDevice()->Create(Shit::ImageCreateInfo{{},
                                                              imageType,
                                                              desc.format,
                                                              {(uint32_t)desc.width, (uint32_t)desc.height, 1u},
                                                              0,
                                                              1,
                                                              Shit::SampleCountFlagBits::BIT_1,
                                                              Shit::ImageTiling::OPTIMAL,
                                                              _usage,
                                                              Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT,
                                                              _imageLayout},
                                        Shit::ImageAspectFlagBits::COLOR_BIT, desc.data);
    _image->GenerateMipmap(Shit::Filter::LINEAR, _imageLayout, _imageLayout);

    _imageView = g_App->getDevice()->Create(
        Shit::ImageViewCreateInfo{_image,
                                  imageViewType,
                                  _image->GetCreateInfoPtr()->format,
                                  {},
                                  {Shit::ImageAspectFlagBits::COLOR_BIT, 0, _image->GetCreateInfoPtr()->mipLevels, 0,
                                   _image->GetCreateInfoPtr()->arrayLayers}});
}
// Shit::ImageView *Texture::getImageView(
//	Shit::ImageViewType viewType,
//	const Shit::ImageSubresourceRange &subresourceRange,
//	Shit::Format format,
//	const Shit::ComponentMapping &components)
//{
//	auto imageViewCreateInfo =
//		Shit::ImageViewCreateInfo{
//			_image,
//			viewType,
//			format,
//			components,
//			subresourceRange};

//	if (_imageViews.contains(imageViewCreateInfo))
//		return _imageViews[imageViewCreateInfo];

//	return _imageViews.emplace(
//						  imageViewCreateInfo,
// g_App->getDevice()->Create(imageViewCreateInfo)) 		.first->second;
//}

TextureManager::TextureManager() {
    static uint32_t whiteColor = 0xFFFFFFFF;
    static uint32_t blackColor = 0;
    _whiteTexture = std::make_unique<Texture>(
        Shit::ImageCreateInfo{{},
                              Shit::ImageType::TYPE_2D,
                              Shit::Format::R8G8B8A8_SRGB,
                              {1u, 1u, 1u},
                              0,
                              1,
                              Shit::SampleCountFlagBits::BIT_1,
                              Shit::ImageTiling::OPTIMAL,
                              Shit::ImageUsageFlagBits::SAMPLED_BIT | Shit::ImageUsageFlagBits::TRANSFER_DST_BIT,
                              Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT,
                              Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL},
        Shit::ImageViewType::TYPE_2D, (void *)&whiteColor);

    _blackTexture = std::make_unique<Texture>(
        Shit::ImageCreateInfo{{},
                              Shit::ImageType::TYPE_2D,
                              Shit::Format::R8G8B8A8_SRGB,
                              {1u, 1u, 1u},
                              0,
                              1,
                              Shit::SampleCountFlagBits::BIT_1,
                              Shit::ImageTiling::OPTIMAL,
                              Shit::ImageUsageFlagBits::SAMPLED_BIT | Shit::ImageUsageFlagBits::TRANSFER_DST_BIT,
                              Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT,
                              Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL},
        Shit::ImageViewType::TYPE_2D, (void *)&blackColor);
}
Texture *TextureManager::createTexture(Image const *image, Shit::ImageUsageFlagBits usage,
                                       Shit::ImageLayout imageLayout) {
    auto ret = new Texture(image, usage, imageLayout);
    _textures.emplace(ret->getId(), std::unique_ptr<Texture>(ret));
    _texturesNameMap.emplace(image->getName(), ret);
    return ret;
}
Texture *TextureManager::createOrRetrieveTexture(Image const *image, Shit::ImageUsageFlagBits usage,
                                                 Shit::ImageLayout imageLayout) {
    if (auto ret = getTexture(image->getName())) return ret;
    return createTexture(image, usage, imageLayout);
}
Texture *TextureManager::getTexture(std::string_view imageName) {
    auto it = _texturesNameMap.find(std::string(imageName));
    if (it != _texturesNameMap.end()) return it->second;
    return nullptr;
}
Texture *TextureManager::createTexture(Shit::ImageCreateInfo const &imageCI, Shit::ImageViewType imageViewType,
                                       const void *data) {
    Texture *ret = new Texture(imageCI, imageViewType, data);
    _textures.emplace(ret->getId(), std::unique_ptr<Texture>(ret));
    return ret;
}