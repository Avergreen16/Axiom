#include <unordered_set>

#include "hash.hpp"
#include "octree.hpp"

namespace axiom {
    
std::vector<std::shared_ptr<Octree_cell>> compute_octree(int power, int min_power, vec3 rel_pos, float split_factor, int max_power) {
    if(max_power == -1) max_power = power;

    std::vector<std::shared_ptr<Octree_cell>> ret;
    std::vector<std::shared_ptr<Octree_cell>> current_cells = {std::make_shared<Octree_cell>(Octree_cell({0, 0, 0, power}))};
    std::vector<std::shared_ptr<Octree_cell>> new_cells;

    float offset = pow(2.0f, power) * 0.5f;

    while(true) {
        for(std::shared_ptr<Octree_cell>& cell : current_cells) {
            int p2 = cell->id.w;
            if(p2 == min_power) continue;

            float side_length = pow(2.0f, p2);
            vec3 center = vec3(cell->id.xyz()) * side_length - offset + side_length * 0.5f;

            vec3 rel = center - rel_pos;
            float max_coord = glm::max(glm::max(abs(rel.x), abs(rel.y)), abs(rel.z));
            rel /= max_coord;

            if(length(rel) * max_coord <= side_length * split_factor || p2 > max_power) {
                ivec4 base = ivec4(cell->id.xyz() * 2, cell->id.w - 1);

                cell->children.push_back(std::make_shared<Octree_cell>(Octree_cell(base, cell)));
                cell->children.push_back(std::make_shared<Octree_cell>(Octree_cell(base + ivec4(1, 0, 0, 0), cell)));
                cell->children.push_back(std::make_shared<Octree_cell>(Octree_cell(base + ivec4(0, 1, 0, 0), cell)));
                cell->children.push_back(std::make_shared<Octree_cell>(Octree_cell(base + ivec4(1, 1, 0, 0), cell)));
                cell->children.push_back(std::make_shared<Octree_cell>(Octree_cell(base + ivec4(0, 0, 1, 0), cell)));
                cell->children.push_back(std::make_shared<Octree_cell>(Octree_cell(base + ivec4(1, 0, 1, 0), cell)));
                cell->children.push_back(std::make_shared<Octree_cell>(Octree_cell(base + ivec4(0, 1, 1, 0), cell)));
                cell->children.push_back(std::make_shared<Octree_cell>(Octree_cell(base + ivec4(1, 1, 1, 0), cell)));

                cell->is_leaf = false;

                new_cells.insert(new_cells.end(), cell->children.begin(), cell->children.end());
            }
        }

        ret.insert(ret.end(), current_cells.begin(), current_cells.end());

        if(new_cells.size() == 0) break;
        current_cells = new_cells;
        new_cells.clear();
    }

    return ret;
}

std::vector<ivec4> get_children(ivec4 cell) {
    ivec4 base = ivec4(cell.xyz() * 2, cell.w - 1);

    return {
        base,
        base + ivec4(1, 0, 0, 0),
        base + ivec4(0, 1, 0, 0),
        base + ivec4(1, 1, 0, 0),
        base + ivec4(0, 0, 1, 0),
        base + ivec4(1, 0, 1, 0),
        base + ivec4(0, 1, 1, 0),
        base + ivec4(1, 1, 1, 0),
    };
}


std::vector<ivec4> get_parents(ivec4 cell) {
    std::vector<ivec4> ret;
    ivec4 base = cell;

    while(!(base.x == 0 && base.y == 0 && base.z == 0)) {
        base.x /= 2;
        base.y /= 2;
        base.z /= 2;
        base.w += 1;
        ret.push_back(base);
    }

    return ret;
}

void octree_insert_cell(std::vector<std::shared_ptr<Octree_cell>>& octree, std::unordered_set<ivec4, hash_coord>& set, ivec4 insert, int power) {
    auto split_leaf = [&](std::shared_ptr<Octree_cell> cell) {
        ivec4 base = ivec4(cell->id.xyz() * 2, cell->id.w - 1);

        cell->children.push_back(std::make_shared<Octree_cell>(Octree_cell(base, cell)));
        cell->children.push_back(std::make_shared<Octree_cell>(Octree_cell(base + ivec4(1, 0, 0, 0), cell)));
        cell->children.push_back(std::make_shared<Octree_cell>(Octree_cell(base + ivec4(0, 1, 0, 0), cell)));
        cell->children.push_back(std::make_shared<Octree_cell>(Octree_cell(base + ivec4(1, 1, 0, 0), cell)));
        cell->children.push_back(std::make_shared<Octree_cell>(Octree_cell(base + ivec4(0, 0, 1, 0), cell)));
        cell->children.push_back(std::make_shared<Octree_cell>(Octree_cell(base + ivec4(1, 0, 1, 0), cell)));
        cell->children.push_back(std::make_shared<Octree_cell>(Octree_cell(base + ivec4(0, 1, 1, 0), cell)));
        cell->children.push_back(std::make_shared<Octree_cell>(Octree_cell(base + ivec4(1, 1, 1, 0), cell)));
        
        set.insert(base);
        set.insert(base + ivec4(1, 0, 0, 0));
        set.insert(base + ivec4(0, 1, 0, 0));
        set.insert(base + ivec4(1, 1, 0, 0));
        set.insert(base + ivec4(0, 0, 1, 0));
        set.insert(base + ivec4(1, 0, 1, 0));
        set.insert(base + ivec4(0, 1, 1, 0));
        set.insert(base + ivec4(1, 1, 1, 0));

        octree.insert(octree.end(), cell->children.begin(), cell->children.end());

        cell->is_leaf = false;
    };

    std::vector<ivec4> path(power - insert.w + 1);
    
    ivec4 c = insert;
    for(int i = insert.w; i <= power; ++i) {
        path[power - i] = c;
        ivec3 cc = c.xyz() / 2;
        c.x = cc.x;
        c.y = cc.y;
        c.z = cc.z;
        c.w = i;
    }

    std::shared_ptr<Octree_cell> current_cell = octree[0];
    for(int i = power - 1; i > insert.w; --i) {
        if(current_cell->is_leaf) split_leaf(current_cell);

        ivec4 c = path[power - i];
        ivec3 base = ivec3(current_cell->id.xyz() * 2);
        base = c.xyz() - base;

        uint32_t j = base.x + base.y * 2 + base.z * 4;

        current_cell = current_cell->children[j];
    }
}

std::vector<std::shared_ptr<Octree_cell>> compute_octree_with_neighbors(int power, int min_power, vec3 rel_pos, float split_factor, float split_add, int max_power) {
    if(max_power == -1) max_power = power;

    std::unordered_set<ivec4, hash_coord> set;

    std::vector<std::shared_ptr<Octree_cell>> octree;
    std::vector<std::shared_ptr<Octree_cell>> current_cells = {std::make_shared<Octree_cell>(Octree_cell({0, 0, 0, power}))};
    std::vector<std::shared_ptr<Octree_cell>> new_cells;

    float offset = pow(2.0f, power) * 0.5f;

    while(true) {
        for(std::shared_ptr<Octree_cell>& cell : current_cells) {
            int p2 = cell->id.w;
            if(p2 == min_power) continue;

            float side_length = pow(2.0f, p2);
            vec3 center = vec3(cell->id.xyz()) * side_length - offset + side_length * 0.5f;

            vec3 rel = center - rel_pos;
            float max_coord = glm::max(glm::max(abs(rel.x), abs(rel.y)), abs(rel.z));
            rel /= max_coord;

            if(length(rel) * max_coord <= side_length * split_factor + split_add || p2 > max_power) {
                ivec4 base = ivec4(cell->id.xyz() * 2, cell->id.w - 1);

                cell->children.push_back(std::make_shared<Octree_cell>(Octree_cell(base, cell)));
                cell->children.push_back(std::make_shared<Octree_cell>(Octree_cell(base + ivec4(1, 0, 0, 0), cell)));
                cell->children.push_back(std::make_shared<Octree_cell>(Octree_cell(base + ivec4(0, 1, 0, 0), cell)));
                cell->children.push_back(std::make_shared<Octree_cell>(Octree_cell(base + ivec4(1, 1, 0, 0), cell)));
                cell->children.push_back(std::make_shared<Octree_cell>(Octree_cell(base + ivec4(0, 0, 1, 0), cell)));
                cell->children.push_back(std::make_shared<Octree_cell>(Octree_cell(base + ivec4(1, 0, 1, 0), cell)));
                cell->children.push_back(std::make_shared<Octree_cell>(Octree_cell(base + ivec4(0, 1, 1, 0), cell)));
                cell->children.push_back(std::make_shared<Octree_cell>(Octree_cell(base + ivec4(1, 1, 1, 0), cell)));
                
                set.insert(base);
                set.insert(base + ivec4(1, 0, 0, 0));
                set.insert(base + ivec4(0, 1, 0, 0));
                set.insert(base + ivec4(1, 1, 0, 0));
                set.insert(base + ivec4(0, 0, 1, 0));
                set.insert(base + ivec4(1, 0, 1, 0));
                set.insert(base + ivec4(0, 1, 1, 0));
                set.insert(base + ivec4(1, 1, 1, 0));

                cell->is_leaf = false;

                new_cells.insert(new_cells.end(), cell->children.begin(), cell->children.end());
            }
        }

        octree.insert(octree.end(), current_cells.begin(), current_cells.end());

        if(new_cells.size() == 0) break;
        current_cells = new_cells;
        new_cells.clear();
    }

    std::vector<ivec4> neighbors = {
        ivec4(-1, 0, 0, 0),
        ivec4(1, 0, 0, 0),
        ivec4(0, -1, 0, 0),
        ivec4(0, 1, 0, 0),
        ivec4(0, 0, -1, 0),
        ivec4(0, 0, 1, 0),
    };

    /*
    std::vector<ivec4> neighbors = {
        ivec4(1, 1, 1, 0),
        ivec4(0, 1, 1, 0),
        ivec4(-1, 1, 1, 0),
        ivec4(1, 0, 1, 0),
        ivec4(0, 0, 1, 0),
        ivec4(-1, 0, 1, 0),
        ivec4(1, -1, 1, 0),
        ivec4(0, -1, 1, 0),
        ivec4(-1, -1, 1, 0),
        
        ivec4(1, 1, 0, 0),
        ivec4(0, 1, 0, 0),
        ivec4(-1, 1, 0, 0),
        ivec4(1, 0, 0, 0),
        //ivec4(0, 0, 0, 0), self
        ivec4(-1, 0, 0, 0),
        ivec4(1, -1, 0, 0),
        ivec4(0, -1, 0, 0),
        ivec4(-1, -1, 0, 0),
        
        ivec4(1, 1, -1, 0),
        ivec4(0, 1, -1, 0),
        ivec4(-1, 1, -1, 0),
        ivec4(1, 0, -1, 0),
        ivec4(0, 0, -1, 0),
        ivec4(-1, 0, -1, 0),
        ivec4(1, -1, -1, 0),
        ivec4(0, -1, -1, 0),
        ivec4(-1, -1, -1, 0),
    };
    */

    for(int i = 0; i < octree.size(); ++i) {
        std::shared_ptr<Octree_cell> cell = octree[i];

        if(cell->id.w < power && cell->is_leaf) {
            uint32_t m = uint32_t(1) << (power - cell->id.w);
            
            for(ivec4 c : neighbors) {
                ivec4 n = cell->id + c;

                if(!(n.x < 0 || n.y < 0 || n.z < 0 || n.x >= m || n.y >= m || n.z >= m)) {
                    ivec4 n2 = ivec4(n.xyz() / 2, n.w + 1);

                    if(!set.contains(n2)) octree_insert_cell(octree, set, n2, power);
                }
            }
        }
    }

    return octree;
}

}