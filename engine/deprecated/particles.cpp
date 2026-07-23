#include "particles.hpp"
#include "physics.hpp"
#include "input.hpp"
#include "core.hpp"

vec3 get_star_color(float f) {
    float f2 = f * 3.0f;
    f = fract(f2);
    
    if(f2 < 1.0f) {
        return vec3(1.0f, f, 0.0f);
    } else if(f2 < 2.0f) {
        return vec3(1.0f, 1.0f, f);
    } else {
        return vec3(1.0f - f, 1.0f - f, 1.0f);
    }
}

Particle_system::Particle_system() {
    Signature s = ecs.update_signature<Transform>();
    collectors.push_back(Collector(s, false));  

    uint32_t num_stars = 0x800;
    float radius = pow(2, 54);

    Random random(0xEEEE);

    for(int i = 0; i < num_stars; ++i) {
        vec3 dir;
        while(true) {
            dir = {random(), random(), random()};
            if(length(dir) < 1.0f) break;
        }
        
        vec3 color = get_star_color(abs(random()));
        color = color * 0.5f + 0.5f;
        float r = abs(random()) * 3.0f;

        float light_year = pow(2.0f, 48.0f);

        pvec3 center_pos = pvec3(light_year * 50000.0f, light_year * -30000.0f, light_year * 40000.0f);

        insert_star(pvec3(dir * radius) + center_pos, vec4(color, 1.0f), r);
    }
}

void Particle_system::call() {
    Input_system& input_system = ecs.get_system<Input_system>();

    for(int i = 0; i < particles.size(); ++i) {
        Particle& p = particles[i];
        if(p.current_lifetime > p.max_lifetime) {
            particles.erase(particles.begin() + i);
            --i;
        }

        p.current_lifetime += core.delta_time;
        p.position += p.velocity * (float)core.delta_time;
        p.velocity.z -= 10 * p.gravity * core.delta_time;
    }
    //uint32_t player_entity = *collectors[1].entities.begin();
    //Transform& ta = ecs.get_component<Transform>(player_entity);
    for(vec3 v : ps) {
        Particle p;

        p.position = pvec3(v) + rel_pos;
        p.max_lifetime = 1000000.0;
        p.current_lifetime = 0.0;
        p.start_color = vec4(1.0f, 0.35f, 0.35f, 1.0f);
        p.end_color = vec4(1.0f, 0.35f, 0.35f, 1.0f);
        p.start_size = vec2(10.0f);
        p.end_size = vec2(10.0f);
        p.gravity = false;
        p.settings = S_CONSTANT_SIZE_BIT;
        p.tex_coord = vec2(5.0f, 0.0f);
        p.tex_size = vec2(5.0f, 5.0f);
        p.velocity = vec3(0, 0, 0);

        icons.emplace_back(std::move(p));
    }
    
    if(input_system.debug_mode) {
        Physics_system& ps = ecs.get_system<Physics_system>();

        pvec3 grav_pos = ps.gravity_center;
        insert_icon(grav_pos, vec4(1.0f, 0.0f, 1.0f, 1.0f), vec2(10.0f), vec4(31, 0, 5, 5), S_CONSTANT_SIZE_BIT | S_DEPTH_BIT);
        
        /*
        for(uint32_t entity : collectors[0].entities) {
            Transform& t = ecs.get_component<Transform>(entity);
            if(ecs.has_component<Collider>(entity)) {
                Collider& c = ecs.get_component<Collider>(entity);

                if(true) {
                    vec3 color;
                    vec2 tex_coord;
                    vec2 tex_size;

                    if(c.is_static) {
                        color = vec3(0.25f, 0.25f, 1);
                        tex_coord = vec2(21, 0);
                        tex_size = vec2(5, 5);
                    } else {
                        if(c.allow_rotation) {
                            color = vec3(0.25f, 1, 0.25f);
                            tex_coord = vec2(16, 0);
                            tex_size = vec2(5, 5);
                        } else {
                            color = vec3(1, 0.25f, 1);
                            tex_coord = vec2(16, 0);
                            tex_size = vec2(5, 5);
                        }
                    }

                    insert_icon(t.position, vec4(color, 1.0f), vec2(10.0f), vec4(tex_coord, tex_size), S_CONSTANT_SIZE_BIT | S_DEPTH_BIT);
                }
            } else {
                vec3 color;
                vec2 tex_coord;
                vec2 tex_size;

                color = vec3(1.0f, 0.25f, 0.25f);
                tex_coord = vec2(21, 0);
                tex_size = vec2(5, 5);

                insert_icon(t.position, vec4(color, 1.0f), vec2(10.0f), vec4(tex_coord, tex_size), S_CONSTANT_SIZE_BIT | S_DEPTH_BIT);
            }
        }
        */
    }
}

void Particle_system::insert_icon(pvec3 pos, vec4 color, vec2 size, vec4 tex_range, uint32_t settings) {
    Particle p;

    p.position = pos;
    p.max_lifetime = 0.0f;
    p.current_lifetime = 0.0f;
    p.start_color = color;
    p.end_color = color;
    p.start_size = size;
    p.end_size = size;
    p.gravity = false;
    p.settings = settings;
    p.tex_coord = tex_range.xy();
    p.tex_size = tex_range.zw();
    p.velocity = vec3(0, 0, 0);

    icons.emplace_back(std::move(p));
}

void Particle_system::insert_particle(pvec3 pos, float lifetime, vec4 start_color, vec4 end_color, vec2 start_size, vec2 end_size, bool gravity, vec4 tex_range, uint32_t settings, vec3 velocity) {
    Particle p;

    p.position = pos;
    p.max_lifetime = lifetime;
    p.current_lifetime = 0.0f;
    p.start_color = start_color;
    p.end_color = end_color;
    p.start_size = start_size;
    p.end_size = end_size;
    p.gravity = false;
    p.settings = settings;
    p.tex_coord = tex_range.xy();
    p.tex_size = tex_range.zw();
    p.velocity = velocity;

    particles.emplace_back(std::move(p));
}

void Particle_system::insert_star(pvec3 pos, vec4 color, float brightness) {
    Particle p;

    p.position = pos;
    p.max_lifetime = FLT_MAX;
    p.current_lifetime = 0.0f;
    p.start_color = color;
    p.end_color = color;
    p.gravity = false;
    p.settings = S_CONSTANT_SIZE_BIT;
    p.velocity = vec3(0.0f);

    Star s;
    s.brightness = brightness;
    s.particle = p;

    stars.emplace_back(std::move(s));
}