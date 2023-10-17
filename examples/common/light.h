#pragma once
#include "component.h"

#define DESCRIPTOR_LIGHT 3

enum LightType {
    LIGHT_DIRECTIONAL = 1,
    LIGHT_SPHERE,
    LIGHT_SPOT,
    LIGHT_TUBE,
    LIGHT_DISK_SINGLE_FACE,
    LIGHT_QUAD_SINGLE_FACE,
    LIGHT_POLYGON_SINGLE_FACE,  // vertex num <=10
};
struct LightSSBO {
    glm::mat4 transformMatrix{1.};
    glm::vec3 color{1};
    int lightType{1};  // 1 directional, 2 sphere, 3 spot, 4 aera, disable when <0
    glm::vec3 radius{1, 1, 1};
    float rmax{100.};

    float radiance{1};
    float cosThetaU;  // spot light angle umbra larger than penumbra
    float cosThetaP;  // spot light angle penumbra
    int vertexNum;

    glm::vec4 vertices[4];  // last component is cascadenum
                            // glm::mat4 PV[CASCADE_NUM];
};
class Light : public Component {
public:
    Light(GameObject *parent, LightType lightType = LIGHT_DIRECTIONAL);
    ~Light();

    constexpr Shit::DescriptorSet *getDescriptorSet(uint32_t frameIndex = 0) const {
        return _descriptorSetsInfo.at(frameIndex).set;
    }

    constexpr LightSSBO *getSSBOData() { return &_ssboLight; }

    // eight vertices
    void setCascadeFrustumVertices(std::span<glm::vec3 const> frustumVertices);
    void generateCubeLightPVs();
    constexpr uint32_t getFrustumCount() const { return _lightPVs.size(); }

    void setSpotLight(float thetaU, float thetaP);

    constexpr float getSpotUmbraAngle() const { return _spotThetaU; }
    constexpr float getSpotPenumbraAngle() const { return _spotThetaP; }

private:
    struct DescriptorSetInfo {
        Shit::Buffer *buffer;
        Shit::DescriptorSet *set;
    };
    std::vector<DescriptorSetInfo> _descriptorSetsInfo;

    LightSSBO _ssboLight{};

    float _spotThetaU;  // umbra
    float _spotThetaP;  // penumbra

    std::vector<glm::vec3> _cascadeFrustumVertices;
    std::vector<glm::mat4> _lightPVs;

    void prepareImpl() override;
    void updateImpl() override;
    void updateLightPVs();

    void updateUBOBuffer();

    void transformListener(Component const *) { needUpdate(); }
};
