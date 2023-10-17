#include "camera.h"

#include "appbase.h"
#include "transform.h"

Camera::Camera(GameObject *parent) : Component(parent) {}
Camera::Camera(GameObject *parent, OrthogonalDescription orth, float n, float f)
    : Component(parent), _near(n), _far(f), _extraDesc(orth) {}
Camera::Camera(GameObject *parent, PerspectiveDescription persp, float n, float f)
    : Component(parent), _near(n), _far(f), _extraDesc(persp) {}
Camera::Camera(GameObject *parent, float aspect, float n, float f)
    : Component(parent),
      _near(n),
      _far(f),
      _extraDesc(PerspectiveDescription{float(2.f * std::atan2(1. / (2 * aspect), 0.05 / _cameraSize)), aspect}) {}
Camera::~Camera() {
    for (auto e : _uboBuffers) g_App->getDevice()->Destroy(e);
    _uboBuffers.clear();

    for (auto e : _descriptorSets) g_App->getDescriptorPool()->Free(1, &e);
    _descriptorSets.clear();
}
void Camera::prepareImpl() {
    static const uint32_t DESCRIPTOR_BINDING_CAMERA = 0;

    auto frameCount = g_App->getSwapchain()->GetImageCount();
    _uboBufferNeedUpdate.resize(frameCount, true);

    _descriptorSets.resize(frameCount);
    auto descriptorSetLayout = g_App->getCameraDescriptorSetLayout();
    std::vector<Shit::DescriptorSetLayout *> setLayouts(frameCount, descriptorSetLayout);
    g_App->getDescriptorPool()->Allocate(
        Shit::DescriptorSetAllocateInfo{(uint32_t)setLayouts.size(), setLayouts.data()}, _descriptorSets.data());

    std::vector<Shit::DescriptorBufferInfo> bufferInfos(frameCount);
    _uboBuffers.resize(frameCount);
    std::vector<Shit::WriteDescriptorSet> writes;
    for (uint32_t i = 0; i < frameCount; ++i) {
        _uboBuffers[i] = g_App->getDevice()->Create(Shit::BufferCreateInfo{
            {},
            offsetof(CameraUBO, cascadeSplit) + _cascadeSplits.size() * sizeof(float),
            Shit::BufferUsageFlagBits::STORAGE_BUFFER_BIT | Shit::BufferUsageFlagBits::TRANSFER_DST_BIT,
            Shit::MemoryPropertyFlagBits::HOST_COHERENT_BIT | Shit::MemoryPropertyFlagBits::HOST_VISIBLE_BIT});

        bufferInfos[i] = {_uboBuffers[i], 0, offsetof(CameraUBO, cascadeSplit) + _cascadeSplits.size() * sizeof(float)};

        writes.emplace_back(_descriptorSets[i], DESCRIPTOR_BINDING_CAMERA, 0, 1, Shit::DescriptorType::STORAGE_BUFFER,
                            nullptr, &bufferInfos[i]);
    };
    g_App->getDevice()->UpdateDescriptorSets(writes);
    updateProjectionMatrix();
}
void Camera::setCascadeSplitRatios(std::span<float const> ratios) {
    _cascadeSplitRatios.clear();
    _uboBufferData.cascadeNum = ratios.size() + 1;
    for (auto e : ratios) {
        _cascadeSplitRatios.emplace_back(e);
        _cascadeSplits.emplace_back((_far - _near) * e + _near);
    }
}
Camera *Camera::setOrthogonal(float xmag, float ymag) {
    _extraDesc = OrthogonalDescription{xmag, ymag};
    updateProjectionMatrix();
    return this;
}
Camera *Camera::setPerspective(float fovy, float aspect) {
    _extraDesc = PerspectiveDescription{fovy, aspect};
    updateProjectionMatrix();
    return this;
}
Camera *Camera::setNearPlane(float n) {
    _near = n;
    updateProjectionMatrix();
    return this;
}
Camera *Camera::setFarPlane(float f) {
    _far = f;
    updateProjectionMatrix();
    return this;
}
Camera *Camera::setViewportAspect(float aspect) {
    if (auto p = std::get_if<PerspectiveDescription>(&_extraDesc)) {
        p->aspect = aspect;
    } else if (auto p2 = std::get_if<OrthogonalDescription>(&_extraDesc)) {
        p2->ymag = p2->xmag / aspect;
    }
    updateProjectionMatrix();
    return this;
}
Camera *Camera::setCameraSize(float cameraSize) {
    _cameraSize = cameraSize;
    if (auto p = std::get_if<PerspectiveDescription>(&_extraDesc)) {
        p->fovy = float(2.f * std::atan2(1. / (2 * p->aspect), _focalLength / _cameraSize));
    }
    updateProjectionMatrix();
    return this;
}
Camera *Camera::setFocalLength(float focalLength) {
    _focalLength = focalLength;
    if (auto p = std::get_if<PerspectiveDescription>(&_extraDesc)) {
        p->fovy = float(2.f * std::atan2(1. / (2 * p->aspect), _focalLength / _cameraSize));
    }
    updateProjectionMatrix();
    return this;
}
float Camera::getFocalLength() { return _focalLength; }
glm::mat4 Camera::getViewMatrix() { return glm::inverse(getParentTransform()->getGlobalTransformMatrix()); }
void Camera::updateProjectionMatrix() {
    if (auto p = std::get_if<PerspectiveDescription>(&_extraDesc)) {
        // depth range [0,1]
        _uboBufferData.P = glm::perspectiveRH_ZO(p->fovy, p->aspect, _near, _far);
        //_perspectiveMatrix = _far
        //						 ? (_viewport.minDepth == 0
        //								? glm::perspectiveRH_ZO(p->fovy, p->aspect,
        //_near,
        //_far) 								: glm::perspectiveRH_NO(p->fovy, p->aspect,
        //_near, _far)) 						 :
        // glm::infinitePerspective(p->fovy, p->aspect, _near);

        auto tanHalfFovy = glm::tan(p->fovy / 2);
        auto yn = tanHalfFovy * _near;
        auto yf = tanHalfFovy * _far;
        auto xn = yn * p->aspect;
        auto xf = yf * p->aspect;

        _nearVerticesInCameraSpace = {
            glm::vec3{-xn, -yn, -_near},
            glm::vec3{xn, -yn, -_near},
            glm::vec3{-xn, yn, -_near},
            glm::vec3{xn, yn, -_near},
        };
        _farVerticesInCameraSpace = {
            glm::vec3{-xf, -yf, -_far},
            glm::vec3{xf, -yf, -_far},
            glm::vec3{-xf, yf, -_far},
            glm::vec3{xf, yf, -_far},
        };
    } else {
        auto p2 = std::get_if<OrthogonalDescription>(&_extraDesc);
        auto x = p2->xmag / 2;
        auto y = p2->ymag / 2;

        // ret = glm::ortho(-p2->xmag / 2, p2->xmag / 2, -p2->ymag / 2, p2->ymag /
        // 2, n, f);
        // TODO: need to detect depth range
        _uboBufferData.P = glm::orthoRH_ZO(-x, x, -y, y, _near, _far);
        //_perspectiveMatrix = _viewport.minDepth == 0
        //						 ? glm::orthoRH_ZO(-x, x, -y, y,
        //_near,
        //_far) 						 : glm::orthoRH_NO(-x, x, -y, y, _near,
        //_far);

        _nearVerticesInCameraSpace = {
            glm::vec3{-x, -y, -_near},
            glm::vec3{x, -y, -_near},
            glm::vec3{-x, y, -_near},
            glm::vec3{x, y, -_near},
        };
        _farVerticesInCameraSpace = {
            glm::vec3{-x, -y, -_far},
            glm::vec3{x, -y, -_far},
            glm::vec3{-x, y, -_far},
            glm::vec3{x, y, -_far},
        };
    }
    needUpdate();
    updateSignal(this);
}
void Camera::update() {
    if (_status == Status::CREATED) prepare();
    _status = Status::UPDATED;
    updateImpl();
    updateSignal(this);
}
void Camera::updateImpl() {
    // updateUBOBuffer();
    _uboBufferNeedUpdate.assign(_uboBufferNeedUpdate.size(), true);
}
void Camera::updateUBOBuffer(uint32_t frameIndex) {
    if (!_uboBufferNeedUpdate[frameIndex]) return;
    _uboBufferNeedUpdate[frameIndex] = false;

    char *data;
    _uboBuffers[frameIndex]->MapMemory(0, _uboBuffers[frameIndex]->GetCreateInfoPtr()->size, (void **)&data);
    _uboBufferData.V = getViewMatrix();
    _uboBufferData.eyePos = getParentTransform()->getGlobalPosition();
    memcpy(data, &_uboBufferData, offsetof(CameraUBO, cascadeSplit));
    memcpy(&data[offsetof(CameraUBO, cascadeSplit)], _cascadeSplits.data(), _cascadeSplits.size() * sizeof(float));
    _uboBuffers[frameIndex]->UnMapMemory();
}
std::array<glm::vec3, 8> Camera::getFrustumVertices(int cascadeIndex) {
    float start = 0;
    // float start = (cascadeIndex - 1) < 0 ? 0 : _cascadeSplitRatios[cascadeIndex
    // - 1];
    float end = (cascadeIndex == _cascadeSplitRatios.size()) ? 1. : (_cascadeSplitRatios[cascadeIndex] + 0.01);

    std::array<glm::vec3, 8> ret;
    for (int i = 0; i < 4; ++i) {
        ret[i] = _nearVerticesInCameraSpace[i] + (_farVerticesInCameraSpace[i] - _nearVerticesInCameraSpace[i]) * start;
        ret[i + 4] =
            _nearVerticesInCameraSpace[i] + (_farVerticesInCameraSpace[i] - _nearVerticesInCameraSpace[i]) * end;
    }
    auto &&m = getParentTransform()->getGlobalTransformMatrix();
    for (auto &e : ret) e = glm::vec3(m * glm::vec4(e.x, e.y, e.z, 1));
    return ret;
}
//=======================
enum DirectionBits {
    LEFT = 1,
    RIGHT = 1 << 1,
    UP = 1 << 2,
    DOWN = 1 << 3,
    LEFT_UP = LEFT | UP,
    LEFT_DOWN = LEFT | DOWN,
    RIGHT_UP = RIGHT | UP,
    RIGHT_DOWN = RIGHT | DOWN,
};
glm::vec3 getDirectionVec(DirectionBits dir) {
    switch (dir) {
        case DirectionBits::LEFT:
            return glm::vec3(-1, 0, 0);
        case DirectionBits::RIGHT:
            return glm::vec3(1, 0, 0);
        case DirectionBits::UP:
            return glm::vec3(0, 0, -1);
        case DirectionBits::DOWN:
            return glm::vec3(0, 0, 1);
        case DirectionBits::LEFT_UP:
            return glm::vec3(-1, 0, -1);
        case DirectionBits::LEFT_DOWN:
            return glm::vec3(-1, 0, 1);
        case DirectionBits::RIGHT_UP:
            return glm::vec3(1, 0, -1);
        case DirectionBits::RIGHT_DOWN:
            return glm::vec3(1, 0, 1);
    }
    return glm::vec3(0, 0, 0);
}

