#include "component.h"

#include "gameobject.h"
#include "input.hpp"
#include "transform.h"

Transform *Component::getParentTransform() { return _parent->getComponent<Transform>(); }

void Component::needUpdate(bool notifyChildren) {
    if (_status == Status::UPDATED) _status = Status::PREPARED;
    _parent->needUpdate(true, notifyChildren);
}

Behaviour::Behaviour(GameObject *parent) : Component(parent) {
    Input::scrollSignal.Connect(std::bind(&Behaviour::onScroll, this, std::placeholders::_1, std::placeholders::_2));
}