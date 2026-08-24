#pragma once;
#include <iostream>
#include <bitset>
#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <typeinfo>
#include <queue>
#include <array>

#include <utilities/utilities.hpp>

using uint = unsigned int;

namespace axiom {

const uint MAX_COMPONENTS = 0xFF;
const uint MAX_ENTITIES = 0xFFFF;
const uint NULL_ENTITY = 0xFFFFFFFF;
using signature = std::bitset<MAX_COMPONENTS>;

struct entity {
    uint id;
    axiom::signature signature = 0;
    
    entity() {}
    
    entity(uint i) {
        id = i;
    }
};

std::size_t hash_entity(const entity& e);

struct entity_manager {
    std::queue<uint> available_ids;
    std::unordered_set<uint> entities;
    std::vector<signature> signatures = std::vector<signature>(MAX_ENTITIES);
    
    void init();
    
    int insert_entity();
    
    void erase_entity(uint entity_);
};

struct clist {
    virtual void remove_component(uint entity_) = 0;
};

template<typename type> 
struct component_list : clist {
    std::unordered_map<uint, uint> entity_to_component;
    std::unordered_map<uint, uint> component_to_entity;
    std::vector<std::shared_ptr<type>> components;
    
    void insert_component(uint entity_, type component) {
        if(entity_to_component.find(entity_) == entity_to_component.end()) {
            uint i = components.size();
            
            entity_to_component.emplace(entity_, i);
            component_to_entity.emplace(i, entity_);
            components.push_back(std::make_shared<type>(component));
        }
    }

    void insert_component_move(uint entity_, type&& component) {
        if(entity_to_component.find(entity_) == entity_to_component.end()) {
            uint i = components.size();
            
            entity_to_component.emplace(entity_, i);
            component_to_entity.emplace(i, entity_);
            components.push_back(std::make_shared<type>(std::move(component)));
        }
    }

    void remove_component(uint entity_) {
        uint i = entity_to_component[entity_]; // component of entity being deleted
        
        entity_to_component.erase(entity_);
        
        uint j = components.size() - 1; // last component index in vector
        
        if(i != j) {
            uint k = component_to_entity[j]; // k is entity id with a component at j
            
            component_to_entity.erase(j);
            
            component_to_entity[i] = k;
            
            entity_to_component[k] = i;
            
            components[i] = std::move(components[j]);
            components.erase(components.begin() + j);
        } else {
            component_to_entity.erase(i);
            components.erase(components.begin() + i);
        }
        
        //components.resize(j);
    }

    type& get_component(uint& entity_) {
        uint i = entity_to_component[entity_];

        return *components[i].get();
    }
};

struct component_manager {
    std::unordered_map<uint, std::size_t> id_to_code;
    std::unordered_map<std::size_t, uint> code_to_id;
    std::unordered_map<std::size_t, std::shared_ptr<clist>> component_lists;
    
    template<typename type>
    void register_component() {
        std::size_t code = typeid(type).hash_code();

        if(!code_to_id.contains(code)) {
            uint i = id_to_code.size();
            
            id_to_code.emplace(i, code);
            code_to_id.emplace(code, i);
            
            component_lists.emplace(code, std::shared_ptr<component_list<type>>(new component_list<type>));
        }
    }
    
    template<typename type>
    component_list<type>& get_component_array() {
        std::size_t ti = typeid(type).hash_code();
        return *(component_list<type>*)(component_lists[ti].get());
    }
    
    void delete_components(uint entity_, axiom::signature signature_) {
        for(int i = 0; i < MAX_COMPONENTS; ++i) {
            std::bitset<MAX_COMPONENTS> a = (signature)1 << i;
            std::bitset<MAX_COMPONENTS> b = signature_;
            
            if((b & a) == a) {
                std::size_t ti = id_to_code[i];
                auto& list = component_lists[ti];
                
                list->remove_component(entity_);
            }
        }
    }
    
    template<typename type>
    void insert_component(uint entity_, axiom::signature& signature_, type t) {
        std::size_t code = typeid(type).hash_code();
        uint i = code_to_id[code];

        axiom::signature s2 = axiom::signature(1) << i;
        signature_ |= s2;
        
        (*(component_list<type>*)(component_lists[code].get())).insert_component(entity_, t);
    }

    template<typename type>
    void insert_component_move(uint entity_, axiom::signature& signature_, type&& t) {
        std::size_t code = typeid(type).hash_code();
        uint i = code_to_id[code];

        axiom::signature s2 = axiom::signature(1) << i;
        signature_ |= s2;
        
        (*(component_list<type>*)(component_lists[code].get())).insert_component_move(entity_, std::move(t));
    }
    
    template<typename type>
    type& get_component(uint entity_) {
        auto& cl = get_component_array<type>();
        
        return cl.get_component(entity_);
    }
};

struct collector {
    std::bitset<MAX_COMPONENTS> signature = 0;
    bool greedy = true;

    std::unordered_set<uint> entities;
};

struct ecs;

struct system {
    std::vector<collector> collectors;
    ecs* ecs;
    
    virtual void call() {};
    virtual void init() {};
};

struct system_manager {
    std::unordered_map<std::size_t, axiom::system*> systems;
    std::vector<std::size_t> call_order;
    
