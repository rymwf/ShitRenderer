#include "light.h"

#include "appbase.h"

Light::Light(GameObject *parent, LightType lightType) : Component(parent) {
    _ssboLight.lightType = lightType;
    switch (_ssboLight.lightType) {
        case LIGHT_TUBE:
            _ssboLight.vertexNum = 2;
            break;
        case LIGHT_QUAD_SINGLE_FACE:
            _ssboLight.vertexNum = 4;
            break;
    }

    setSpotLight(glm::radians(30.f), glm::radians(25.f));

    getParentTransform()->updateSignal.Connect(std::bind(&Light::transformListener, this, std::placeholders::_1));
}
Light::~Light() {
    for (auto &&e : _descriptorSetsInfo) {
        g_App->getDevice()->Destroy(e.buffer);
        g_App->getDescriptorPool()->Free(1, &e.set);
    }
}
void Light::prepareImpl() {
    updateLightPVs();
    auto count = 1;
    _descriptorSetsInfo.resize(count);
    std::vector<Shit::WriteDescriptorSet> writes;
    std::vector<Shit::DescriptorBufferInfo> buffersInfo(count);
    for (int i = 0; i < count; ++i) {
        auto &&e = _descriptorSetsInfo[i];
        e.buffer = g_App->getDevice()->Create(
            Shit::BufferCreateInfo{
                {},
                sizeof(LightSSBO) + _lightPVs.size() * sizeof(glm::mat4),
                Shit::BufferUsageFlagBits::STORAGE_BUFFER_BIT | Shit::BufferUsageFlagBits::TRANSFER_DST_BIT,
                Shit::MemoryPropertyFlagBits::HOST_VISIBLE_BIT | Shit::MemoryPropertyFlagBits::HOST_COHERENT_BIT},
            &_ssboLight);

        auto a = g_App->getLightDescriptorSetLayout();
        g_App->getDescriptorPool()->Allocate({1, &a}, &e.set);

        buffersInfo[i] = {e.buffer, 0, e.buffer->GetCreateInfoPtr()->size};
        writes.emplace_back(
            Shit::WriteDescriptorSet{e.set, 1, 0, 1, Shit::DescriptorType::STORAGE_BUFFER, 0, &buffersInfo[i]});
    }
    g_App->getDevice()->UpdateDescriptorSets(writes);
}
void Light::updateImpl() {
    _ssboLight.transformMatrix = getParentTransform()->getGlobalTransformMatrix();

    switch (_ssboLight.lightType) {
        case LIGHT_DIRECTIONAL:
            break;
        case LIGHT_SPOT:
            break;
        case LIGHT_TUBE:
            _ssboLight.radius = {glm::length(glm::mat3(_ssboLight.transformMatrix) * glm::vec3(1, 0, 0)), 1, 1};
            _ssboLight.vertices[0] = _ssboLight.transformMatrix * glm::vec4(0, 0, 0, 1);
            _ssboLight.vertices[1] = _ssboLight.transformMatrix * glm::vec4(1, 0, 0, 1);
            break;
        case LIGHT_SPHERE:
            _ssboLight.radius = {glm::length(glm::mat3(_ssboLight.transformMatrix) * glm::vec3(1, 0, 0)), 1, 1};
            break;
        case LIGHT_DISK_SINGLE_FACE:
            _ssboLight.radius = {glm::length(glm::mat3(_ssboLight.transformMatrix) * glm::vec3(1, 0, 0)), 1,
                                 glm::length(glm::mat3(_ssboLight.transformMatrix) * glm::vec3(0, 0, 1))};
            break;
        case LIGHT_QUAD_SINGLE_FACE:
            _ssboLight.radius = {glm::length(glm::mat3(_ssboLight.transformMatrix) * glm::vec3(1, 0, 0)), 1,
                                 glm::length(glm::mat3(_ssboLight.transformMatrix) * glm::vec3(0, 0, 1))};
            _ssboLight.vertices[0] = _ssboLight.transformMatrix * glm::vec4(1, 0, 1, 1);
            _ssboLight.vertices[1] = _ssboLight.transformMatrix * glm::vec4(1, 0, -1, 1);
            _ssboLight.vertices[2] = _ssboLight.transformMatrix * glm::vec4(-1, 0, -1, 1);
            _ssboLight.vertices[3] = _ssboLight.transformMatrix * glm::vec4(-1, 0, 1, 1);
            break;
    }

    updateLightPVs();

    updateUBOBuffer();
}
void Light::setCascadeFrustumVertices(std::span<glm::vec3 const> frustumVertices) {
    _cascadeFrustumVertices.resize(frustumVertices.size());
    int i = 0;
    for (auto &&e : frustumVertices) {
        _cascadeFrustumVertices[i] = e;
        ++i;
    }
    needUpdate();
}
void Light::setSpotLight(float thetaU, float thetaP) {
    _spotThetaU = thetaU;
    _spotThetaP = thetaP;
    _ssboLight.cosThetaU = std::cos(thetaU);
    _ssboLight.cosThetaP = std::cos(thetaP);
    needUpdate();
}
void Light::updateLightPVs() {
    switch (_ssboLight.lightType) {
        case 1: {
            // directional light
            glm::vec3 pmin = glm::vec3((std::numeric_limits<float>::max)());
            glm::vec3 pmax = glm::vec3((std::numeric_limits<float>::min)());
            auto m_v = glm::inverse(glm::mat3(getParentTransform()->getGlobalTransformMatrix()));
            glm::vec3 a;

            auto count = _cascadeFrustumVertices.size() / 8;  // frustum count
            _lightPVs.resize(count * 2);
            for (int i = 0; i < count; ++i) {
                pmin = glm::vec3((std::numeric_limits<float>::max)());
                pmax = glm::vec3((std::numeric_limits<float>::lowest)());
                for (int j = 0; j < 8; ++j) {
                    auto &&e = _cascadeFrustumVertices[i * 8 + j];
                    a = m_v * e;

                    pmin = (glm::min)(a, pmin);
                    pmax = (glm::max)(a, pmax);
                }
                _lightPVs[i * 2] = glm::orthoRH_ZO(pmin.x, pmax.x, pmin.y, pmax.y, -pmax.z, -pmin.z);
                _lightPVs[i * 2 + 1] = glm::mat4(m_v);
            }
        } break;
        case 2: {
            // point light
            _lightPVs.resize(12);
            auto lightPos = getParentTransform()->getGlobalPosition();
            const glm::mat4 M[]{
                glm::lookAtRH(lightPos, lightPos + glm::vec3{1, 0, 0}, {0, -1, 0}),
                glm::lookAtRH(lightPos, lightPos + glm::vec3{-1, 0, 0}, {0, -1, 0}),
                glm::lookAtRH(lightPos, lightPos + glm::vec3{0, 1, 0}, {0, 0, 1}),
                glm::lookAtRH(lightPos, lightPos + glm::vec3{0, -1, 0}, {0, 0, -1}),
                glm::lookAtRH(lightPos, lightPos + glm::vec3{0, 0, 1}, {0, -1, 0}),
                glm::lookAtRH(lightPos, lightPos + glm::vec3{0, 0, -1}, {0, -1, 0}),

            };
            const glm::mat4 P = glm::perspectiveRH_ZO(glm::radians(90.f), 1.f, 0.1f, _ssboLight.rmax);
            for (int i = 0; i < 6; ++i) {
                _lightPVs[i * 2] = P;
                _lightPVs[i * 2 + 1] = M[i];
            }
        } break;
        case 3: {
            // spot light
            _lightPVs = {glm::perspectiveRH_ZO(_spotThetaU * 2.f, 1.f, 0.1f, _ssboLight.rmax),
                         glm::inverse(getParentTransform()->getGlobalTransformMatrix())};
        } break;

        default:
            break;
    }
}
void Light::updateUBOBuffer() {
    auto &&e = _descriptorSetsInfo[0];
    char *data;
    auto l = sizeof(LightSSBO);
    e.buffer->MapMemory(0, e.buffer->GetCreateInfoPtr()->size, (void **)&data);
    memcpy(data, &_ssboLight, l);
    memcpy(&data[l], _lightPVs.data(), _lightPVs.size() * sizeof(glm::mat4));
    e.buffer->UnMapMemory();
}
