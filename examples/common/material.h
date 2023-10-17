#pragma once
#include "idObject.h"
#include "prerequisites.h"
#include "texture.h"

/*
https://www.loc.gov/preservation/digital/formats/fdd/fdd000508.shtml

Ka: specifies ambient color, to account for light that is scattered about the
entire scene [see Wikipedia entry for Phong Reflection Model] using values
between 0 and 1 for the RGB components. Kd: specifies diffuse color, which
typically contributes most of the color to an object [see Wikipedia entry for
Diffuse Reflection]. In this example, Kd represents a grey color, which will get
modified by a colored texture map specified in the map_Kd statement Ks:
specifies specular color, the color seen where the surface is shiny and
mirror-like [see Wikipedia entry for Specular Reflection]. Ns: defines the focus
of specular highlights in the material. Ns values normally range from 0 to 1000,
with a high value resulting in a tight, concentrated highlight. Ni: defines the
optical density (aka index of refraction) in the current material. The values
can range from 0.001 to 10. A value of 1.0 means that light does not bend as it
passes through an object. d: specifies a factor for dissolve, how much this
material dissolves into the background. A factor of 1.0 is fully opaque. A
factor of 0.0 is completely transparent. illum: specifies an illumination model,
using a numeric value. See Notes below for more detail on the illum keyword. The
value 0 represents the simplest illumination model, relying on the Kd for the
material modified by a texture map specified in a map_Kd statement if present.
The compilers of this resource believe that the choice of illumination model is
irrelevant for 3D printing use and is ignored on import by some software
applications. For example, the MTL Loader in the threejs Javascript library
appears to ignore illum statements. Comments welcome. map_Kd: specifies a color
texture file to be applied to the diffuse reflectivity of the material. During
rendering, map_Kd values are multiplied by the Kd values to derive the RGB
components.

- Ka - material ambient is multiplied by the texture value
 - Kd - material diffuse is multiplied by the texture value
 - Ks - material specular is multiplied by the texture value
  - Ns - material specular exponent is multiplied by the texture value
 - d - material dissolve is multiplied by the texture value
 - decal - uses a scalar value to deform the surface of an object to create
surface roughness


Pr/map_Pr     # roughness
Pm/map_Pm     # metallic
Ps/map_Ps     # sheen
Pc            # clearcoat thickness
Pcr           # clearcoat roughness
Ke/map_Ke     # emissive
aniso         # anisotropy
anisor        # anisotropy rotation
norm          # normal map (RGB components represent XYZ components of the
surface normal)
*/
class MaterialDataBlockManager;

class MaterialDataBlock : public IdObject<MaterialDataBlock> {
    MaterialDataBlockManager *_creator;
    std::string _name;
    struct UBO {
        float ambient[3]{};
        float shininess{200};
        float diffuse[3]{};
        float ior{1};  // index of refraction
        float specular[3]{};
        float dissolve{1};  // 1 == opaque; 0 == fully transparent
        float transmittance[3]{};
        float roughness{0};
        float emission[3]{};
        float metallic{1};

        float sheen{1};                // [0, 1] default 0
        float clearcoat_thickness{0};  // [0, 1] default 0
        float clearcoat_roughness{0};  // [0, 1] default 0
        float anisotropy{0};           // aniso. [0, 1] default 0

        float anisotropy_rotation{0};  // anisor. [0, 1] default 0

        int ambient_tex_index{-1};             // map_Ka
        int diffuse_tex_index{-1};             // map_Kd
        int specular_tex_index{-1};            // map_Ks
        int specular_highlight_tex_index{-1};  // map_Ns
        //
        int bump_tex_index{-1};          // map_bump, map_Bump, bump
        int displacement_tex_index{-1};  // disp
        int alpha_tex_index{-1};         // map_d
        int reflection_tex_index{-1};    // refl
        //
        int roughness_tex_index{-1};  // map_Pr
        int metallic_tex_index{-1};   // map_Pm
        int sheen_tex_index{-1};      // map_Ps
        int emissive_tex_index{-1};   // map_Ke
        // int normal_tex_index{-1};	 //norm. For normal mapping, use bump
        // instead

        //
        float padding[3];
        float offset[3]{};
        float rotateAngle{0};
        float scale[3]{1, 1, 1};
        int shadingModel{0};

    } _uboMaterial{};

    std::vector<Texture *> _textures;

    Shit::DescriptorSet *_descriptorSet;
    Shit::Buffer *_uboBuffer;

    void setTexture(int &index, std::string_view imagePath, bool isSRGB);

public:
    MaterialDataBlock(MaterialDataBlockManager *creator) : _creator(creator) {}
    MaterialDataBlock(MaterialDataBlockManager *creator, std::string_view name) : _creator(creator), _name(name) {}
    MaterialDataBlock(MaterialDataBlock const &other);
    MaterialDataBlock &operator=(MaterialDataBlock const &other);

    ~MaterialDataBlock();

    UBO *getUBOData() { return &_uboMaterial; }

    constexpr Shit::DescriptorSet *getDescriptorSet() const { return _descriptorSet; }

    void prepare();
    void update();
    void setAmbientTex(std::string_view imagePath, bool isSRGB = false);
    void setDiffuseTex(std::string_view imagePath, bool isSRGB = true);
    void setSpecularTex(std::string_view imagePath, bool isSRGB = false);
    void setSpecularHighlightTex(std::string_view imagePath, bool isSRGB = false);
    void setBumpTex(std::string_view imagePath, bool isSRGB = false);
    void setDisplacementTex(std::string_view imagePath, bool isSRGB = false);
    void setAlphaTex(std::string_view imagePath, bool isSRGB = false);
    void setRelectionTex(std::string_view imagePath, bool isSRGB = false);
    void setEmissiveTex(std::string_view imagePath, bool isSRGB = true);
    void setMetallicTex(std::string_view imagePath, bool isSRGB = false);
    void setRoughnessTex(std::string_view imagePath, bool isSRGB = false);
    void setSheenTex(std::string_view imagePath, bool isSRGB = false);

    void setTextureOffset(float offsetx, float offsety, float offsetz);
    void setTextureScale(float scalex, float scaley, float scalez);
    void setTextureRotateAngle(float rad);
};

class MaterialDataBlockManager {
    std::unordered_map<uint32_t, std::unique_ptr<MaterialDataBlock>> _materials;
    uint32_t _defaultMaterialId{};

    void createDescriptorSetLayout();

public:
    MaterialDataBlockManager();
    ~MaterialDataBlockManager();

    MaterialDataBlock *createMaterialDataBlock(std::string_view name = {});
    MaterialDataBlock *createMaterialDataBlock(MaterialDataBlock const *other);

    bool containsMaterialDataBlock(uint32_t id) { return _materials.contains(id); }

    MaterialDataBlock *getMaterialDataBlockById() const { return _materials.at(_defaultMaterialId).get(); };
    MaterialDataBlock *getMaterialDataBlockById(uint32_t id) const { return _materials.at(id).get(); }
    void removeMaterialDataBlockById(uint32_t id) {
        if (id != _defaultMaterialId) _materials.erase(id);
    }
};