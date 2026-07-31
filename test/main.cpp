#include <window.hpp>
#include <math.hpp>
#include <ecs.hpp>
#include <render.hpp>
#include <ui.hpp>
#include <platform.hpp>
#include <utilities.hpp>
#include <nlohmann/json.hpp>

#include <iostream>

using json = nlohmann::json;

struct message {
    std::string sender;
    std::string message;
    ulong timestamp;
    ulong index;
};

struct chat_system : axiom::system {
    std::vector<message> messages;
    std::unordered_map<ulong, ulong> message_map;

    ivec2 message_range = ivec2(0, 0);

    //

    ulong root_id;
    bool enabled = false;

    vec3 player_color = vec3(0.0625f);
    vec3 self_color = axiom::color_rose;
    vec3 addie_color = axiom::color_blue;

    bool rebuild_flag = false;
    
    std::vector<json> json_files;
    
    std::unordered_map<ulong, ulong> pop_headers();
    void push_headers(std::unordered_map<ulong, ulong> headers);

    void insert_message(ulong message_root, std::string sender, std::string message, ulong timestamp, vec3 color, ulong map_index, int index = -1);
    void remove_message(ulong message_root, ulong id, ulong map_index);

    void change_range() {
        auto& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();

        std::vector<ulong> existing_messages;
        for(auto [message_id, widget_id] : message_map) existing_messages.push_back(message_id);

        std::sort(existing_messages.begin(), existing_messages.end());

        //

        auto& root_w = ui_system.widgets[root_id];
        axiom::scroll_widget* scroll_w = dynamic_cast<axiom::scroll_widget*>(ui_system.widgets[root_w->parent].get());

        //

        float bottom;
        float top;
        float start_height;
        
        float buf = 100.0f;
        float extents = 600.0f;

        float bottom_target = scroll_w->size.y + extents;
        float top_target = extents;

        float start_scroll = scroll_w->scroll_pos;
        float start_size = root_w->size.y;
        
        //std::cout << "TOP ADD\n";

        bottom = scroll_w->scroll_pos + root_w->size.y;
        top = -scroll_w->scroll_pos;
        start_height = root_w->size.y;

        if(top < top_target - buf) { // add top
            float delta;
            int message_id;
            if(existing_messages.size()) message_id = existing_messages[0];
            else message_id = messages.size();

            while(true) {
                message_id -= 1;
                message& mes = messages[message_id];
                
                //

                auto headers = pop_headers();

                vec3 color = player_color;
                if(mes.sender == "Averie") color = self_color;
                else if(mes.sender == "Addie") color = addie_color;

                insert_message(root_id, mes.sender, mes.message, mes.timestamp, color, message_id, 0);

                push_headers(headers);

                //

                for(int i = 0; i < 2; ++i) ui_system.measure(root_id);

                float new_height = root_w->size.y;
                float delta = new_height - start_height;
                start_height = new_height;
                
                float new_top = top + delta;
                top = new_top;

                if(new_top > top_target + buf) break;

                scroll_w->anchor_mode = 0;
            }
        }
        
        //std::cout << "TOP REMOVE\n";
        
        bottom = scroll_w->scroll_pos + root_w->size.y;
        top = -scroll_w->scroll_pos;
        start_height = root_w->size.y;

        if(top > top_target + buf) { // remove top
            float delta;
            int message_index = 0;

            while(true) {
                int message_id = existing_messages[message_index];
                ulong widget_id = message_map[message_id];

                auto& widget = ui_system.widgets[widget_id];

                //

                auto headers = pop_headers();

                remove_message(root_id, widget_id, message_id);
                existing_messages.erase(existing_messages.begin());

                push_headers(headers);
                
                for(int i = 0; i < 2; ++i) ui_system.measure(root_id);

                //
                
                float new_height = root_w->size.y;
                
                float delta = new_height - start_height;
                start_height = new_height;
                
                float new_top = top + delta;
                top = new_top;

                if(new_top <= top_target - buf) {
                    message& mes = messages[message_id];
                    
                    auto headers = pop_headers();

                    vec3 color = player_color;
                    if(mes.sender == "Averie") color = self_color;
                    else if(mes.sender == "Addie") color = addie_color;

                    insert_message(root_id, mes.sender, mes.message, mes.timestamp, color, message_id, 0);

                    push_headers(headers);
                
                    for(int i = 0; i < 2; ++i) ui_system.measure(root_id);

                    break;
                }

                scroll_w->anchor_mode = 0;
            }
        }

        //std::cout << "BOTTOM ADD\n";
        
        bottom = scroll_w->scroll_pos + root_w->size.y;
        top = -scroll_w->scroll_pos;
        start_height = root_w->size.y;

        if(bottom < bottom_target - buf) { // add bottom
            float delta;
            int message_id;
            if(existing_messages.size()) message_id = existing_messages.back();
            else message_id = messages.size() - 1;

            while(true) {
                message_id += 1;
                if(message_id >= messages.size()) break;

                message& mes = messages[message_id];
                
                //

                auto headers = pop_headers();

                vec3 color = player_color;
                if(mes.sender == "Averie") color = self_color;
                else if(mes.sender == "Addie") color = addie_color;

                insert_message(root_id, mes.sender, mes.message, mes.timestamp, color, message_id, root_w->children.size());
                existing_messages.push_back(message_id);

                push_headers(headers);

                //

                for(int i = 0; i < 2; ++i) ui_system.measure(root_id);
                
                float new_height = root_w->size.y;
                float delta = new_height - start_height;
                start_height = new_height;

                float new_bottom = bottom + delta;
                bottom = new_bottom;

                if(new_bottom > bottom_target + buf) break;

                scroll_w->anchor_mode = 0;
            }   
        }
        
        //std::cout << "BOTTOM REMOVE\n";

        bottom = scroll_w->scroll_pos + root_w->size.y;
        top = -scroll_w->scroll_pos;
        start_height = root_w->size.y;

        if(bottom > bottom_target + buf) { // remove bottom
            std::cout << bottom << " " << bottom_target << "\n";
            float delta;

            while(true) {
                if(existing_messages.size() == 0) break;

                int message_id = existing_messages.back();

                ulong widget_id = message_map[message_id];

                auto& widget = ui_system.widgets[widget_id];

                auto headers = pop_headers();

                remove_message(root_id, widget_id, message_id);

                existing_messages.erase(existing_messages.end() - 1);

                push_headers(headers);
                
                for(int i = 0; i < 2; ++i) ui_system.measure(root_id);

                //
                
                float new_height = root_w->size.y;
                float delta = new_height - start_height;
                start_height = new_height;
                
                float new_bottom = bottom + delta;
                bottom = new_bottom;
                
                if(new_bottom < bottom_target - buf) {
                    message& mes = messages[message_id];
                    
                    auto headers = pop_headers();

                    vec3 color = player_color;
                    if(mes.sender == "Averie") color = self_color;
                    else if(mes.sender == "Addie") color = addie_color;

                    insert_message(root_id, mes.sender, mes.message, mes.timestamp, color, message_id, root_w->children.size());
                    existing_messages.push_back(message_id);

                    push_headers(headers);
                
                    for(int i = 0; i < 2; ++i) ui_system.measure(root_id);

                    break;
                }

                scroll_w->anchor_mode = 0;
            }
        }

        //

        if(std::find(root_w->children.begin(), root_w->children.end(), scroll_w->anchor_widget) == root_w->children.end()) {
            scroll_w->anchor_mode = 2;
            std::cout << "NOT FOUND\n";
        }
        

        /* else if(top < top_target - buf) { // add top
            float delta;
            int message_id;
            if(existing_messages.size()) message_id = existing_messages[0];
            else message_id = messages.size() - 1;

            while(true) {
                message& mes = messages[message_id];
                message_id -= 1;
                
                //

                auto headers = pop_headers();

                vec3 color = player_color;
                if(mes.sender == "Averie") color = self_color;
                else if(mes.sender == "Addie") color = addie_color;

                insert_message(root_id, mes.sender, mes.message, mes.timestamp, color, message_id, 0);

                push_headers(headers);

                //

                for(int i = 0; i < 2; ++i) ui_system.measure(root_id);
                float new_height = root_w->size.y;

                float delta = new_height - start_height;
                float new_top = top + delta;

                std::cout << " SIZE -> " << top << " " << new_top << "\n";

                if(new_top > top_target + buf) break;

                scroll_w->anchor_mode = 0;
            }
        }*/

        //while(true) {
            //auto headers = pop_headers();


            //std::cout << bottom << " " << top << "\n";
            /*
            message& mes = messages[add];

            vec3 color = player_color;
            if(mes.sender == "Averie") color = self_color;
            else if(mes.sender == "Addie") color = addie_color;

            if(new_range.x < message_range.x) { // insert at top
                insert_message(root_id, mes.sender, mes.message, mes.timestamp, color, add, pos);
                ++pos;
            } else { // insert at bottom
                insert_message(root_id, mes.sender, mes.message, mes.timestamp, color, add);
            }
            */
        //}


        /*

        std::vector<ulong> to_add;
        std::vector<ulong> to_remove;

        std::set_difference(
            new_messages.begin(), new_messages.end(),
            existing_messages.begin(), existing_messages.end(),
            std::back_inserter(to_add));
            
        std::set_difference(
            existing_messages.begin(), existing_messages.end(),
            new_messages.begin(), new_messages.end(),
            std::back_inserter(to_remove));

        //

        for(ulong remove : to_remove) {
            ulong widget_id = message_map[remove];
            
            remove_message(root_id, widget_id, remove);
            //ui_system.widgets.erase(widget_id);
        }

        auto& root_widget = ui_system.widgets[root_id];
        uint pos = 0;

        //std::reverse(to_add.begin(), to_add.end());

        for(ulong add : to_add) {
            message& mes = messages[add];

            vec3 color = player_color;
            if(mes.sender == "Averie") color = self_color;
            else if(mes.sender == "Addie") color = addie_color;

            if(new_range.x < message_range.x) { // insert at top
                insert_message(root_id, mes.sender, mes.message, mes.timestamp, color, add, pos);
                ++pos;
            } else { // insert at bottom
                insert_message(root_id, mes.sender, mes.message, mes.timestamp, color, add);
            }
        }

        push_headers(headers);

        message_range = new_range;
        */
    }

