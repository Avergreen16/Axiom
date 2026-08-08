#include <physics-3d/mesh.hpp>
#include <physics-3d/collider.hpp>

#include <iostream>

namespace axiom {

struct mesh_face {
    int i0;
    int i1;
    int i2;
};

void create_mesh(std::vector<vertex_element3d> elements, std::vector<vec3>* vertices, std::vector<uint>* indices) {
    std::vector<vec3> points = {support(vec3(1.0f, 0.0f, 0.0f), elements), support(vec3(-1.0f, 0.0f, 0.0f), elements), support(vec3(0.0f, 1.0f, 1.0f), elements)};

    std::vector<mesh_face> faces = {mesh_face(0, 1, 2), mesh_face(2, 1, 0)};
    
    while(true) {
        std::vector<mesh_face> new_faces;

        std::vector<uint> to_erase;

        float threshold = 0.01f;

        int index = 0;

        for(mesh_face& face : faces) {
            vec3 normal = normalize(cross(points[face.i0] - points[face.i2], points[face.i1] - points[face.i2]));

            vec3 s = support(normal, elements);

            //

            if(dot(s, normal) > dot(points[face.i0], normal) + threshold) {
                std::vector<uint> to_erase2 = {};

                int index2 = index;

                for(uint i = 0; i < faces.size(); ++i) {
                    mesh_face& face = faces[i];
                    
                    vec3 nn = normalize(cross(points[face.i0] - points[face.i2], points[face.i1] - points[face.i2]));
                    
                    if(dot(s, nn) > dot(points[face.i0], nn)) {
                        to_erase2.push_back(i);
                    }
                }

                std::unordered_set<uint> indices;
                std::unordered_map<uint, uint> edges;
                for(int i : to_erase2) {
                    mesh_face& face = faces[i];

                    uint a = face.i0;
                    uint b = face.i1;
                    uint c = face.i2;
                    
                    indices.insert(a);
                    indices.insert(b);
                    indices.insert(c);

                    uint e0 = ((glm::max(a, b) << 16) | glm::min(a, b));
                    uint e1 = ((glm::max(b, c) << 16) | glm::min(b, c));
                    uint e2 = ((glm::max(c, a) << 16) | glm::min(c, a));

                    if(edges.contains(e0)) ++edges[e0];
                    else edges[e0] = 1;
                    
                    if(edges.contains(e1)) ++edges[e1];
                    else edges[e1] = 1;
                    
                    if(edges.contains(e2)) ++edges[e2];
                    else edges[e2] = 1;
                }

                vec3 center = vec3(0.0f);
                for(uint i : indices) center += points[i];
                center /= indices.size();

                vec3 dir = s - center;

                for(auto [e, n] : edges) {
                    if(n == 1) {
                        uint e0 = e & 0xfFFF;
                        uint e1 = e >> 16;

                        vec3 nn = normalize(cross(points[e0] - s, points[e1] - s));
                        if(dot(nn, dir) < 0.0f) std::swap(e0, e1);
                        
                        new_faces.push_back(mesh_face(e0, e1, points.size()));
                    }
                }

                to_erase.insert(to_erase.end(), to_erase2.begin(), to_erase2.end());

                points.push_back(s);

                break;
            }

            ++index;
        }

        if(index == faces.size()) break;

        for(auto iter = to_erase.rbegin(); iter != to_erase.rend(); ++iter) {
            faces.erase(faces.begin() + *iter);
        }
        faces.insert(faces.end(), new_faces.begin(), new_faces.end());

        std::cout << faces.size() << " " << points.size() << "\n";
    }

    for(vec3 point : points) {
        vertices->push_back(point);
    }
    
    for(mesh_face& face : faces) {
        indices->push_back(face.i0);
        indices->push_back(face.i1);
        indices->push_back(face.i2);
    }
    
    //

    /*

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
    */
}

/*
void create_mesh(axiom::collider2d& collider, std::vector<vec3>* surface) {
    std::vector<vec2>* s = new std::vector<vec2>();

    for(auto& shape : collider.shapes) {
        create_mesh(shape.vertices, s);
        for(vec2& v : *s) v = shape.position + shape.orientation * v;
        
        surface->insert(surface->end(), s->begin(), s->end());
    }
    
    delete s;
}
*/

}