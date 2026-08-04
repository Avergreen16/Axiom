#include <physics-2d/mesh.hpp>
#include <physics-2d/collider.hpp>

#include <iostream>

namespace axiom {

struct mesh_face {
    int i0;
    int i1;

    bool finished = false;
};

void create_mesh(std::vector<vertex_element> vertices, std::vector<vec2>* perimeter, std::vector<vec2>* area) {
    std::vector<vec2> points = {support(vec2(1.0f, 0.0f), vertices), support(vec2(-1.0f, 0.0f), vertices)};

    std::vector<mesh_face> faces = {mesh_face(0, 1, false), mesh_face(1, 0, false)};
    
    while(true) {
        std::vector<mesh_face> new_faces;

        bool finish = true;

        for(mesh_face& face : faces) {
            if(!face.finished) {
                vec2 normal = vec2(points[face.i0] - points[face.i1]);
                normal = vec2(-normal.y, normal.x);

                vec2 s = support(normal, vertices);

                bool overwrite = false;

                if(dot(s, normal) < dot(points[face.i0], normal) + 0.00001f) {
                    face.finished = true;
                } else overwrite = true;

                if(overwrite) {
                    points.push_back(s);
                    new_faces.push_back(mesh_face(face.i0, points.size() - 1, false));
                    new_faces.push_back(mesh_face(points.size() - 1, face.i1, false));

                    finish = false;
                }
            }
        }

        faces = new_faces;

        if(finish) break;
    }

    vec2 c = vec2(0.0f);
    for(vec2 v : points) c += v;
    c /= points.size();

    std::sort(points.begin(), points.end(), 
        [c](const vec2& a, const vec2& b) {
            float theta_a = std::atan2(a.y - c.y, a.x - c.x);
            float theta_b = std::atan2(b.y - c.y, b.x - c.x);

            return theta_a < theta_b;
        }
    );

    if(perimeter != nullptr) {
        std::vector<vec2> vs;
        
        for(int i = 0; i < points.size(); ++i) {
            vs.push_back(points[i]);
            vs.push_back(points[(i + 1) % points.size()]);
        }

        *perimeter = vs;
    }

    if(area != nullptr) {
        vec2 center = vec2(0.0f);

        for(int i = 0; i < points.size(); ++i) {
            center += points[i];
        }

        center /= float(points.size());

        //
        
        std::vector<vec2> vs;

        for(int i = 0; i < points.size(); ++i) {
            vs.push_back(points[i]);
            vs.push_back(points[(i + 1) % points.size()]);
            vs.push_back(center);
        }

        *area = vs;
    }
}


void create_mesh(axiom::collider2d& collider, std::vector<vec2>* perimeter, std::vector<vec2>* area) {
    std::vector<vec2>* p = new std::vector<vec2>();
    std::vector<vec2>* a = nullptr;
    if(area) a = new std::vector<vec2>();

    for(auto& shape : collider.shapes) {
        create_mesh(shape.vertices, p, a);
        for(vec2& v : *p) v = shape.position + shape.orientation * v;
        if(area) for(vec2& v : *a) v = shape.position + shape.orientation * v;
        
        perimeter->insert(perimeter->end(), p->begin(), p->end());
        if(area) area->insert(area->end(), a->begin(), a->end());
    }
    
    delete p;
    if(a) delete a;
}

}