    void generate_placeholder() {
        json root;
        root["messages"] = json::array();

        //
        
        axiom::random32 rand(0xFF55FF66);

        ulong timestamp = axiom::get_timestamp();
        ulong start_timestamp = timestamp - 365.0f * 86400.0f * 1000000.0f;

        std::vector<std::string> names = {
            "Averie",
            "Addie",
            "Luna",
            "Nova",
            "Kael",
            "Mira",
            "Rowan",
            "Lyra12",
            "RigelOrion",
            "Violet",
            "Atlas",
            "Ember",
            "Jasper",
            "Willow",
            "Phoenix",
            "xXSkyeXx",
            "Echo",
            "Aria",
            "Finn",
            "Cora",
            "Riven",
            "NovaByte86",
            "PixelFox3",
            "Solaris",
            "Zenith",
            "Nyx",
            "Rune",
            "Vale",
            "Axel",
            "Maris",
            "Cypher50",
            "Elara"
        };

        std::vector<std::string> messages = {
            "Hey, are you online? I was wondering if you wanted to explore the new area together because I found a really interesting path that I don't think many people have discovered yet.",
            "I just finished the new update and there are so many small changes that I didn't notice at first. The new interface feels much cleaner and the animations are really nice.",
            "Want to explore the new area together? I heard there are some rare resources hidden near the mountains, but we should probably bring some supplies before we go.",
            "That was a really close fight. I thought we were going to lose when the boss started using that final attack, but everyone worked together and we barely survived.",
            "I found a secret room behind the waterfall. It had some old decorations, a few mysterious items, and a note that looked like it was left there years ago.",
            "Can you help me with this quest? I have been stuck on this part for a while because the enemies keep spawning faster than I can defeat them.",
            "The weather looks amazing today. It would be nice to find somewhere peaceful to build a small base and just watch the sunset.",
            "I finally got the item I was looking for! It took several hours of searching, but it was completely worth it because it fits perfectly with my current setup.",
            "Does anyone want to join the party? We could probably finish the dungeon much faster if we had a few more people helping with the harder sections.",
            "I'll be there in five minutes. I just need to finish organizing my inventory because somehow I managed to fill every single storage slot again.",
            "That was the funniest thing I've seen all day. I was not expecting the game physics to completely break like that, but somehow it made the moment even better.",
            "I think we should build our base here. The location is close to resources, has a nice view, and would be easy to defend if anything attacks us.",
            "Have you tried the new feature yet? I think it has a lot of potential, but there are still a few things that could be improved.",
            "The server is running really smoothly today. I remember when we first started playing and everything was lagging constantly whenever too many people joined.",
            "I saved you some resources from my last trip. I wasn't sure what you needed, so I grabbed a little bit of everything just in case.",
            "Let's meet at the northern gate around sunset. From there we can decide where to go next and make a plan before we start exploring.",
            "I need to take a quick break. My hands are getting tired from gathering materials for so long, but I'll be back soon.",
            "Did you see what happened earlier? The entire area changed after the event started, and I think there might be more secrets hidden nearby.",
            "This place has such a cool atmosphere. The lighting, music, and small details make it feel like someone put a lot of effort into designing it.",
            "I finished organizing the inventory. It took longer than expected because I kept finding old items that I forgot I had collected.",
            "Your idea actually worked! I wasn't sure the strategy would succeed, but it ended up being much better than what we were doing before.",
            "Let's try a different strategy this time. The last attempt was close, but I think we can make a few changes that will improve our chances.",
            "I can't believe we actually won. That was probably one of the hardest challenges we've completed so far.",
            "Do you remember where we found that? I want to go back there later because I think there might have been something else we missed.",
            "I'll send you the coordinates. It should be easy to find, but make sure you bring enough supplies because the journey takes longer than it looks.",
            "The update notes look interesting. I'm especially curious about the changes to the crafting system because it could completely change how people play.",
            "That animation looks really polished. Small details like that make the whole world feel much more alive and enjoyable.",
            "I think we are ready to continue. Everyone has their equipment prepared, and we should have enough resources for the next part.",
            "Thanks for helping me out. I know that took a lot of time, and I really appreciate you sticking around until we finished.",
            "Something feels different today. I can't really explain it, but the world feels more alive than usual.",
            "I have a new idea I want to test. It might not work perfectly at first, but I think it could lead to something really interesting.",
            "See you again soon! Hopefully next time we can finish the rest of the adventure and discover what happens next."
        };

        int num_messages = 5000;
        ivec2 num_per_person = {1, 6};

        int counter = 0;
        int person_num = 0;
        std::string name;
        ulong time;
        for(int i = 0; i < num_messages; ++i) {
            float frac = float(i) / num_messages;

            if(counter >= person_num) {
                person_num = floor(num_per_person.x + (num_per_person.y - num_per_person.x) * (rand() * 0.5f + 0.5f)); 

                counter = 0;

                name = names[floor((rand() * 0.5f + 0.5f) * names.size())];

                time = start_timestamp + float(timestamp - start_timestamp) * frac;
            }

            message m;
            m.sender = name;

            uint n = floor((rand() * 0.5f + 0.5f) * 3) + 1;
            std::string str;
            for(int i = 0; i < n; ++i) {
                str += messages[floor((rand() * 0.5f + 0.5f) * messages.size())];
                if(i < n - 1) str += "\n";
            }
            m.message = str;

            m.timestamp = time;
            m.index = i;

            ++counter;

            //

            json mes;
            mes["sender"] = m.sender;
            mes["text"] = m.message;
            mes["timestamp"] = m.timestamp;

            root["messages"].push_back(mes);
        }
        
        axiom::write_text_to_file("resources/json/messages.json", root.dump(4));
    }

