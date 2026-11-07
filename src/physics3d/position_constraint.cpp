#include <physics3d/position_constraint.hpp>
#include <include/core.hpp>
#include <physics3d/system.hpp>

namespace axiom {

void position_constraint::before() {
    ca = &axiom::ecs.get_component<collider3d>(a);
    ta = &axiom::ecs.get_component<transform3d>(a);
    if(b != NULL_ENTITY) {
        cb = &axiom::ecs.get_component<collider3d>(b);
        tb = &axiom::ecs.get_component<transform3d>(b);
    }

    //

    baumgarte.resize(vs.size());
    inertia_a.resize(vs.size());
    inertia_b.resize(vs.size());
    lambda.resize(vs.size(), 0.0f);

    ra = ta->orientation * va;
    wa = ra + ta->position;

    mat3 inverse_tensor_a = ta->orientation * ca->inverse_inertia_tensor * transpose(ta->orientation);
    ca->iit_rot = inverse_tensor_a;
    

    for(int i = 0; i < vs.size(); ++i) {
        vec3 v = vs[i];
        vec3 d = cross(ra, v); 

        inertia_a[i] = 1.0f / ca->mass + dot(d, (ta->orientation * ca->inverse_inertia_tensor * transpose(ta->orientation)) * d);
    }

    if(b != NULL_ENTITY) {
        mat3 inverse_tensor_b = tb->orientation * cb->inverse_inertia_tensor * transpose(tb->orientation);
        cb->iit_rot = inverse_tensor_b;
        
        rb = tb->orientation * vec3(vb);
        wb = rb + tb->position;

        for(int i = 0; i < vs.size(); ++i) {
            vec3 v = vs[i];
            vec3 d = cross(rb, v); 
            
            inertia_b[i] = 1.0f / cb->mass + dot(d, (tb->orientation * cb->inverse_inertia_tensor * transpose(tb->orientation)) * d);
        }
    } else {
        wb = vb;
    }

    for(int i = 0; i < vs.size(); ++i) {
        vec3 diff = wa - wb;
        float dd = dot(diff, vs[i]);
        baumgarte[i] = dd;
    }

    //
    
    uint i = 0;
    for(vec3 v : vs) {
        vec3 impulse = v * lambda[i];

        ca->apply_impulse(impulse, ra);
        if(b != NULL_ENTITY) {
            cb->apply_impulse(-impulse, rb);
        }

        ++i;
    }
}

void position_constraint::solve(float delta_time) {
    axiom::physics_system3d& physics_system3d = axiom::ecs.get_system<axiom::physics_system3d>();

    if(b == NULL_ENTITY) {
        //std::cout << "CALLED!\n";
        uint32_t i = 0;
        for(vec3 v : vs) {
            float diff = baumgarte[i] * spring * (1.0f / delta_time);

            vec3 vel = ca->get_velocity(ra);

            float inertia = inertia_a[i];

            float vv = dot(vel, v);

            float L = -vv - diff;
            L /= inertia;
            L -= softness * lambda[i];

            //vec2 limits = vec2(0.0f, FLT_MAX);

            float new_lambda = lambda[i] + L;
            //new_lambda = glm::clamp(new_lambda, limits.x, limits.y);
            L = new_lambda - lambda[i];
            lambda[i] = new_lambda;

            vec3 impulse = v * L;

            ca->apply_impulse(impulse, ra);

            ++i;
        }
    } else {
        uint32_t i = 0;
        for(vec3 v : vs) {
            float diff = baumgarte[i] * spring * (1.0f / delta_time);

            vec3 vel = ca->get_velocity(ra) - cb->get_velocity(rb);

            float inertia = inertia_a[i] + inertia_b[i];

            float vv = dot(vel, v);

            float L = -vv - diff;
            L /= inertia;
            L -= softness * lambda[i];

            //vec2 limits = vec2(0.0f, FLT_MAX);

            float new_lambda = lambda[i] + L;
            //new_lambda = glm::clamp(new_lambda, limits.x, limits.y);
            L = new_lambda - lambda[i];
            lambda[i] = new_lambda;

            vec3 impulse = v * L;

            ca->apply_impulse(impulse, ra);
            cb->apply_impulse(-impulse, rb);

            ++i;
        }
    }
}

void position_constraint::after() {
    
}

}

/*
0 0 9.53674e-07 
0 0 9.53674e-07 
0 0 9.53674e-07 
0 0 0 
4.76837e-07 0 9.53674e-07 
-4.76837e-07 -2.38419e-07 9.53674e-07 
0 -2.38419e-07 9.53674e-07 
0 4.76837e-07 9.53674e-07 
4.76837e-07 4.76837e-07 9.53674e-07 
0 0 9.53674e-07 
0 0 9.53674e-07 
0 0 9.53674e-07 
0 0 0 
4.76837e-07 0 9.53674e-07 
-4.76837e-07 -2.38419e-07 9.53674e-07 
0 -2.38419e-07 9.53674e-07 
0 4.76837e-07 9.53674e-07 
4.76837e-07 4.76837e-07 9.53674e-07 
..
nan nan nan nan 
*/