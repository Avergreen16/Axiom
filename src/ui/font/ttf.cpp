#include <ui/font/ttf.hpp>

namespace axiom {

ttf_font process_ttf(std::string filepath) {
    axiom::binary_asset bin = axiom::binary_asset::load(filepath);
    uint cursor = 0;

    ttf_font font;
    
    auto read_byte = [&bin, &cursor]() {
        byte ret = bin.data[cursor];
        ++cursor;
        return ret;    
    };
    
    auto read_short_be = [&bin, &cursor]() {
        byte ret_a = bin.data[cursor];
        byte ret_b = bin.data[cursor + 1];
        cursor += 2;

        uint16_t ret = (uint16_t(ret_a) << 8) | uint16_t(ret_b);
        int16_t r;
        memcpy(&r, &ret, 2);
        return r;    
    };

    auto read_ushort_be = [&bin, &cursor]() {
        byte ret_a = bin.data[cursor];
        byte ret_b = bin.data[cursor + 1];
        cursor += 2;

        return (uint16_t(ret_a) << 8) | uint16_t(ret_b);    
    };

    auto read_uint_be = [&bin, &cursor]() {
        byte ret_a = bin.data[cursor];
        byte ret_b = bin.data[cursor + 1];
        byte ret_c = bin.data[cursor + 2];
        byte ret_d = bin.data[cursor + 3];
        cursor += 4;

        return (uint32_t(ret_a) << 24) | (uint32_t(ret_b) << 16) | (uint32_t(ret_c) << 8) | uint32_t(ret_d);
    };
    
    auto read_ulong_be = [&bin, &cursor]() {
        byte ret_a = bin.data[cursor];
        byte ret_b = bin.data[cursor + 1];
        byte ret_c = bin.data[cursor + 2];
        byte ret_d = bin.data[cursor + 3];
        byte ret_e = bin.data[cursor + 4];
        byte ret_f = bin.data[cursor + 5];
        byte ret_g = bin.data[cursor + 6];
        byte ret_h = bin.data[cursor + 7];
        cursor += 8;

        return (uint64_t(ret_a) << 56) | (uint64_t(ret_b) << 48) | (uint64_t(ret_c) << 40) | (uint64_t(ret_d) << 32) |
            (uint64_t(ret_e) << 24) | (uint64_t(ret_f) << 16) | (uint64_t(ret_g) << 8) | uint64_t(ret_h);
    };

    //

    uint32_t version = read_uint_be();
    uint16_t num_tables = read_ushort_be();

    struct ttf_table {
        uint32_t checksum;
        uint32_t offset;
        uint32_t length;
    };

    std::unordered_map<std::string, ttf_table> tables;

    for(int i = 0; i < num_tables; ++i) {
        cursor = 12 + i * 16;

        std::string tag;
        
        tag += read_byte();
        tag += read_byte();
        tag += read_byte();
        tag += read_byte();

        uint32_t checksum = read_uint_be();
        uint32_t offset = read_uint_be();
        uint32_t length = read_uint_be();

        /*
        std::cout << "tag: " << tag << " | " << 
            "checksum: " << checksum << " | " <<
            "offset: " << offset << " | " << 
            "length: " << length << std::endl;
        */

        ttf_table table{
            .checksum = checksum,
            .offset = offset,
            .length = length
        };

        tables.emplace(tag, table);
    }

    //

    int16_t loca_format;

    {
        ttf_table table = tables["head"];

        cursor = table.offset;

        uint16_t major_version = read_ushort_be();
        uint16_t minor_version = read_ushort_be();
        uint32_t revision = read_uint_be();

        uint32_t checksum = read_uint_be();
        uint32_t magic_number = read_uint_be();

        uint16_t flags = read_ushort_be();
        uint16_t units_per_em = read_ushort_be();

        font.base_unit = units_per_em;
        //std::cout << font.base_unit << "\n";

        int64_t created_timestamp = read_ulong_be();
        int64_t modified_timestamp = read_ulong_be();

        int16_t xmin = read_short_be();
        int16_t ymin = read_short_be();
        int16_t xmax = read_short_be();
        int16_t ymax = read_short_be();

        font.bounding_box = {xmin, ymin, xmax, ymax};

        //std::cout << "BOUNDING BOX " << font.bounding_box << "\n";
        
        uint16_t mac_style = read_ushort_be();

        uint16_t smallest_readable_size = read_ushort_be();
        //std::cout << "SMALLEST SIZE " << smallest_readable_size << "\n";
        int16_t font_direction = read_short_be();
        loca_format = read_short_be(); // <- what we want
        int16_t glyph_format = read_short_be();
    }

    uint16_t num_metrics_hmtx;

    {
        ttf_table table = tables["hhea"];

        cursor = table.offset;

        uint16_t major_version = read_ushort_be();
        uint16_t minor_version = read_ushort_be();

        int16_t ascender = read_short_be();
        int16_t descender = read_short_be();
        int16_t line_gap = read_short_be();

        uint16_t max_advance = read_ushort_be();
        int16_t min_left_bearing = read_short_be();
        int16_t min_right_bearing = read_short_be();
        int16_t max_extent = read_short_be();
        
        int16_t caret_slope_rise = read_short_be();
        int16_t caret_slope_run = read_short_be();
        int16_t caret_offset = read_short_be();

        //reserved
        read_short_be();
        read_short_be();
        read_short_be();
        read_short_be();
        
        int16_t metric_data_format = read_short_be();
        num_metrics_hmtx = read_ushort_be();
        
        font.ascender = ascender;
        font.descender = descender;
        font.line_gap = line_gap;

        std::cout << font.ascender << " " << font.descender << " " << font.line_gap << "\n";

        font.line_height = ascender - descender + line_gap;
    }

    uint16_t num_glyphs;

    {
        ttf_table table = tables["maxp"];

        cursor = table.offset;

        uint32_t version = read_uint_be();

        if(version = 0x00005000) {
            num_glyphs = read_ushort_be();
        } else if(version = 0x00010000) {
            num_glyphs = read_ushort_be();

            uint16_t max_points = read_ushort_be();
            uint16_t max_contours = read_ushort_be();
            uint16_t max_composite_points = read_ushort_be();
            uint16_t max_composite_contours = read_ushort_be();
            uint16_t max_zones = read_ushort_be();
            uint16_t max_twilight_points = read_ushort_be();
            uint16_t max_storage = read_ushort_be();
            uint16_t max_function_defs = read_ushort_be();
            uint16_t max_instruction_devs = read_ushort_be();
            uint16_t max_stack_elements = read_ushort_be();
            uint16_t max_size_of_instructions = read_ushort_be();
            uint16_t max_component_elements = read_ushort_be();
            uint16_t max_component_depth = read_ushort_be();
        }
    }

    {
        ttf_table table = tables["cmap"];

        cursor = table.offset;

        uint16_t version = read_ushort_be(); // always 0
        uint16_t num_tables = read_ushort_be();

        uint prev_offset = cursor;
        for(int i = 0; i < num_tables; ++i) {
            cursor = prev_offset;
            uint16_t platform = read_ushort_be();
            uint16_t encoding = read_ushort_be();
            uint32_t offset = read_uint_be();

            prev_offset = cursor;

            if(platform == 0) {
                cursor = table.offset + offset;

                uint16_t format = read_ushort_be();
                uint16_t length = read_ushort_be();
                uint16_t language = read_ushort_be();
                uint16_t seg_count_x2 = read_ushort_be();
                uint16_t search_range = read_ushort_be();
                uint16_t entry_selector = read_ushort_be();
                uint16_t range_shift = read_ushort_be();

                //

                uint seg_count = seg_count_x2 / 2.0f;

                // end code is an array of seg_count of uint16_t
                
                std::vector<uint16_t> end_code;
                std::vector<uint16_t> start_code;
                std::vector<int16_t> id_delta;
                std::vector<uint16_t> id_range_offset;
                std::vector<uint32_t> addresses;

                end_code.reserve(seg_count);
                start_code.reserve(seg_count);
                id_delta.reserve(seg_count);
                id_range_offset.reserve(seg_count);

                for(int i = 0; i < seg_count; ++i) {
                    end_code.push_back(read_ushort_be());
                }

                read_ushort_be(); // pad
                
                for(int i = 0; i < seg_count; ++i) {
                    start_code.push_back(read_ushort_be());
                }
                for(int i = 0; i < seg_count; ++i) {
                    id_delta.push_back(read_short_be());
                }
                for(int i = 0; i < seg_count; ++i) {
                    addresses.push_back(cursor);
                    id_range_offset.push_back(read_ushort_be());
                }

                //std::cout << seg_count << " SEGS\n";

                //

                uint start_cursor = cursor;

                for(int i = 0; i < seg_count; ++i) {
                    uint16_t start = start_code[i];
                    uint16_t end = end_code[i];

                    //std::cout << start << " " << end << "\n";

                    for(int j = start; j <= end; ++j) {
                        uint glyph_id;

                        if(id_range_offset[i] == 0) {
                            glyph_id = j + id_delta[i];
                        } else {
                            uint address = addresses[i];
                            address += int(id_range_offset[i]) + (j - int(start)) * 2;
                            cursor = address;

                            glyph_id = read_ushort_be();

                            if(glyph_id != 0) glyph_id += id_delta[i];
                        }
                        glyph_id &= 0xFFFF;
                        
                        ttf_glyph glyph;
                        glyph.glyph_id = glyph_id;
                        glyph.codepoint = j;

                        font.glyph_map.emplace(j, glyph_id);
                        font.glyphs.emplace(glyph_id, glyph);
                    }
                }
            }
        }
    }

    {
        ttf_table table = tables["loca"];

        cursor = table.offset;

        //std::cout << "FORMAT " << loca_format << "\n";

        uint prev = 0;
        if(loca_format == 0) {
            for(int i = 0; i < num_glyphs; ++i) {
                if(!font.glyphs.contains(i)) {
                    font.glyphs.emplace(i, ttf_glyph());
                }

                uint16_t offset = read_ushort_be();

                uint address = uint(offset) * 2;
                font.glyphs[i].glyph_address = address;

                if(address == prev) font.glyphs[i - 1].glyph_address = 0xFFFFFFFF;
                prev = address;
            }
        } else if(loca_format == 1) {
            for(int i = 0; i < num_glyphs; ++i) {
                if(!font.glyphs.contains(i)) {
                    font.glyphs.emplace(i, ttf_glyph());
                }

                uint32_t offset = read_uint_be();

                uint address = uint(offset);
                font.glyphs[i].glyph_address = address;

                if(address == prev) font.glyphs[i - 1].glyph_address = 0xFFFFFFFF;
                prev = address;
            }
        }
    }    

    {
        ttf_table table = tables["glyf"];

        for(auto& [k, glyph] : font.glyphs) {
            if(glyph.glyph_address == 0xFFFFFFFF) continue;

            cursor = table.offset + glyph.glyph_address;

            // read header

            int16_t num_contours = read_short_be();
            int16_t xmin = read_short_be();
            int16_t ymin = read_short_be();
            int16_t xmax = read_short_be();
            int16_t ymax = read_short_be();

            glyph.bounding_box = {xmin, ymin, xmax, ymax};

            std::vector<ttf_point> points;

            if(num_contours > 0) {
                uint start_cursor = cursor;

                cursor += num_contours * 2;
                cursor -= 2;

                uint16_t num_vertices = read_short_be() + 1;

                uint16_t instruction_length = read_ushort_be();
                glyph.bytecode_glyf.reserve(instruction_length);
                for(int i = 0; i < instruction_length; ++i) {
                    glyph.bytecode_glyf.push_back(read_byte());
                }

                std::vector<byte> flags;
                flags.reserve(num_vertices);

                for(int i = 0; i < num_vertices; ++i) {
                    byte flag = read_byte();
                    flags.push_back(flag);

                    if((flag & 0x08) != 0x00) {
                        byte repeat = read_byte();
                        for(int i = 0; i < repeat; ++i) flags.push_back(flag);

                        i += int(repeat);
                    }
                }

                points.resize(num_vertices);

                vec2 prev = vec2(0.0f);

                for(int i = 0; i < num_vertices; ++i) {
                    byte flag = flags[i];

                    // if 0x02 is set, x coord is 1 byte long
                    // and sign is determined by the 0x10 bit

                    // else, 
                    // if the 0x10 bit is set, the x coordinate is the SAME as the previous one,
                    // and no element is added to the list (just copy it)
                    // ... if the 0x10 bit is NOT set (meaning both the 0x02 and 0x10 bits are NOT set), then the x coord is a signed 2-byte integer

                    // the number will be relative to the previous x coordinate
                    
                    if((flag & 0x02) != 0x00) { 
                        if((flag & 0x10) != 0x00) { // x coord is 1 byte long and positive
                            byte b = read_byte();
                            int16_t offset = b;

                            prev.x += offset;
                        } else { // x coord is 1 byte long and negative
                            byte b = read_byte();

                            int16_t offset = b;
                            offset = -offset;

                            prev.x += offset;
                        }
                    } else {
                        if((flag & 0x10) != 0x00) { // x coordinate is the same as previous, duplicate previous value
                            
                        } else { // x coordinate is a signed 2 byte integer
                            int16_t offset = read_short_be();
                            
                            prev.x += offset;
                        }
                    }
                    
                    points[i].point.x = prev.x;
                    if(flag & 0x01) points[i].curve = true;
                }

                //
                
                for(int i = 0; i < num_vertices; ++i) {
                    byte flag = flags[i];

                    // if 0x04 is set, y coord is 1 byte long
                    // and sign is determined by the 0x20 bit

                    // else, 
                    // if the 0x20 bit is set, the y coordinate is the SAME as the previous one,
                    // and no element is added to the list (just copy it)
                    // ... if the 0x20 bit is NOT set (meaning both the 0x04 and 0x20 bits are NOT set), then the y coord is a signed 2-byte integer

                    // the number will be relative to the previous y coordinate

                    // if 0x01 is set, the point is ON the curve, else it is OFF the curve
                    
                    if((flag & 0x04) != 0x00) { 
                        if((flag & 0x20) != 0x00) { // y coord is 1 byte long and positive
                            byte b = read_byte();
                            int16_t offset = b;

                            prev.y += offset;
                        } else { // y coord is 1 byte long and negative
                            byte b = read_byte();

                            int16_t offset = b;
                            offset = -offset;

                            prev.y += offset;
                        }
                    } else {
                        if((flag & 0x20) != 0x00) { // y coordinate is the same as previous, duplicate previous value
                            
                        } else { // y coordinate is a signed 2 byte integer
                            int16_t offset = read_short_be();
                            
                            prev.y += offset;
                        }
                    }
                    
                    points[i].point.y = prev.y;
                    if(flag & 0x01) points[i].curve = true;
                }

                cursor = start_cursor;

                uint16_t start_index = 0;
                for(int i = 0; i < num_contours; ++i) {
                    uint16_t end_index = read_ushort_be();

                    ttf_contour contour;
                    contour.points = std::vector<ttf_point>(points.begin() + start_index, points.begin() + (end_index + 1));

                    for(int i = 0; i < contour.points.size(); ++i) {
                        int ia = i;
                        int ib = (i + 1) % contour.points.size();

                        ttf_point& point_a = contour.points[ia];
                        ttf_point& point_b = contour.points[ib];

                        if(!point_a.curve && !point_b.curve) {
                            ttf_point new_point;
                            new_point.curve = true;
                            new_point.point = (point_a.point + point_b.point) * 0.5f;

                            contour.points.insert(contour.points.begin() + ib, new_point);
                            ++i;
                        }
                    }

                    for(int i = 0; i < contour.points.size(); ++i) {
                        int ia = i;
                        int ib = (i + 1) % contour.points.size();
                        int ic = (i + 2) % contour.points.size();
                        
                        ttf_point& point_a = contour.points[ia];
                        ttf_point& point_b = contour.points[ib];
                        ttf_point& point_c = contour.points[ic];

                        if(point_a.curve && !point_b.curve && point_c.curve) {
                            ttf_bezier bezier;
                            bezier.a = ia;
                            bezier.b = ib;
                            bezier.c = ic;

                            contour.beziers.push_back(bezier);
                        }
                    }

                    uint offset = 0;
                    for(auto& bezier : contour.beziers) {
                        uint pa = bezier.a + offset;
                        uint pb = bezier.b + offset;
                        uint pc = bezier.c + offset;
                        if(bezier.b < 2) pb = bezier.b;
                        if(bezier.c < 2) pc = bezier.c;

                        ttf_point& point_a = contour.points[pa];
                        ttf_point& point_b = contour.points[pb];
                        ttf_point& point_c = contour.points[pc];

                        std::vector<ttf_point> new_points;
                        int num_points = 3;

                        for(int i = 0; i < num_points; ++i) {
                            float frac = float(i + 1) / (num_points + 2);

                            vec2 pa = point_a.point * (1.0f - frac) + point_b.point * frac;
                            vec2 pb = point_b.point * (1.0f - frac) + point_c.point * frac;

                            vec2 point = pa * (1.0f - frac) + pb * frac;

                            new_points.push_back(ttf_point(point, true));
                        }

                        contour.points.erase(contour.points.begin() + pb);
                        contour.points.insert(contour.points.begin() + pb, new_points.begin(), new_points.end());
                        offset += num_points - 1;
                    }

                    glyph.contours.push_back(contour);

                    start_index = end_index + 1;
                }

                //std::cout << "num vertices: " << num_vertices << "\n";
            } else if(num_contours < 0) { // compound glyph
                while(true) {
                    uint16_t flags = read_ushort_be();
                    uint16_t glyph_index = read_ushort_be();

                    // flags:
                    // 0x0001 -> on: arguments are 16 bits, off: arguments are 8 bits
                    // 0x0002 -> on: arguments are signed coordinates, off: arguments are unsigned point indices 
                    // this means that point a on the composite glyph constructed so far, is aligned with point b on this component glyph

                    // 0x0004 -> if 0x0002 is set (so arguments are coordinates), then... on: the coordinate arguments are rounded to the nearest grid line,
                    // grid lines are used for rendering text at small sizes (so the SDF implementation will not use this)
                    // off: arguments are NOT rounded to the nearest grid line

                    // 0x0008 -> the component has a simple scale factor
                    // 0x0020 -> there is another component glyph after this one
                    // 0x0040 -> there are two scale factors, one for the x axis and another for the y axis
                    // 0x0080 -> there is a 2x2 matrix transformation factor that must be applied to the component glyph
                    // 0x0100 -> there are instructions following the end of the last glyph
                    // 0x0200 -> use the metrics (advance width, right/left side bearing) of this component glyph for the entire glyph
                    // 0x0400 -> the components of this compound glyph overlap (remember, each component glyph can itself be a composite glyph)... also this flag is not required to be set
                    // 0x0800 -> the component's offset is transformed by the component's transform (ignored if 0x0002 is not set)
                    // 0x1000 -> the component's offset is NOT transformed by the component's transform (ignored if 0x0002 is not set)
                
                    if((flags & 0x0001) && (flags & 0x0002)) { // arguments are signed 16 bit coords
                        int16_t arg_a = read_short_be();
                        int16_t arg_b = read_short_be();

                        //

                    } else if(!(flags & 0x0001) && (flags & 0x0002)) { // arguments are signed 8 bit coords
                        int8_t arg_a = read_byte();
                        int8_t arg_b = read_byte();

                        //

                    } else if((flags & 0x0001) && !(flags & 0x0002)) { // arguments are unsigned 16 bit indices
                        uint16_t arg_a = read_ushort_be();
                        uint16_t arg_b = read_ushort_be();

                        //

                    } else if(!(flags & 0x0001) && !(flags & 0x0002)) { // arguments are unsigned 8 bit indices
                        byte arg_a = read_byte();
                        byte arg_b = read_byte();

                        //

                    }

                    if(flags & 0x0008) { // simple scale factor
                        uint16_t scale_factor = read_ushort_be();
                    } else if(flags & 0x0040) { // x and y scale factors
                        uint16_t scale_factor_x = read_ushort_be();
                        uint16_t scale_factor_y = read_ushort_be();
                    } else if(flags & 0x0080) { // 2x2 transformation
                        uint16_t nxx = read_ushort_be();
                        uint16_t nxy = read_ushort_be();
                        uint16_t nyx = read_ushort_be();
                        uint16_t nyy = read_ushort_be();
                    }

                    if(!(flags & 0x0020)) { // there is NOT another component glyph after this one
                        if(flags & 0x0100) {
                            uint16_t num_instructions = read_ushort_be();
                            std::vector<byte> instructions;
                            for(uint i = 0; i < num_instructions; ++i) {
                                instructions.push_back(read_byte());
                            }
                        }

                        //step = cursor - offset;
                        break;
                    }
                }
            } else if(num_contours == 0) {

            }
        }
    }
        
    {
        ttf_table table = tables["hmtx"];

        cursor = table.offset;

        uint16_t num_metrics = num_metrics_hmtx;
        uint16_t num_bearings = num_glyphs - num_metrics;

        uint16_t prev_advance;

        for(int i = 0; i < num_metrics; ++i) {
            uint16_t advance = read_ushort_be();
            int16_t bearing = read_short_be();

            font.glyphs[i].h_advance = advance;
            font.glyphs[i].h_bearing = bearing;

            prev_advance = advance;
        }

        for(int i = 0; i < num_bearings; ++i) {
            uint ii = i + num_metrics;
            
            int16_t bearing = read_short_be();
            
            font.glyphs[i].h_advance = prev_advance;
            font.glyphs[i].h_bearing = bearing;
        }
    }

    //

    auto create_glyph = [&font](ttf_glyph& glyph, int units, float frac) -> axiom::texture_asset {
        auto intersect = [](vec2 a0, vec2 a1, vec2 p0, vec2& p1) {
            vec2 dir_p = vec2(1.0f, 0.0f);

            float a = (p0.y - a0.y) / (a1.y - a0.y);

            p1 = a0 + (a1 - a0) * a;
            if(glm::isnan(a) && p0.y == a0.y) {
                vec2 n = a1 - a0;
                float d = (p0.x - a0.x) / (a1.x - a0.x);
                p1 = a0 + (a1 - a0) * glm::clamp(d, 0.0f, 1.0f);
            }

            return (glm::isnan(a) && p0.y == a0.y) || (!glm::isnan(a) && a >= 0.0f && a <= 1.0f); //!(a0.x == a1.x) && 
        };
        

        //

        float descender = font.descender;

        vec2 pmin = glyph.bounding_box.xy();
        float y_offset = pmin.y - descender;
        y_offset = y_offset / float(font.base_unit) * float(units);
        float offset = y_offset - floor(y_offset);
        offset = offset * float(font.base_unit) / float(units);
        pmin.y -= offset;

        //vec2 pmin2 = glm::floor(pmin / float(font.base_unit) * float(units));
        //pmin2 = pmin2 / float(units) * float(font.base_unit);
        //pmin.y += pmin2.y - pmin.y;

        vec2 psize = vec2(glyph.bounding_box.zw()) - pmin;
        ivec2 size = glm::ceil(psize / float(font.base_unit) * float(units) + vec2(frac, 0.0f));
        psize = (vec2(size) / float(units)) * float(font.base_unit);
        
        std::vector<byte> colors(size.x * size.y * 4);
        for(int k = 0; k < size.x * size.y; ++k) {
            vec2 pt = vec2(k % size.x - frac, k / size.x);
            //float w = glm::max(glyph.bounding_box.z - glyph.bounding_box.x, glyph.bounding_box.w - glyph.bounding_box.y) * 0.25f;

            float min_width = glm::min(size.x, size.y);

            uint supersample = 5;

            float frac = 0.0f;

            for(int ii = 0; ii < supersample * supersample; ++ii) {
                vec2 offset = vec2(ii % supersample + 0.5f, ii / supersample + 0.5f) / float(supersample);
                vec2 pt_o = pt + offset;

                pt_o = (pt_o / vec2(size)) * psize + pmin;

                int count = 0;
                float min_dist = axiom::max_float;
                
                for(int i = 0; i < glyph.contours.size(); ++i) {
                    auto& contour = glyph.contours[i];

                    for(int j = 0; j < contour.points.size(); ++j) {
                        int a = j;
                        int b = (j + 1) % contour.points.size();

                        auto& point_a = contour.points[a];
                        auto& point_b = contour.points[b];

                        vec2 pt_s;

                        bool did_intersect = intersect(point_a.point, point_b.point, pt_o, pt_s);

                        if(pt_s.x < pt_o.x) did_intersect = false;

                        if(did_intersect) {
                            if(point_a.point.y < point_b.point.y || (point_a.point.y == point_b.point.y && point_a.point.x < point_b.point.x)) {
                                ++count;
                            } else {
                                --count;
                            }
                        }

                        //

                        /*
                        vec2 a0 = point_a.point;
                        vec2 a1 = point_b.point;

                        vec2 r = normalize(a1 - a0);
                        vec2 rx = vec2(r.y, -r.x);

                        vec2 rel = pt - a0;
                        rel -= rx * dot(rel, rx);

                        float l = dot(r, rel);
                        l = glm::clamp(l, 0.0f, length(a1 - a0));

                        vec2 point = a0 + r * l;

                        float dist = length((pt_o - point) / size);

                        min_dist = glm::min(dist, min_dist);
                        */
                    }
                }
                
                if(count != 0) {
                    frac += 1.0f;
                }
            }

            frac = (frac / (supersample * supersample)) * 0xFF;

            colors[k * 4] = 0xFF;
            colors[k * 4 + 1] = 0xFF;
            colors[k * 4 + 2] = 0xFF;
            colors[k * 4 + 3] = frac;

            /*
            float f = min_dist * 255;
            if(count != 0) {
                colors[k * 4] = glm::clamp((int)glm::round(127.5f + f), 0x00, 0xFF);
                colors[k * 4 + 1] = 0x00;
                colors[k * 4 + 2] = 0x00;
                colors[k * 4 + 3] = 0xFF;
            } else {
                colors[k * 4] = glm::clamp((int)glm::round(127.5f - f), 0x00, 0xFF);
                colors[k * 4 + 1] = 0x00;
                colors[k * 4 + 2] = 0x00;
                colors[k * 4 + 3] = 0xFF;
            }
            */
        }

        axiom::texture_asset asset = axiom::texture_asset::load(colors, size, 4);

        return asset;
    };

    //

    std::vector<byte> bytes;
    ivec2 img_size = ivec2(0);

    uint em_size = 13;

    uint width = 1024;
    std::vector<uint> heights(1024, 0);
    cursor = 0;

    uint frac = 4;

    uint ctr = 0;
    for(auto& [id, glyph] : font.glyphs) {
        if(glyph.contours.size()) {
            for(int j = 0; j < frac; ++j) {
                float f = float(j) / frac;

                axiom::texture_asset asset = create_glyph(glyph, em_size, f);

                if(cursor + asset.size.x > width) cursor = 0;

                uint height = 0;
                for(int i = 0; i < asset.size.x; ++i) {
                    uint h = heights[i + cursor];
                    height = glm::max(height, h);
                }
                
                for(int i = 0; i < asset.size.x; ++i) {
                    heights[i + cursor] = height + asset.size.y;
                }

                uint new_height = glm::max(img_size.y, int(height + asset.size.y) + 1);

                bytes.resize(width * new_height * 4, 0x00);

                vec2 dst_pos = vec2(cursor, height);

                ivec4 atlas = ivec4(dst_pos, dst_pos + vec2(asset.size));
                glyph.atlas.push_back(atlas);
                glyph.frac.push_back(f);

                for(int j = 0; j < asset.size.x * asset.size.y; ++j) {
                    ivec2 src_pos = {j % asset.size.x, j / asset.size.x};
                    ivec2 ndst_pos = ivec2(dst_pos) + src_pos;

                    int src_i = src_pos.y * asset.size.x + src_pos.x;
                    int dst_i = ndst_pos.y * width + ndst_pos.x;

                    bytes[dst_i * 4] = asset.data[src_i * 4];
                    bytes[dst_i * 4 + 1] = asset.data[src_i * 4 + 1];
                    bytes[dst_i * 4 + 2] = asset.data[src_i * 4 + 2];
                    bytes[dst_i * 4 + 3] = asset.data[src_i * 4 + 3];
                }

                cursor += asset.size.x;

                img_size.x = glm::max(img_size.x, (int)cursor);
                img_size.y = glm::max(img_size.y, (int)new_height);
            }
        }

        //++ctr;
        //if(ctr > 10) break;
    }

    axiom::texture_asset asset = axiom::texture_asset::load(bytes, {width, img_size.y}, 4);

    asset.save("output/font-test.png");

    font.texture.load(asset, axiom::texture_format::RGBA8, 0);

    return font;
}

}