    void rebuild() {
        rebuild_flag = false;

        auto header_map = pop_headers();

        push_headers(header_map);
    }

    void call() {
        if(enabled) {
            axiom::ui_system& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();
            
            if(ui_system.window->pressed_buttons.contains(axiom::input_code::KEY_F4) || ui_system.window->repeat_buttons.contains(axiom::input_code::KEY_F4)) {
                rebuild_flag = true;
            }

            axiom::widget* column_root = ui_system.widgets[root_id].get();
            axiom::scroll_widget* scroll_root = dynamic_cast<axiom::scroll_widget*>(ui_system.widgets[column_root->parent].get());

            if(!scroll_root->capture_scroll) change_range();
        }
    }

    void enable(ulong root_widget) {
        root_id = root_widget;
        enabled = true;
        
        for(auto& mes : json_files[0]["messages"]) {
            std::string sender = mes["sender"];
            std::string content = mes["text"];
            ulong timestamp = mes["timestamp"];

            message message_;
            message_.message = content;
            message_.sender = sender;
            message_.timestamp = timestamp;

            messages.push_back(message_);

            //vec3 color = player_color;
            //if(sender == "Averie") color = self_color;
            //else if(sender == "Addie") color = addie_color;

            //insert_message(root_id, sender, content, timestamp, color, false);
        }

        change_range();

        ivec2 total_range = ivec2(0, json_files[0]["messages"].size());

        ivec2 target_range = ivec2(glm::max(0, total_range.y - 64), total_range.y);
    }

