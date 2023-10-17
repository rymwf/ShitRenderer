#pragma once
#include "prerequisites.h"
#include "transform.h"

class GameObject : public IdObject<GameObject> {
protected:
    std::string _name;

    std::vector<std::unique_ptr<Component>> _components;

    GameObject *_parent{};
    std::vector<std::unique_ptr<GameObject>> _children;

    bool _needUpdate = true;
    bool _enable = true;

    virtual void updateImpl();
    virtual void prepareImpl() {}

public:
    GameObject(GameObject *parent = nullptr);
    GameObject(std::string_view name, GameObject *parent = nullptr);
    constexpr void enable() { _enable = true; }
    constexpr void disable() { _enable = false; }
    constexpr bool isEnable() const { return _enable; }

    virtual ~GameObject() {}

    GameObject *addChild(std::string_view name = {});
    void attachChild(std::unique_ptr<GameObject> &&child);
    std::unique_ptr<GameObject> &&detachFromParent();

    constexpr GameObject *getParent() const { return _parent; }

    constexpr decltype(auto) childBegin() { return _children.begin(); }
    constexpr decltype(auto) childEnd() { return _children.end(); }
    constexpr decltype(auto) childCbegin() const { return _children.cbegin(); }
    constexpr decltype(auto) childCend() const { return _children.cend(); }

    constexpr std::string_view getName() const { return _name; }

    template <typename T, typename... Args>
    T *addComponent(Args... args) {
        auto a = new T(this, args...);
        _components.emplace_back(std::unique_ptr<T>(a));
        return a;
    }

    /**
     * @brief Get the Component object
     *
     * @tparam T
     * @return T* return first component T
     */
    template <typename T>
    T *getComponent() {
        for (auto &&e : _components) {
            if (auto it = dynamic_cast<T *>(e.get())) {
                return it;
            }
        }
        return nullptr;
    }

    template <typename T>
    void getComponents(std::vector<T *> &components) {
        for (auto &&e : _components) {
            if (auto it = dynamic_cast<T *>(e.get())) {
                components.emplace_back(it);
            }
        }
    }

    constexpr decltype(auto) componentCbegin() const { return _components.cbegin(); }
    constexpr decltype(auto) componentCend() const { return _components.cend(); }
    constexpr decltype(auto) componentBegin() { return _components.begin(); }
    constexpr decltype(auto) componentEnd() { return _components.end(); }

    void prepare();

    /**
     * @brief update all children
     *
     */
    void update();

    void needUpdate(bool notifyParent = true, bool notifyChildren = true);

    Shit::Signal<void()> updateSignal;
};
