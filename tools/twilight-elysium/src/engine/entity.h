#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include "math_types.h"

namespace te {

struct Component {
    virtual ~Component() = default;
    uint64_t id = 0;
};

struct TransformComponent : public Component {
    Vec3 position{0.0f, 0.0f, 0.0f};
    Vec3 rotation{0.0f, 0.0f, 0.0f};
    Vec3 scale{1.0f, 1.0f, 1.0f};
};

struct MeshComponent : public Component {
    std::string mesh_path;
    uint32_t material_id = 0;
};

struct CameraComponent : public Component {
    float fov = 60.0f;
    float near_plane = 0.1f;
    float far_plane = 1000.0f;
};

class Entity {
public:
    Entity(uint64_t id, const std::string& name);
    ~Entity();

    uint64_t id() const { return m_id; }
    const std::string& name() const { return m_name; }

    template<typename T>
    T* add_component() {
        static_assert(std::is_base_of_v<Component, T>, "T must derive Component");
        auto comp = std::make_unique<T>();
        T* ptr = comp.get();
        m_components[ptr->id] = std::move(comp);
        return ptr;
    }

    template<typename T>
    T* get_component() const {
        static_assert(std::is_base_of_v<Component, T>, "T must derive Component");
        for (const auto& pair : m_components) {
            if (dynamic_cast<T*>(pair.second.get())) {
                return static_cast<T*>(pair.second.get());
            }
        }
        return nullptr;
    }

private:
    uint64_t m_id;
    std::string m_name;
    std::unordered_map<uint64_t, std::unique_ptr<Component>> m_components;
};

class Scene {
public:
    Scene(const std::string& name);
    ~Scene();

    Entity* create_entity(const std::string& name);
    Entity* get_entity(uint64_t id) const;
    void remove_entity(uint64_t id);
    const std::vector<std::unique_ptr<Entity>>& entities() const { return m_entities; }

    void clear();

private:
    std::string m_name;
    std::vector<std::unique_ptr<Entity>> m_entities;
    uint64_t m_next_id = 1;
};

} // namespace te