    void disable() {
        enabled = false;
    }

    chat_system() {
        json root_json;

        std::ifstream stream("resources/json/messages.json");
        stream >> root_json;

        json_files.push_back(std::move(root_json));
    }
};

std::unordered_map<ulong, ulong> chat_system::pop_headers() {
    auto& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();
    auto& root = ui_system.widgets[root_id];

    std::vector<int> remove;
    std::unordered_map<ulong, ulong> headers;

    for(int i = 0; i < root->children.size(); ++i) {
        ulong current_id = root->children[i];
        bool rem = !dynamic_cast<axiom::message_widget*>(ui_system.widgets[current_id].get());
        if(rem) {
            if(i + 1 < root->children.size()) {
                ulong next_id = root->children[i + 1];
                bool n = dynamic_cast<axiom::message_widget*>(ui_system.widgets[next_id].get());

                if(n) {
                    headers.emplace(next_id, current_id);
                }
            }
            remove.push_back(i);
        }
    }

    std::sort(remove.begin(), remove.end());

    for(int i = remove.size() - 1; i >= 0; --i) {
        //ui_system.widgets.erase(root->children[remove[i]]);
        root->children.erase(root->children.begin() + remove[i]);
    }

    return headers;
}

void chat_system::push_headers(std::unordered_map<ulong, ulong> headers) {
    auto& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();
    auto& root = ui_system.widgets[root_id];

    std::unordered_set<ulong> del_map;

    ui_system.input_set(root_id);
    
    ulong time_delta = 2.0f * 60.0f * 1000000.0f;
    
    std::vector<glm::vec<2, ulong>> to_insert;

    std::string sender = "";
    ulong timestamp = 0;
    ulong prev = axiom::NULL_WIDGET;

    bool inserted_prev = false;
    for(int i = 0; i < root->children.size(); ++i) {
        ulong current_id = root->children[i];

        bool d = dynamic_cast<axiom::message_widget*>(ui_system.widgets[current_id].get());
        if(!d) continue;

        axiom::message_widget* message = dynamic_cast<axiom::message_widget*>(ui_system.widgets[current_id].get());

        //

        message->tail_settings = 0;
        message->tail_size = 0.0f;

        if(message->sender != sender) {
            ulong label;
            if(headers.contains(current_id) && !message->inserted) {
                label = headers[current_id];
                del_map.emplace(current_id);
            } else {
                if(message->sender == "Averie") {
                    ui_system.position(axiom::position_mode::TOP_RIGHT);
                    ui_system.buffer(vec4(4.0f, 4.0f, 4.0f, 14.0f));

                    //
                    
                    std::string L = "[" + std::to_string(message->index) + "] " + message->sender + " <"; // axiom::get_date_time_string(message->timestamp)
                    label = axiom::text_widget::insert(L, axiom::text_alignment::RIGHT);
                } else {
                    ui_system.position(axiom::position_mode::TOP_LEFT);
                    ui_system.buffer(vec4(4.0f, 4.0f, 4.0f, 14.0f));

                    //

                    std::string L = "> " + message->sender + " [" + std::to_string(message->index) + "]"; 
                    label = axiom::text_widget::insert(L, axiom::text_alignment::LEFT);
                }

                root->children.erase(root->children.end() - 1, root->children.end());
            }

            to_insert.push_back({i, label});
            
            if(prev != axiom::NULL_WIDGET) {
                axiom::message_widget* message = dynamic_cast<axiom::message_widget*>(ui_system.widgets[prev].get());

                message->tail_size = 8.0f;
                if(message->sender == "Averie") message->tail_settings = 2;
                else message->tail_settings = 1;
            }
        } else if(message->timestamp > timestamp + time_delta) {
            ulong label;
            if(headers.contains(current_id) && !message->inserted) {
                label = headers[current_id];
                del_map.emplace(current_id);
            } else {
                if(message->sender == "Averie") {
                    ui_system.position(axiom::position_mode::TOP_RIGHT);
                    ui_system.buffer(vec4(4.0f, 4.0f, 4.0f, 14.0f));
                    
                    //

                    std::string L = "[" + std::to_string(message->index) + "] " + message->sender + " <"; 
                    label = axiom::text_widget::insert(L, axiom::text_alignment::RIGHT);
                } else {
                    ui_system.position(axiom::position_mode::TOP_LEFT);
                    ui_system.buffer(vec4(4.0f, 4.0f, 4.0f, 14.0f));

                    //

                    std::string L = "> " + message->sender + " [" + std::to_string(message->index) + "]"; 
                    label = axiom::text_widget::insert(L, axiom::text_alignment::LEFT);
                }

                root->children.erase(root->children.end() - 1, root->children.end());
            }

            to_insert.push_back({i, label});

            if(prev != axiom::NULL_WIDGET) {
                axiom::message_widget* message = dynamic_cast<axiom::message_widget*>(ui_system.widgets[prev].get());

                message->tail_size = 8.0f;
                if(message->sender == "Averie") message->tail_settings = 2;
                else message->tail_settings = 1;
            }
        } else if(message->inserted) {
            if(headers.contains(current_id)) {
                ulong label = headers[current_id];
                del_map.emplace(current_id);
                
                to_insert.push_back({i, label});
            } else inserted_prev = false;
        } else inserted_prev = false;

        sender = message->sender;
        timestamp = message->timestamp;
        prev = root->children[i];

        if(i == root->children.size() - 1) {
            if(prev != axiom::NULL_WIDGET) {
                axiom::message_widget* message = dynamic_cast<axiom::message_widget*>(ui_system.widgets[prev].get());

                message->tail_size = 8.0f;
                if(message->sender == "Averie") message->tail_settings = 2;
                else message->tail_settings = 1;
            }
        }
    }

    ulong inserted = 0;
    for(auto& vec : to_insert) {
        root->children.insert(root->children.begin() + (vec.x + inserted), vec.y);
        ++inserted;
    }
    
    ui_system.buffer(vec4(0.0f));
    ulong spacer = axiom::spacer_widget::insert(vec2(0.0f, 0.0f), vec2(FLT_MAX, 0.0f), false);
    root->children.erase(root->children.end() - 1, root->children.end());
    root->children.insert(root->children.begin(), spacer);

    for(auto [k, i] : headers) {
        if(!del_map.contains(k)) {
            ui_system.widgets.erase(i);
        }
    }
}

