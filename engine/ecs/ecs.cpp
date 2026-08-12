#include "ecs.hpp"

namespace axiom {

std::size_t hash_entity(const entity& e) {
    return std::hash<uint>()(e.id);
}

void entity_manager::init() {
    for(int i = 0; i < MAX_ENTITIES; ++i) {
        available_ids.push(i);
    }
    
    std::fill(signatures.begin(), signatures.end(), 0);
}

int entity_manager::insert_entity() {
    if(available_ids.size()) {
        uint entity_id = available_ids.front();
        available_ids.pop();
        
        entities.emplace(entity_id);
        return entity_id;
    }
    
    return -1;
}

void entity_manager::erase_entity(uint entity_) {
    entities.erase(entity_);
    available_ids.push(entity_);
}

ecs::ecs() {
    entity_manager_.init();
}

axiom::collector& get_collector(std::string name) {
    return axiom::global_core.ecs->collectors[name];
};

axiom::core global_core;

}

