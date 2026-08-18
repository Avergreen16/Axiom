#include <memory>
#include <cmath>

#define GLM_FORCE_SWIZZLE
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/glm.hpp"
#include "glm/gtx/matrix_transform_2d.hpp"
#include "glm/gtx/transform.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtx/orthonormalize.hpp"

using ivec2 = glm::ivec2;
using ivec3 = glm::ivec3;
using ivec4 = glm::ivec4;

using vec2 = glm::vec2;
using vec3 = glm::vec3;
using vec4 = glm::vec4;

using uvec2 = glm::uvec2;
using uvec3 = glm::uvec3;
using uvec4 = glm::uvec4;

using mat2 = glm::mat2;
using mat3 = glm::mat3;
using mat4 = glm::mat4;

using uint = unsigned int;
using ulong = uint64_t;

namespace axiom {
    
struct Octree_cell {
    ivec4 id;
    std::shared_ptr<Octree_cell> parent = nullptr;
    std::vector<std::shared_ptr<Octree_cell>> children;

    bool is_leaf = true;
};

std::vector<std::shared_ptr<Octree_cell>> compute_octree(int power, int min_power, vec3 rel_pos, float split_factor, int max_power = -1);
//void octree_insert_cell(std::vector<std::shared_ptr<Octree_cell>>& octree, ivec4 insert, int power);
std::vector<std::shared_ptr<Octree_cell>> compute_octree_with_neighbors(int power, int min_power, vec3 rel_pos, float split_factor, float split_add = 0.0f, int max_power = -1);
std::vector<ivec4> get_children(ivec4 cell);
std::vector<ivec4> get_parents(ivec4 cell);

}