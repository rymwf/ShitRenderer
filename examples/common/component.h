#pragma once
#include "common.h"
#include "idObject.h"
#include "prerequisites.h"

class Transform;
class Component : public IdObject<Component> {
protected:
    GameObject *_parent;
    std::string _name;

    virtual void prepareImpl() {}
    virtual void updateImpl() {}

public:
    enum class Status {
        CREATED,
        PREPARED,
        UPDATED,
    };

    Component(GameObject *parent) : _parent(parent) {}
    Component(GameObject *parent, std::string_view name) : _parent(parent), _name(name) {}
    virtual ~Component() {}

    constexpr std::string_view getName() const { return _name; }
    constexpr GameObject *getParent() const { return _parent; }

    Transform *getParentTransform();

    void prepare() {
        if (_status != Status::CREATED) return;
        _status = Status::PREPARED;
        prepareImpl();
    }
    virtual void update() {
        if (_status == Status::UPDATED)
            return;
        else if (_status == Status::CREATED)
            prepare();
        _status = Status::UPDATED;
        updateImpl();
        updateSignal(this);
    }

    constexpr Status getStatus() const { return _status; }

    void needUpdate(bool notifyChildren = false);

    Shit::Signal<void(Component const *)> updateSignal;

protected:
    Status _status{Status::CREATED};
    friend class GameObject;
};

class Behaviour : public Component {
    virtual void updatePerFrameImpl(float frameDtMs) {}
    bool _pause = false;

public:
    Behaviour(GameObject *parent);
    void updatePerFrame(float frameDtMs) {
        if (_pause) return;
        updatePerFrameImpl(frameDtMs);
    }
    constexpr bool isRunning() const { return !_pause; }
    constexpr void pause() { _pause = true; }
    constexpr void run() { _pause = false; }

    virtual void onScroll(double xoffset, double yoffset) {}
};