EditCameraController::EditCameraController(GameObject *parent, GLFWwindow *window)
    : Behaviour(parent), _window(window) {}
void EditCameraController::prepareImpl() { getParentTransform()->translate({0., 0., 3.}); }
void EditCameraController::updatePerFrameImpl(float frameDtMs) {
    ImGuiIO &io = ImGui::GetIO();
    if (!io.WantCaptureKeyboard) {
        auto keyStateW = glfwGetKey(_window, GLFW_KEY_W) == GLFW_PRESS;
        auto keyStateS = glfwGetKey(_window, GLFW_KEY_S) == GLFW_PRESS;
        auto keyStateA = glfwGetKey(_window, GLFW_KEY_A) == GLFW_PRESS;
        auto keyStateD = glfwGetKey(_window, GLFW_KEY_D) == GLFW_PRESS;

        auto keyStateLeft = glfwGetKey(_window, GLFW_KEY_LEFT) == GLFW_PRESS;
        auto keyStateRight = glfwGetKey(_window, GLFW_KEY_RIGHT) == GLFW_PRESS;
        auto keyStateUp = glfwGetKey(_window, GLFW_KEY_UP) == GLFW_PRESS;
        auto keyStateDown = glfwGetKey(_window, GLFW_KEY_DOWN) == GLFW_PRESS;

        // bit0: left
        // bit1: right
        // bit2: up
        // bit3: down
        static int keystateBits;

        if (keyStateA || keyStateLeft)
            keystateBits |= 1;
        else
            keystateBits &= ~1;
        if (keyStateD || keyStateRight)
            keystateBits |= (1 << 1);
        else
            keystateBits &= ~(1 << 1);
        if (keyStateW || keyStateUp)
            keystateBits |= (1 << 2);
        else
            keystateBits &= ~(1 << 2);
        if (keyStateS || keyStateDown)
            keystateBits |= (1 << 3);
        else
            keystateBits &= ~(1 << 3);

        glm::vec3 dir = getDirectionVec((DirectionBits)keystateBits);
        if (dir.x != 0 || dir.y != 0 || dir.z != 0) {
            dir *= _speed * frameDtMs / 1000;
            getParentTransform()->translate(dir, TransformSpace::LOCAL);
        }
    }

    if (!io.WantCaptureMouse) {
        //=============================================================
        auto keyStateMouseL = glfwGetMouseButton(_window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        auto keyStateMouseR = glfwGetMouseButton(_window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
        auto keyStateMouseM = glfwGetMouseButton(_window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;
        // wheel
        auto keyStateAltL = glfwGetKey(_window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS;

        // wheel
        // auto keyStateAltL = Shit::GetKeyState(Shit::KeyCode::KEY_LMENU);
        static bool flag = true;
        static double preCursorX, preCursorY, cursorX, cursorY;
        if (keyStateMouseL)  //|| (keyStateMouseL.down && keyStateAltL.down))
        {
            static float c = 0.001;
            glfwGetCursorPos(_window, &cursorX, &cursorY);

            if (flag) {
                glfwGetCursorPos(_window, &preCursorX, &preCursorY);
                flag = false;
            }
            if (cursorX != preCursorX) {
                getParentTransform()->yaw((preCursorX - cursorX) * c, TransformSpace::PARENT);
            }
            if (cursorY != preCursorY) {
                auto a = glm::mat3(getParentTransform()->getGlobalTransformMatrix()) *
                         glm::vec3((preCursorY - cursorY) * c, 0, 0);
                getParentTransform()->rotate(a, TransformSpace::PARENT);
                // getParentTransform()->pitch((preCursorY - cursorY) *
                // c,TransformSpace::PARENT);
            }
            preCursorX = cursorX, preCursorY = cursorY;
        } else if (keyStateMouseR) {
            static float c = 0.001;
            c = 0.001 * glm::length(getParentTransform()->getGlobalPosition());
            glfwGetCursorPos(_window, &cursorX, &cursorY);
            if (flag) {
                glfwGetCursorPos(_window, &preCursorX, &preCursorY);
                flag = false;
            }
            if (cursorX != preCursorX) {
                getParentTransform()->translate(glm::vec3((preCursorX - cursorX) * c, 0, 0), TransformSpace::LOCAL);
            }
            if (cursorY != preCursorY) {
                getParentTransform()->translate(glm::vec3(0, (cursorY - preCursorY) * c, 0), TransformSpace::LOCAL);
            }
            preCursorX = cursorX, preCursorY = cursorY;
        } else {
            flag = true;
        }
    }
}

void EditCameraController::onScroll(double xoffset, double yoffset) {
    auto &io = ImGui::GetIO();
    if (io.WantCaptureMouse) return;
    static float c = 0.1;
    getParentTransform()->translate(
        glm::vec3(0, 0, -glm::length(getParentTransform()->getGlobalPosition()) * c * yoffset), TransformSpace::LOCAL);
}