void chat_system::insert_message(ulong message_root, std::string sender, std::string message, ulong timestamp, vec3 color, ulong map_index, int index) {
    auto& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();
    
    auto& root = ui_system.widgets[message_root];

    ui_system.input_set(message_root);
    ui_system.buffer(vec4(4.0f));

    if(sender == "Averie") ui_system.position(axiom::position_mode::TOP_RIGHT);
    else ui_system.position(axiom::position_mode::TOP_LEFT);

    //

    ulong w = axiom::message_widget::insert(sender, timestamp, message, axiom::text_alignment::LEFT, vec2(160, 10000), color, vec2(8.0f), 0);
    
    auto ww = dynamic_cast<axiom::message_widget*>(ui_system.widgets[w].get());
    ww->index = map_index;
    
    message_map.emplace(map_index, w);

    if(index != -1) {
        root->children.erase(root->children.end() - 1);
        root->children.insert(root->children.begin() + index, w);
    }

    rebuild_flag = true;
}

void chat_system::remove_message(ulong message_root, ulong id, ulong map_index) {
    auto& ui_system = axiom::global_core.ecs->get_system<axiom::ui_system>();
    auto& root = ui_system.widgets[message_root];

    int index = 0;
    bool contains = true;
    while(true) {
        if(root->children[index] == id) break;

        ++index;
        if(index >= root->children.size()) {
            contains = false;
            break;
        }
    }

    message_map.erase(map_index);

    if(contains) {
        root->children.erase(root->children.begin() + index);

        rebuild_flag = true;
    }

    ui_system.widgets.erase(id);
}

struct basic_system : axiom::system {
    axiom::window* win;
    axiom::shader shad;
    axiom::vertices vertices;
    axiom::texture tex;

    axiom::texture ui_texture;
    axiom::texture* font_texture;

    basic_system(axiom::window* win_, axiom::texture* texture) {
        win = win_;

        axiom::text_asset vert = axiom::text_asset::load("resources/shaders/ui.vert");
        axiom::text_asset frag = axiom::text_asset::load("resources/shaders/ui.frag");
        
        shad = std::move(axiom::shader(vert, frag));
        
        axiom::texture_asset texasset = axiom::texture_asset::load("resources/textures/ui.png");

        tex = std::move(axiom::texture(texasset, axiom::texture_format::RGBA8));
        font_texture = texture;

        /*
        struct color_vertex {
            vec3 position;
            vec3 color;
            vec2 tex_coord;
        };

        std::vector<color_vertex> vs = {
            color_vertex({-0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}),
            color_vertex({0.0f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {0.5f, 1.0f}),
            color_vertex({0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f})
        };
        */

        vertices.init();

        /*
        vertices.vertex_buffer_data(vs.data(), 3, sizeof(color_vertex), GL_STATIC_DRAW);

        vertices.add_vertex_attribute(0, 3, GL_FLOAT, false, sizeof(color_vertex), 0);
        vertices.add_vertex_attribute(1, 3, GL_FLOAT, false, sizeof(color_vertex), sizeof(float) * 3);
        vertices.add_vertex_attribute(2, 2, GL_FLOAT, false, sizeof(color_vertex), sizeof(float) * 6);
        */
    }

    void call() {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        if(win->pressed_buttons.contains(axiom::input_code::KEY_F11)) {
            if(win->is_fullscreen()) win->make_windowed();
            else {
                win->make_fullscreen();
            }
        }

        glViewport(0, 0, win->viewport_size.x, win->viewport_size.y);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //
        
        glEnable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_GEQUAL);
        glClearDepth(0.0f);
        glDepthRange(0, 1);
        glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
        glDisable(GL_DEPTH_CLAMP);
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE);

        shad.use();
        font_texture->bind(0);
        tex.bind(1);

        axiom::ui_system& ui_system = ecs->get_system<axiom::ui_system>();
        for(int i = 0; i < ui_system.target_textures.size(); ++i) {
            ui_system.target_textures[i]->bind(i + 2);
        }

        //std::cout << win->is_fullscreen() << " " << win->screen_size.x << " " << win->screen_size.y << " " << win->viewport_size.x << " " << win->viewport_size.y << "\n";

        /*
        struct ui_vertex {
            vec3 pos;
            vec2 tex_pos;
            vec4 color = vec4(1.0f);
            vec4 range = vec4(-FLT_MAX, -FLT_MAX, FLT_MAX, FLT_MAX);
            uint data = 0;
        };
        */

        mat3 view_matrix = glm::translate(glm::identity<mat3>(), vec2(-1.0f, -1.0f)) * glm::scale(glm::identity<mat3>(), vec2(2.0f / win->viewport_size.x, 2.0f / win->viewport_size.y));
        mat3 trans_matrix = glm::identity<mat3>();

        vertices.vertex_buffer_data(ui_system.vertices.data(), ui_system.vertices.size(), sizeof(axiom::ui_vertex), GL_STREAM_DRAW);

        vertices.add_vertex_attribute(0, 3, GL_FLOAT, false, sizeof(axiom::ui_vertex), 0);
        vertices.add_vertex_attribute(1, 2, GL_FLOAT, false, sizeof(axiom::ui_vertex), sizeof(float) * 3);
        vertices.add_vertex_attribute(2, 4, GL_FLOAT, false, sizeof(axiom::ui_vertex), sizeof(float) * 5);
        vertices.add_vertex_attribute(3, 4, GL_FLOAT, false, sizeof(axiom::ui_vertex), sizeof(float) * 9);
        vertices.add_vertex_attribute(4, 1, GL_UNSIGNED_INT, false, sizeof(axiom::ui_vertex), sizeof(float) * 13);

        vertices.bind();

        glUniformMatrix3fv(0, 1, false, &view_matrix[0][0]);
        glUniformMatrix3fv(1, 1, false, &trans_matrix[0][0]);

        vertices.draw_vertices(GL_TRIANGLES);

        glfwSwapBuffers(win->window_handle);
    }
};

