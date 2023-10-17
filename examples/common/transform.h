#pragma once
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "component.h"
#include "glm/glm.hpp"

enum class TransformSpace { LOCAL, PARENT, WORLD };

class Transform : public Component {
    glm::mat4 _globalTransformMatrix{1};
    glm::mat4 _localTransformMatrix{1};

    glm::vec3 _localTranslation{0};
    glm::vec3 _localScale{1};
    glm::quat _localRotation{1, 0, 0, 0};  // wxyz

    glm::vec3 _defaultLocalTranslation{0};
    glm::vec3 _defaultLocalScale{1};
    glm::quat _defaultLocalRotation{1, 0, 0, 0};  // wxyz

    Shit::Buffer *_transformBuffer;
    Shit::DescriptorSet *_descriptorSet;

    void init();
    void updateTransformBuffer();

public:
    Transform(GameObject *parent);
    Transform(GameObject *parent, std::string_view name);
    ~Transform();

    constexpr Shit::DescriptorSet *getDescriptorSet() const { return _descriptorSet; }

    Transform *reset();

    constexpr Transform *setDefaultLocalTranslation(glm::vec3 const &translation) {
        _defaultLocalTranslation = translation;
        return this;
    }
    constexpr Transform *setDefaultLocalScale(glm::vec3 const &scale) {
        _defaultLocalScale = scale;
        return this;
    }
    constexpr Transform *setDefaultLocalRotation(glm::quat const &rotation) {
        _defaultLocalRotation = rotation;
        return this;
    }

    Transform *setLocalTranslation(glm::vec3 const &translation);
    Transform *setLocalScale(glm::vec3 const &scale);
    Transform *setLocalRotation(glm::quat const &rotation);

    constexpr glm::vec3 const &getLocalTranslation() const { return _localTranslation; }
    constexpr glm::vec3 const &getLocalScale() const { return _localScale; }
    constexpr glm::quat const &getLocalRotation() const { return _localRotation; }

    Transform *translate(const glm::vec3 d, TransformSpace space = TransformSpace::PARENT);
    Transform *rotate(const glm::quat &auat, TransformSpace space = TransformSpace::PARENT);
    Transform *rotate(glm::vec3 const eulerAngle, TransformSpace space = TransformSpace::PARENT);
    Transform *scale(const glm::vec3 &scale, TransformSpace space = TransformSpace::LOCAL);

    // roll yaw pitch is working in local space
    Transform *roll(float radian, TransformSpace space = TransformSpace::LOCAL) {
        return rotate(glm::quat(glm::vec3(0, 0, radian)), space);
    }
    Transform *yaw(float radian, TransformSpace space = TransformSpace::LOCAL) {
        return rotate(glm::quat(glm::vec3(0, radian, 0)), space);
    }
    Transform *pitch(float radian, TransformSpace space = TransformSpace::LOCAL) {
        return rotate(glm::quat(glm::vec3(radian, 0, 0)), space);
    }
    constexpr glm::mat4 const &getGlobalTransformMatrix() const { return _globalTransformMatrix; }
    constexpr glm::mat4 const &getLocalTransformMatrix() const { return _localTransformMatrix; }
    constexpr glm::vec3 getGlobalPosition() const { return glm::vec3(_globalTransformMatrix[3]); }

    constexpr Shit::Buffer *getTransformBufferHandle() const { return _transformBuffer; }

    void updateImpl() override;
};
