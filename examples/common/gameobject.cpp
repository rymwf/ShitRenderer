#pragma once
#include "gameobject.h"

#include "transform.h"

GameObject::GameObject(GameObject *parent) : _parent(parent) { addComponent<Transform>(); }
GameObject::GameObject(std::string_view name, GameObject *parent) : _parent(parent), _name(name) {
    addComponent<Transform>();
}
GameObject *GameObject::addChild(std::string_view name) {
    return _children.emplace_back(std::make_unique<GameObject>(name, this)).get();
}
void GameObject::attachChild(std::unique_ptr<GameObject> &&child) {
    _children.emplace_back(std::move(child));
    child->_parent = this;
}
std::unique_ptr<GameObject> &&GameObject::detachFromParent() {
    auto it = std::find_if(_parent->_children.begin(), _parent->_children.end(),
                           [this](auto &&e) { return e.get() == this; });
    auto &&ret = std::move(*it);
    _parent->_children.erase(it);
    return std::move(ret);
}
void GameObject::prepare() {
    for (auto &&e : _components) e->prepare();

    prepareImpl();
    for (auto &&e : _children) {
        e->prepare();
    }
}
void GameObject::update() {
    if (_needUpdate) {
        updateImpl();
        for (auto &&e : _children) {
            e->update();
        }
        _needUpdate = false;
    }
}
void GameObject::needUpdate(bool notifyParent, bool notifyChildren) {
    _needUpdate = true;
    if (getComponent<Transform>()->_status == Component::Status::UPDATED)
        getComponent<Transform>()->_status = Component::Status::PREPARED;
    if (_parent && notifyParent) {
        _parent->needUpdate(true, false);
    }
    if (notifyChildren) {
        for (auto &&e : _children) {
            e->needUpdate(false, true);
        }
    }
}
void GameObject::updateImpl() {
    for (auto &&e : _components) {
        e->update();
    }
    updateSignal();
}