int main(int argc, char* argv[]) {
    float param = 0.0f;

    axiom::window win = axiom::window(ivec2(64, 64), ivec2(512, 512), 6, "axiom test", false);
    win.hide_cursor();

    axiom::ecs ecs;
    ecs.make_active();

    axiom::font_asset default_font = axiom::font_asset::load("resources/fonts/axiom_default.bdf"); //

    axiom::texture font_tex = std::move(axiom::texture(default_font.texture, axiom::texture_format::RGBA8));
    
    axiom::ui_system ui_system(&win);
    ui_system.font_assets = {&default_font};

    chat_system csystem;
    csystem.generate_placeholder();

    ecs.register_system(csystem);
    ecs.register_system(ui_system);

    std::function<void()> lipsum_func = [&ui_system]() {
        ui_system.position(axiom::position_mode::TOP_LEFT);

        ui_system.input_reset();
        ui_system.input_z(0.1f);

        vec2 window_size = vec2(384, 384);

        axiom::window_widget::insert("WINDOW", window_size, (vec2(ui_system.window->screen_size) - window_size) * 0.5f, axiom::color_blue);
        
        ui_system.buffer(vec4(0.0f));
        axiom::panel_widget::insert();
        axiom::scroll_widget::insert(6.0f, true);
        ui_system.buffer(vec4(2.0f));

        axiom::column_widget::insert();

        std::string lipsum = R"(Lorem ipsum dolor sit amet, consectetur adipiscing elit. Vestibulum neque mi, tincidunt vitae efficitur in, porta eget erat. Nam vitae leo nec ligula imperdiet lacinia. Praesent sed elit vitae diam finibus convallis at a leo. Duis finibus dolor nisl, vitae tristique lectus egestas a. Nullam quam lectus, fringilla a iaculis vel, suscipit sit amet lectus. Nulla rutrum dapibus enim et tincidunt. Suspendisse et lacus ac dui tristique bibendum et a orci. Donec maximus nulla quis scelerisque placerat. Sed sagittis quam est, vestibulum condimentum sem feugiat ac. Cras non est at nisl fringilla interdum. Vestibulum ut neque sagittis, dictum ligula non, gravida mi. Quisque a nunc lorem. Nam libero libero, aliquet eu tincidunt sit amet, sollicitudin suscipit ex. Curabitur lacinia magna augue, vitae laoreet nisl placerat a. Donec convallis nulla sed nulla lacinia, sed tempus orci volutpat. Quisque vitae turpis eu nisl cursus dignissim.

    Aliquam interdum lectus risus, id efficitur ipsum bibendum vitae. Donec nulla ante, pretium in ullamcorper nec, bibendum ac nunc. Vivamus metus nisl, suscipit ac commodo at, viverra at enim. Pellentesque egestas facilisis sagittis. Duis vel sodales augue. Aliquam erat volutpat. Nam vel lectus at dui congue tincidunt. Phasellus placerat aliquet urna eu congue. Quisque turpis mauris, accumsan at tincidunt ut, dapibus sit amet erat. Nullam enim felis, facilisis nec vulputate eu, congue et nunc. Nunc eros turpis, placerat ac sem eget, pulvinar ultrices arcu. Etiam placerat dui eros, eget commodo metus tempus et. Maecenas volutpat lacinia nisi, eu laoreet sapien ultrices nec.)";

        axiom::text_widget::insert(lipsum, axiom::text_alignment::LEFT, true);
    };

    std::function<void()> render_func = [&ui_system]() {
        ui_system.position(axiom::position_mode::TOP_LEFT);

        ui_system.input_reset();
        ui_system.input_z(0.1f);

        vec2 window_size = vec2(384, 384);

        axiom::window_widget::insert("WINDOW", window_size, (vec2(ui_system.window->screen_size) - window_size) * 0.5f, axiom::color_blue);
        
        axiom::panel_widget::insert();
    };

    
    std::shared_ptr<axiom::menu_node> node(new axiom::menu_node{
        "",
        {
            axiom::menu_node("Debug Windows", {
                axiom::menu_node("Lipsum", {}, lipsum_func),
                axiom::menu_node("Render", {}, render_func),
            })
        }
    });

    //

    /*
    std::string lipsum = R"(Lorem ipsum dolor sit amet, consectetur adipiscing elit. Pellentesque at dolor leo. Cras volutpat, dolor vitae venenatis dignissim, nisl tortor semper ex, nec sagittis lorem sem ac lorem. Maecenas non sem vel tortor rhoncus faucibus. Donec eu rutrum tellus. Pellentesque justo ante, bibendum nec hendrerit non, aliquet in elit. Sed a nibh rutrum, tempor elit scelerisque, aliquam urna. Mauris ultrices, metus vel consectetur aliquet, urna tellus eleifend tellus, in iaculis sem ligula at eros. Nulla pretium sed eros id vestibulum. Cras sodales ligula vitae leo vulputate tincidunt eget a nibh. Fusce augue nunc, condimentum non vulputate quis, laoreet vel augue. Interdum et malesuada fames ac ante ipsum primis in faucibus. Nullam efficitur metus eget diam molestie sollicitudin. Mauris hendrerit, ligula a scelerisque viverra, nisi ex venenatis ex, nec hendrerit ante tortor eu ante. Nullam sapien arcu, porta in dictum ac, dignissim ac ex. Nam euismod fermentum molestie.

    Aenean cursus a odio in luctus. Integer mollis lacus et nisl vulputate, quis faucibus nisl tristique. Maecenas at sodales elit. Nam ut ex mollis ipsum ultrices aliquet in ut odio. Phasellus pharetra ipsum euismod cursus rutrum. Fusce at ligula iaculis, sodales mauris quis, convallis ex. Pellentesque a iaculis nunc, commodo egestas felis. Quisque pharetra volutpat justo, eu sollicitudin enim vulputate ullamcorper. Nunc dapibus aliquam lacus id sollicitudin. Proin vestibulum feugiat imperdiet. Morbi non nunc at orci tristique lacinia. Pellentesque euismod vestibulum eros ut lacinia. Donec hendrerit est eget metus pharetra semper. Vivamus volutpat leo eu sem porttitor tristique. Suspendisse potenti. Fusce commodo odio vestibulum, accumsan augue sed, egestas arcu.

    Suspendisse sit amet consectetur tellus. Nunc ornare scelerisque magna sed gravida. Proin eu condimentum felis. Nunc eu dui quis enim suscipit ornare. Mauris sed nisi enim. Aliquam malesuada interdum lorem, sit amet sodales nisl. In elementum euismod elit non euismod. Nunc tempor erat lacus, quis condimentum mauris aliquam eget. Morbi non hendrerit ex. Duis pharetra commodo lacus ac facilisis. Cras elementum vehicula ante. Duis sodales elementum erat, quis venenatis erat.

    Lorem ipsum dolor sit amet, consectetur adipiscing elit. Phasellus non justo consequat, luctus lectus eu, porta est. Phasellus tincidunt ligula a fringilla semper. Nunc quis diam in dui hendrerit porttitor. Aenean blandit vitae quam feugiat malesuada. Sed non ipsum diam. Sed tincidunt velit non bibendum tincidunt. Morbi et purus metus. Aenean fermentum, elit sed pretium venenatis, nisl leo molestie tellus, ac facilisis metus elit eget orci.

    Sed vel augue eu leo gravida dictum. Nullam pharetra turpis sem, sed rhoncus massa hendrerit at. Pellentesque molestie tincidunt mollis. Fusce ipsum mauris, sodales dictum ipsum vel, vulputate pretium tellus. Phasellus odio nisl, pellentesque vitae fermentum sed, blandit non sem. Curabitur eget bibendum ligula, vel dignissim massa. Donec non risus id elit suscipit egestas. Vivamus vel lectus faucibus, porta augue id, viverra purus.)";
    */

    std::function<void()> func_chat = [&ui_system, &csystem]() {
        ui_system.buffer(vec4(0.0f, 0.0f, 0.0f, 2.0f));

        ui_system.position(axiom::position_mode::TOP_LEFT);

        axiom::panel_widget::insert();
        ui_system.buffer(vec4(0.0f));
        axiom::column_widget::insert();
        axiom::scroll_widget::insert(8.0f, true);
        ui_system.buffer(vec4(0.0f));

        float buffer = 8.0f;

        ui_system.buffer(vec4(4.0f));
        ulong message_root = axiom::column_widget::insert();

        //
        ui_system.input_step(2);
        axiom::spacer_widget::insert(vec2(0.0f), vec2(FLT_MAX), false, true);
        ui_system.buffer(vec4(8.0f));
        ui_system.position(axiom::position_mode::BOTTOM_LEFT);
        axiom::column_widget::insert();
        axiom::text_box_widget::insert(FLT_MAX, vec2(8.0f, 8.0f), ""//, 
            /*[&ui_system, buffer, message_root, self_color](axiom::text_box_widget& self) {
                auto& root = ui_system.widgets[message_root];
                ulong time_delta = 2.0f * 60.0f * 1000000.0f;

                if(self.text[0]->string.size()) {
                    insert_message(message_root, "Averie", self.text[0]->string, axiom::get_timestamp(), self_color);
                    axiom::scroll_widget* parent = dynamic_cast<axiom::scroll_widget*>(ui_system.widgets[root->parent].get());
                    parent->scroll_pos = -FLT_MAX * 0.5f;
                    parent->anchor_widget = 0xFFFFFFFFFFFFFFFD;

                    self.text[0]->string = "";
                }
            }*/
        );
        ui_system.set_attrib(vec2(160, 16), vec2(FLT_MAX, 16), vec2(1.0f));

        csystem.enable(message_root);
    };

    std::function<void()> func_settings = [&ui_system, &node, &param]() {
        ui_system.buffer(vec4(0.0f, 0.0f, 0.0f, 2.0f));

        axiom::panel_widget::insert();

        ui_system.buffer(vec4(4.0f));
        ui_system.position(axiom::position_mode::TOP_LEFT);

        axiom::column_widget::insert();
        axiom::grid_widget::insert(3);

        //
        
        ui_system.position(axiom::position_mode::CENTER_LEFT);
        axiom::text_widget::insert("slider", axiom::text_alignment::LEFT, false);
        axiom::spacer_widget::insert(vec2(0, 0), vec2(FLT_MAX, 0), false);
        axiom::row_widget::insert();
        ui_system.set_attrib(vec2(160, 16), vec2(160, 16), vec2(1.0f));

        axiom::slider_widget::insert(vec2(120, 16), 6, axiom::color_blue, vec2(-16.0f, 16.0f), 0.0f, 0.0f, "",
            [&param](axiom::slider_widget& self) {
                if(!self.pressed) self.current_value = param;
                else param = self.current_value;
            }
        );
        ui_system.set_attrib(vec2(0, 16), vec2(FLT_MAX, 16), vec2(1.0f));

        axiom::input_box_widget<float>::insert(vec2(40, 16), 0.0f, 
            [&param](axiom::input_box_widget<float>& self) {
                if(self.update) {
                    param = self.value;
                } else {
                    self.value = param;
                }
            }
        );

        ui_system.input_step();
    };

    axiom::screen_widget::insert("axiom text", axiom::color_blue);
    axiom::relative_widget::insert(
        [&ui_system](axiom::relative_widget& widget) {
            axiom::screen_widget* parent = (axiom::screen_widget*)ui_system.widgets[widget.parent].get();
            widget.position = vec2(parent->header, parent->size.y - parent->header);

            for(auto child : widget.children) {
                auto& child_widget = ui_system.widgets[child];

                child_widget->position = widget.position;
                child_widget->size = vec2(parent->size.x - parent->header, parent->header);
            }
        }
    );

    ui_system.position(axiom::position_mode::CENTER_LEFT);
    axiom::row_widget::insert();

    axiom::button_widget::insert(vec2(40.0f, 16.0f), axiom::color_blue, "TEST", 
        [&node, &ui_system](axiom::button_widget& self) {
            if(self.pressed) {
                ui_system.position(axiom::position_mode::TOP_LEFT);

                ui_system.input_reset();
                axiom::menu_widget::insert(self.position, 0.001, axiom::color_blue, 200, 16, FLT_MAX, node, {});
            }
        }
    );
    
    ui_system.input_root(1);
    ui_system.position(axiom::position_mode::TOP_LEFT);

    axiom::split_widget::insert(axiom::layout_mode::ROW, {{1.0f, axiom::panel_mode::SCALE}, {1.0f, axiom::panel_mode::SCALE}});
    axiom::panel_widget::insert();
    
    ui_system.buffer(vec4(0.0f, 0.0f, 0.0f, 2.0f));
    axiom::tab_widget::insert(24.0f, 2.0f, {
        axiom::tab("Settings", 80.0f, axiom::color_blue, func_settings),
        axiom::tab("Chat", 80.0f, axiom::color_blue, func_chat),
    });

    //
    //
    //

    ui_system.input_root(2);
    
    axiom::panel_widget::insert();
    
    //
    
    axiom::text_asset vert_asset = axiom::text_asset::load("resources/shaders/test.vert");
    axiom::text_asset frag_asset = axiom::text_asset::load("resources/shaders/test.frag");
    axiom::shader shad = std::move(axiom::shader(vert_asset, frag_asset));
    
    axiom::texture_asset tex_asset = axiom::texture_asset::load("resources/textures/test.png");
    axiom::texture tex = std::move(axiom::texture(tex_asset, axiom::texture_format::RGBA8));
    
    ui_system.buffer(vec4(0.0f, 0.0f, 0.0f, 0.0f));
    axiom::render_target target;
    {
        std::function<void(axiom::render_target&)> render_func = [&shad, &tex](axiom::render_target& f) {
            static axiom::vertices vertices;
            static double rotation = 0.0f;

            vec2 scale = vec2(f.size) / float(glm::min(f.size.x, f.size.y));
            mat4 matrix = glm::scale(vec3(1.0f / scale, 1.0f));
            matrix = matrix * glm::rotate((float)rotation, vec3(0.0f, 0.0f, 1.0f));

            if(axiom::global_core.ecs->delta_time < 1.0f) rotation += axiom::global_core.ecs->delta_time;

            struct color_vertex {
                vec3 position;
                vec3 color;
                vec2 tex_coord;
            };

            float r = 0.75f;

            std::vector<color_vertex> vs = {
                color_vertex({-sqrt(3.0f) * 0.5f * r, -0.5f * r, 0.5f}, {1.0f, 0.0f, 0.0f}, vec2(-sqrt(3.0f) * 0.5f, -0.5f)),
                color_vertex({0.0f, r, 0.5f}, {0.0f, 1.0f, 0.0f}, vec2(0.0f, 1.0f)),
                color_vertex({sqrt(3.0f) * 0.5f * r, -0.5f * r, 0.5f}, {0.0f, 0.0f, 1.0f}, vec2(sqrt(3.0f) * 0.5f, -0.5f))
            };

            if(!vertices.initialized) vertices.init();

            vertices.vertex_buffer_data(vs.data(), 3, sizeof(color_vertex), GL_STATIC_DRAW);

            vertices.add_vertex_attribute(0, 3, GL_FLOAT, false, sizeof(color_vertex), 0);
            vertices.add_vertex_attribute(1, 3, GL_FLOAT, false, sizeof(color_vertex), sizeof(float) * 3);
            vertices.add_vertex_attribute(2, 2, GL_FLOAT, false, sizeof(color_vertex), sizeof(float) * 6);

            shad.use();
            tex.bind(0);
            vertices.bind();

            glUniformMatrix4fv(0, 1, false, &matrix[0][0]);

            vertices.draw_vertices(GL_TRIANGLES);
        };
        std::vector<axiom::texture_format> formats = {axiom::texture_format::RGBA8};
        std::vector<axiom::texture_attachment> attachments = {axiom::texture_attachment::COLOR0};

        target = axiom::render_target::create(render_func, ivec2(400, 400), formats, attachments);
    }

    axiom::render_widget::insert(&target, 0);

    //

    basic_system bsystem(&win, &font_tex);
    ecs.register_system(bsystem);

    double prev_time = axiom::get_time();
    double frame_rate = 1;
    

    //

    while(!win.should_close) {
        double current_time = axiom::get_time();
        double delta_time = current_time - prev_time;
        prev_time = current_time;

        if(win.input_map[axiom::input_code::KEY_F1]) {
            double sleep_for = (1.0f / frame_rate) - delta_time;
            prev_time += sleep_for;
            if(sleep_for > 0.0f) std::this_thread::sleep_for(std::chrono::microseconds(int(sleep_for * 1000000.0f)));
        }

        win.poll_events();

        ecs.do_frame();
    }
}