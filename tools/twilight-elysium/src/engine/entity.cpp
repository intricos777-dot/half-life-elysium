#include "entity.h"
#include "engine.h"
#include "math_types.h"
#include <cstdio>
#include <algorithm>

namespace te {

Entity::Entity(uint64_t id, const std::string& name)
    : m_id(id), m_name(name) {}

Entity::~Entity() = default;

Scene::Scene(const std::string& name) : m_name(name) {}
Scene::~Scene() { clear(); }

Entity* Scene::create_entity(const std::string& name) {
    auto entity = std::make_unique<Entity>(m_next_id++, name);
    Entity* ptr = entity.get();
    m_entities.push_back(std::move(entity));
    return ptr;
}

Entity* Scene::get_entity(uint64_t id) const {
    for (const auto& e : m_entities) {
        if (e->id() == id) {
            return e.get();
        }
    }
    return nullptr;
}

void Scene::remove_entity(uint64_t id) {
    m_entities.erase(
        std::remove_if(m_entities.begin(), m_entities.end(),
            [id](const std::unique_ptr<Entity>& e) { return e->id() == id; }),
        m_entities.end()
    );
}

void Scene::clear() {
    m_entities.clear();
    m_next_id = 1;
}

} // namespace te
