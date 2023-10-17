#pragma once
#include <array>
#include <variant>

#include "component.h"
#include "glm/glm.hpp"

struct PerspectiveDescription {
    float fovy;    // radians
    float aspect;  // width/height
};
struct OrthogonalDescription {
    float xmag;
    float ymag;
};

struct CameraUBO {
    glm::mat4 V;
    glm::mat4 P;
    glm::vec3 eyePos;
    int cascadeNum = 1;
    float cascadeSplit;
};

class Camera : public Component {
    float _near{0.1f};
    float _far{1000.f};  // if far==0, means infinite perspective, orthogonal cannot be 0
    // float _aspect{};
    // bool _orthogonal{false};
    // float _orthgonalScale{10};
    // float _focalLength{0.05};

    std::variant<PerspectiveDescription, OrthogonalDescription> _extraDesc =
        PerspectiveDescription{glm::radians(60.f), 1.777};

    std::array<glm::vec3, 4> _nearVerticesInCameraSpace;
    std::array<glm::vec3, 4> _farVerticesInCameraSpace;

    std::vector<float> _cascadeSplits;
    std::vector<float> _cascadeSplitRatios;

    CameraUBO _uboBufferData;

    std::vector<Shit::Buffer *> _uboBuffers;
    std::vector<Shit::DescriptorSet *> _descriptorSets;
    std::vector<bool> _uboBufferNeedUpdate;

    float _focalLength{0.05};  // m
    float _cameraSize{0.036};  // m

    void updateProjectionMatrix();

    void updateImpl() override;
    void prepareImpl() override;

public:
    Camera(GameObject *parent);
    Camera(GameObject *parent, OrthogonalDescription orth, float n, float f);
    Camera(GameObject *parent, PerspectiveDescription persp, float n, float f);
    Camera(GameObject *parent, float aspect, float n, float f);
    ~Camera();

    void update() override;

    constexpr float getNear() const { return _near; }
    constexpr float getFar() const { return _far; }
    constexpr OrthogonalDescription *getOrthgonalDesc() { return std::get_if<OrthogonalDescription>(&_extraDesc); }
    constexpr PerspectiveDescription *getPerspectiveDesc() { return std::get_if<PerspectiveDescription>(&_extraDesc); }

    constexpr Shit::DescriptorSet *getDescriptorSet(uint32_t frameIndex) const {
        return _descriptorSets.at(frameIndex);
    }

    Camera *setOrthogonal(float xmag, float ymag);
    Camera *setPerspective(float fovy, float aspect);
    Camera *setNearPlane(float n);
    Camera *setFarPlane(float f);

    Camera *setViewportAspect(float aspect);
    Camera *setFocalLength(float focalLength);
    Camera *setCameraSize(float cameraSize);

    float getFocalLength();

    constexpr float getCameraSize() const { return _cameraSize; }

    void updateUBOBuffer(uint32_t frameIndex);

    glm::mat4 getViewMatrix();
    constexpr glm::mat4 const &getProjectionMatrix() const { return _uboBufferData.P; }
    void setCascadeSplitRatios(std::span<float const> ratios);
    std::array<glm::vec3, 8> getFrustumVertices(int cascadeIndex);
    constexpr int getCascadeNum() const { return _cascadeSplitRatios.size() + 1; }
};

class EditCameraController : public Behaviour {
    // per second
    float _speed = 3.f;
    GLFWwindow *_window;

    void prepareImpl() override;
    void updatePerFrameImpl(float frameDtMs) override;

public:
    EditCameraController(GameObject *gameobject, GLFWwindow *window);
    ~EditCameraController() {}

    void onScroll(double xoffset, double yoffset) override;

    constexpr void setSpeed(float speed) { _speed = speed; }
};