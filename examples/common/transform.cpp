#include "transform.h"

#include "appbase.h"
#include "gameobject.h"

glm::mat3 TBN(glm::vec3 n) {
    glm::vec3 up{0, 1, 0};
    n = glm::normalize(n);
    if (n.y > 0.999) up = {1, 0, 0};
    auto t = glm::cross(up, n);
    auto b = glm::cross(n, t);
    return glm::mat3(t, b, n);
}

Transform::Transform(GameObject *parent) : Component(parent) { init(); }
Transform::Transform(GameObject *parent, std::string_view name) : Component(parent, name) { init(); }
Transform::~Transform() {
    g_App->getDevice()->Destroy(_transformBuffer);
    g_App->getDescriptorPool()->Free(1, &_descriptorSet);
}
void Transform::init() {
    static const uint32_t DESCRIPTOR_BINDING_TRANSFORM = 0;
    _transformBuffer = g_App->getDevice()->Create(Shit::BufferCreateInfo{
        {},
        sizeof(float) * 16,
        Shit::BufferUsageFlagBits::UNIFORM_BUFFER_BIT | Shit::BufferUsageFlagBits::TRANSFER_DST_BIT,
        Shit::MemoryPropertyFlagBits::HOST_COHERENT_BIT | Shit::MemoryPropertyFlagBits::HOST_VISIBLE_BIT});

    auto descriptorSetLayout = g_App->getTransformDescriptorSetLayout();
    g_App->getDescriptorPool()->Allocate(Shit::DescriptorSetAllocateInfo{1, &descriptorSetLayout}, &_descriptorSet);

    // write
    Shit::DescriptorBufferInfo bufferInfo{_transformBuffer, 0, sizeof(glm::mat4)};
    Shit::WriteDescriptorSet setWrite{
        _descriptorSet, DESCRIPTOR_BINDING_TRANSFORM, 0, 1, Shit::DescriptorType::UNIFORM_BUFFER, nullptr, &bufferInfo};
    g_App->getDevice()->UpdateDescriptorSets(std::span(&setWrite, 1));
}

Transform *Transform::reset() {
    _localTranslation = _defaultLocalTranslation;
    _localRotation = _defaultLocalRotation;
    _localScale = _defaultLocalScale;
    needUpdate(true);
    return this;
}
Transform *Transform::setLocalTranslation(glm::vec3 const &translation) {
    _localTranslation = translation;
    needUpdate(true);
    return this;
}
Transform *Transform::setLocalScale(glm::vec3 const &scale) {
    _localScale = scale;
    needUpdate(true);
    return this;
}
Transform *Transform::setLocalRotation(glm::quat const &rotation) {
    _localRotation = rotation;
    needUpdate(true);
    return this;
}
Transform *Transform::translate(const glm::vec3 d, TransformSpace space) {
    switch (space) {
        case TransformSpace::LOCAL:
            _localTranslation += _localRotation * d;
            break;
        case TransformSpace::PARENT:
            _localTranslation += d;
            break;
        case TransformSpace::WORLD: {
            glm::mat4 temp{1};
            // if (_parent)
            //	temp = glm::inverse(static_cast<SceneNode
            //*>(_parent)->getGlobalTransformMatrix());
            _localTranslation += glm::vec3(temp * glm::vec4(d, 1));
            break;
        }
        default:
            return this;
    }
    needUpdate(true);
    return this;
}
Transform *Transform::rotate(const glm::quat &rotation, TransformSpace space) {
    switch (space) {
        case TransformSpace::LOCAL:
            _localRotation *= rotation;
            break;
        case TransformSpace::PARENT:
            _localTranslation = rotation * _localTranslation;
            _localRotation = rotation * _localRotation;
            break;
        case TransformSpace::WORLD:
            if (auto p = _parent->getParent()) {
                auto v = p->getComponent<Transform>()->getGlobalPosition();
                _localTranslation = rotation * (v + _localTranslation);
                _localRotation = rotation * _localRotation;
            } else {
                rotate(rotation, TransformSpace::PARENT);
                return this;
            }
            break;
    }
    needUpdate(true);
    return this;
}
Transform *Transform::rotate(glm::vec3 const eulerAngle, TransformSpace space) {
    return rotate(glm::quat(eulerAngle), space);
}
Transform *Transform::scale(const glm::vec3 &scale, TransformSpace space) {
    switch (space) {
        case TransformSpace::LOCAL:
            _localScale *= scale;
            break;
        case TransformSpace::PARENT:
            _localScale *= scale;
            break;
        case TransformSpace::WORLD: {
            //_globalTransformMatrix = getGlobalTransformMatrix();
            //_localTransform.rotation =
            // glm::quat_cast(glm::inverse(_globalTransformMatrix) *
            // glm::scale(glm::mat4(1), scale) * _globalTransformMatrix) *
            //						   _localTransform.rotation;
            break;
        }
    }
    needUpdate(true);
    return this;
}
void Transform::updateImpl() {
    _localTransformMatrix = glm::mat4_cast(_localRotation) * glm::scale(glm::mat4(1), _localScale);
    _localTransformMatrix[3].x += _localTranslation.x;
    _localTransformMatrix[3].y += _localTranslation.y;
    _localTransformMatrix[3].z += _localTranslation.z;
    if (auto p = _parent->getParent()) {
        _globalTransformMatrix = p->getComponent<Transform>()->getGlobalTransformMatrix() * _localTransformMatrix;
    } else
        _globalTransformMatrix = _localTransformMatrix;

    // update ubo buffer
    void *data;
    _transformBuffer->MapMemory(0, sizeof(glm::mat4), &data);
    memcpy(data, &_globalTransformMatrix, sizeof(glm::mat4));
    _transformBuffer->UnMapMemory();
}