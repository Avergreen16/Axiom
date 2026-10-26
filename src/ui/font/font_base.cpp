#include <ui/font/font.hpp>

namespace axiom {
    
bool operator==(const glyph_key& a, const glyph_key& b) {
    return a.glyph == b.glyph && a.size == b.size && a.phase_x == b.phase_x && a.phase_y == b.phase_y;
}

bool glyph_key_less::operator()(const glyph_key& a, const glyph_key& b) const {
    return a.glyph < b.glyph || a.glyph == b.glyph && (a.size < b.size || a.size == b.size && (a.phase_x < b.phase_x || a.phase_x == b.phase_x && a.phase_y < b.phase_y));
}

}