    template<typename type>
    void register_system(type& system, axiom::ecs* ecs) {
        std::size_t code = typeid(type).hash_code();
        
        type* ptr = &system;
        ptr->ecs = ecs;

        systems.emplace(code, ptr);
        call_order.push_back(code);

        ptr->init();
    }
};

struct ecs;

struct core {
    axiom::ecs* ecs;
};

extern axiom::core global_core;

struct ecs {
    entity_manager entity_manager_;
    component_manager component_manager_;
    system_manager system_manager_;

    std::unordered_map<std::string, collector> collectors;

    double delta_time;
    double prev_time = FLT_MAX;

    ecs();
    
    template<typename type>
    void register_component() {
        component_manager_.register_component<type>();
    }
    
    template<typename type>
    void register_system(type& system) {
        system_manager_.register_system<type>(system, this);
    }

    void create_collector(std::string key, collector& collector) {
        collectors.emplace(key, std::move(collector));
    }
    
    int insert_entity() {
        return entity_manager_.insert_entity();
    }    
    
    template<typename type>
    std::bitset<MAX_COMPONENTS> update_signature() {
        register_component<type>();

        std::size_t code = typeid(type).hash_code();
        return axiom::signature(1) << component_manager_.code_to_id[code];
    }
    
    template<typename type>
    void update_signature(std::bitset<MAX_COMPONENTS>& a) {
        register_component<type>();

        std::size_t code = typeid(type).hash_code();
        a |= axiom::signature(1) << component_manager_.code_to_id[code];
    }

    template<typename type>
    bool has_component(uint entity_) {
        return (entity_manager_.signatures[entity_] & update_signature<type>()) != axiom::signature(0);
    }
    
    template<typename type>
    bool has_system() {
        return system_manager_.systems.contains(typeid(type).hash_code());
    }

    template<typename type>
    type& get_component(uint entity_) {
        return component_manager_.get_component<type>(entity_);
    }
    
    template<typename type>
    void insert_component(uint entity_, type component) {
        register_component<type>();

        component_manager_.insert_component(entity_, entity_manager_.signatures[entity_], component);
        axiom::signature new_signature = entity_manager_.signatures[entity_];
        
        for(auto& s : system_manager_.systems) {
            bool remove = false;
            for(collector& c : s.second->collectors) {
                if(!remove) {
                    axiom::signature s_signature = c.signature;
                    if((new_signature & s_signature) == s_signature) {
                        if(c.greedy) remove = true;
                        if(c.entities.find(entity_) == c.entities.end()) {
                            c.entities.emplace(entity_);
                        }
                    }
                } else {
                    axiom::signature s_signature = c.signature;
                    if((new_signature & s_signature) == s_signature) {
                        if(c.entities.find(entity_) != c.entities.end()) {
                            c.entities.erase(entity_);
                        }
                    }
                }
            }
        }
        
        for(auto& [key, c] : collectors)  {
            axiom::signature s_signature = c.signature;
            if((new_signature & s_signature) == s_signature) {
                if(c.entities.find(entity_) == c.entities.end()) {
                    c.entities.emplace(entity_);
                }
            }            
        }
    }

    template<typename type>
    void insert_component_move(uint entity_, type&& component) { 
        using raw_type = std::remove_reference_t<type>;
        register_component<raw_type>();

        component_manager_.insert_component_move(entity_, entity_manager_.signatures[entity_], std::move(component));
        axiom::signature new_signature = entity_manager_.signatures[entity_];
        
        for(auto& s : system_manager_.systems) {
            bool remove = false;
            for(collector& c : s.second->collectors) {
                if(!remove) {
                    axiom::signature s_signature = c.signature;
                    if((new_signature & s_signature) == s_signature) {
                        if(c.greedy) remove = true;
                        if(c.entities.find(entity_) == c.entities.end()) {
                            c.entities.emplace(entity_);
                        }
                    }
                } else {
                    axiom::signature s_signature = c.signature;
                    if((new_signature & s_signature) == s_signature) {
                        if(c.entities.find(entity_) != c.entities.end()) {
                            c.entities.erase(entity_);
                        }
                    }
                }
            }
        }

        for(auto& [key, c] : collectors)  {
            axiom::signature s_signature = c.signature;
            if((new_signature & s_signature) == s_signature) {
                if(c.entities.find(entity_) == c.entities.end()) {
                    c.entities.emplace(entity_);
                }
            }            
        }
    }
    
    void erase_entity(uint entity_) {
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

    template<typename type> 
    type& get_system() {
        return *(type*)system_manager_.systems[typeid(type).hash_code()];
    }

    void make_active() {
        global_core.ecs = this;
    }

    void do_frame() {
        make_active();
        
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
};

template<typename type>
type& get_component(uint entity) {
    return axiom::global_core.ecs->get_component<type>(entity);
};

template<typename type>
type& get_system() {
    return axiom::global_core.ecs->get_system<type>();
};

uint insert_entity();

template<typename type>
void insert_component(uint entity, const type& component) {
    return axiom::global_core.ecs->insert_component(entity, component);
};

template<typename type>
void insert_component(uint entity, type&& component) {
    return axiom::global_core.ecs->insert_component_move(entity, component);
};

template<typename type>
bool has_component(uint entity) {
    return axiom::global_core.ecs->has_component<type>(entity);
};

double delta_time();

axiom::collector& get_collector(std::string name);

}
