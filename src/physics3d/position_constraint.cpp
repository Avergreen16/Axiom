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
            vec3 vela = ca->get_velocity(ra);
            vec3 acca = ca->get_acceleration(ra);
            vec3 angvela = ca->get_angular_velocity();

            //

            float inertia = inertia_a[i];

            vec3 jacobian = v;

            float jdotvel = dot(v, cross(angvela, cross(angvela, ra)));
            
            float xdot = dot(jacobian, vela);
            float x = baumgarte[i];
            float zeta = 1.25f;
            float omega = 2.0f * axiom::pi * 20.0f;

            float geometric = -jdotvel;
            float damping = -2.0f * zeta * omega * xdot;
            float spring = -omega * omega * x;

            float b = geometric + damping + spring;
            float Ja = dot(jacobian, acca);

            float L = b - Ja;
            L /= inertia;
            
            vec3 force = v * L;
            ca->apply_force(force, ra);

            ++i;
        }
    } else {
        uint32_t i = 0;
        for(vec3 v : vs) {
            vec3 vela = ca->get_velocity(ra);
            vec3 velb = cb->get_velocity(rb);
            vec3 acca = ca->get_acceleration(ra);
            vec3 accb = cb->get_acceleration(rb);

            vec3 angvela = ca->get_angular_velocity();
            vec3 angvelb = cb->get_angular_velocity();

            //

            float inertia = inertia_a[i] + inertia_b[i];

            vec3 jacobian = v;

            float jdotvela = dot(v, cross(angvela, cross(angvela, ra)));
            float jdotvelb = dot(v, cross(angvelb, cross(angvelb, rb)));
            float jdotvel = jdotvela - jdotvelb;
            
            float xdot = dot(jacobian, vela - velb);
            float x = baumgarte[i];
            float zeta = 1.25f;
            float omega = 2.0f * axiom::pi * 20.0f;

            float geometric = -jdotvel;
            float damping = -2.0f * zeta * omega * xdot;
            float spring = -omega * omega * x;

            float b = geometric + damping + spring;
            float Ja = dot(jacobian, acca - accb);

            float L = b - Ja;
            L /= inertia;
            
            vec3 force = v * L;
            ca->apply_force(force, ra);
            cb->apply_force(-force, rb);

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