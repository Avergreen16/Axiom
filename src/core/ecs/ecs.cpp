#include "ecs/ecs.hpp"

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

ecs_core::ecs_core() {
    entity_manager_.init();
}

void ecs_core::erase_entity(uint entity_) {
    axiom::signature entity_signature = entity_manager_.signatures[entity_];
    
    component_manager_.delete_components(entity_, entity_manager_.signatures[entity_]);
    
    entity_manager_.erase_entity(entity_);
    
    for(auto& s : system_manager_.systems) {
        for(collector& c : s.second->collectors) {
            axiom::signature s_signature = c.signature;
            if((entity_signature & s_signature) == s_signature) {
                if(c.entities.find(entity_) != c.entities.end()) {
                    c.entities.erase(entity_);
                }
            }
        }
    }
    

    for(auto& [key, c] : collectors)  {
        axiom::signature s_signature = c.signature;
        if((entity_signature & s_signature) == s_signature) {
            if(c.entities.find(entity_) != c.entities.end()) {
                c.entities.erase(entity_);
            }
        }            
    }
}

void ecs_core::frame() {
    double current_time = axiom::get_absolute_time();
    delta_time = glm::max(0.0, current_time - prev_time);
    prev_time = current_time;

    int i = 0;

    for(std::size_t& code : system_manager_.call_order) {
        auto& system = system_manager_.systems[code];

        system->call();

        ++i;
    }
}

void ecs_core::create_collector(std::string key, collector& collector) {
    collectors.emplace(key, std::move(collector));
}

int ecs_core::insert_entity() {
    return entity_manager_.insert_entity();
}    

ecs_core ecs;

}

