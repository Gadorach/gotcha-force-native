#pragma once
#include <cmath>
#include "engine/gameplay/story_map_projection.hpp"
#include "engine/graphics/scene_draw.hpp"
#include "engine/graphics/text.hpp"

namespace gf {

class StoryMapFocus {
public:
    std::size_t index(const StoryMapProjection& map) const {
        for(std::size_t i = 0; i < map.nodes.size(); ++i) if(map.nodes[i].id == id_) return i;
        for(std::size_t i = 0; i < map.nodes.size(); ++i) if(map.nodes[i].selected) return i;
        return 0;
    }
    bool select(const StoryMapProjection& map, std::size_t i) {
        if(i >= map.nodes.size() || id_ == map.nodes[i].id) return false;
        id_ = map.nodes[i].id;
        return true;
    }
    bool step(const StoryMapProjection& map, bool backwards) {
        const auto count = map.nodes.size();
        if(count < 2) return false;
        const auto i = index(map);
        return select(map, (i + (backwards ? count-1 : 1)) % count);
    }
private:
    std::string id_;
};

struct StoryMapRect {
    s32 x = 0, y = 0, w = 0, h = 0;
    bool contains(f32 px, f32 py) const {
        return px >= x && py >= y && px < x+w && py < y+h;
    }
};

// Drawable-pixel geometry shared by the draw and hit-test paths. The panels
// reflow independently of the map's fit; the integer UI scale keeps glyphs
// sharp and uniform even if the two drawable/window ratios differ.
struct StoryMapLayout {
    s32 scale = 1;
    bool ready = false, stacked = false;
    StoryMapRect header, map_panel, map, details, footer;

    static StoryMapLayout fit(unsigned pixel_w, unsigned pixel_h, unsigned window_w, unsigned window_h,
                              unsigned art_w = 4, unsigned art_h = 3) {
        StoryMapLayout layout;
        if(!window_w || !window_h || pixel_w > 32768 || pixel_h > 32768 || !art_w || !art_h) return layout;
        layout.pixel_w_ = pixel_w; layout.pixel_h_ = pixel_h;
        layout.window_w_ = window_w; layout.window_h_ = window_h;
        layout.scale = s32(std::clamp(std::min(pixel_w/window_w, pixel_h/window_h), 1u, 3u));
        const s32 s = layout.scale, w = s32(pixel_w), h = s32(pixel_h);
        if(w < 112*s || h < 112*s) return layout;
        const s32 margin = 8*s, gap = 8*s, body_top = 42*s, body_bottom = h-25*s;
        const s32 body_h = body_bottom-body_top, body_w = w-2*margin;
        layout.ready = true;
        layout.stacked = w < 580*s;
        layout.header = {margin, margin, body_w, body_top-margin};
        layout.footer = {margin, body_bottom+gap, body_w, 12*s};
        if(layout.stacked) {
            const s32 info_h = std::min(58*s, body_h/2);
            layout.map_panel = {margin, body_top, body_w, body_h-info_h-gap};
            layout.details = {margin, body_bottom-info_h, body_w, info_h};
        } else {
            const s32 info_w = std::clamp(body_w/3, 186*s, 340*s);
            layout.map_panel = {margin, body_top, body_w-info_w-gap, body_h};
            layout.details = {w-margin-info_w, body_top, info_w, body_h};
        }
        s32 fitted_w = layout.map_panel.w, fitted_h = layout.map_panel.h;
        if(u64(fitted_w)*art_h <= u64(fitted_h)*art_w)
            fitted_h = std::max(1, s32(u64(fitted_w)*art_h/art_w));
        else
            fitted_w = std::max(1, s32(u64(fitted_h)*art_w/art_h));
        layout.map = {layout.map_panel.x + (layout.map_panel.w-fitted_w)/2,
                      layout.map_panel.y + (layout.map_panel.h-fitted_h)/2, fitted_w, fitted_h};
        return layout;
    }

    std::pair<f32, f32> position(const StoryMapNode& node) const {
        return {f32(map.x) + node.x*map.w, f32(map.y) + node.y*map.h};
    }

    StoryMapRect marker(const StoryMapNode& node, s32 radius) const {
        const auto [x, y] = position(node);
        const s32 left = std::max(map.x, s32(std::round(x))-radius);
        const s32 right = std::min(map.x+map.w, s32(std::round(x))+radius);
        const s32 top = std::max(map.y, s32(std::round(y))-radius);
        const s32 bottom = std::min(map.y+map.h, s32(std::round(y))+radius);
        return {left, top, right-left, bottom-top};
    }

    std::optional<std::size_t> hit_node(s32 window_x, s32 window_y, const StoryMapProjection& projection,
                                        const StoryMapFocus& focus) const {
        if(!ready || window_x < 0 || window_y < 0 || unsigned(window_x) >= window_w_ ||
           unsigned(window_y) >= window_h_) return {};
        const f32 x = f32(f64(window_x)*pixel_w_/window_w_), y = f32(f64(window_y)*pixel_h_/window_h_);
        if(!map.contains(x, y)) return {}; // Never hit in fitted-map letterbox bars.
        std::optional<std::size_t> hit;
        f32 distance = 0.0f;
        const auto focused = focus.index(projection);
        for(std::size_t i = 0; i < projection.nodes.size(); ++i) {
            const auto& node = projection.nodes[i];
            if(!marker(node, 8*scale).contains(x, y)) continue;
            const auto [cx, cy] = position(node);
            const f32 d = (cx-x)*(cx-x) + (cy-y)*(cy-y);
            if(!hit || d < distance || (d == distance && i == focused)) {
                hit = i;
                distance = d;
            }
        }
        return hit;
    }
private:
    unsigned pixel_w_ = 0, pixel_h_ = 0, window_w_ = 0, window_h_ = 0;
};

inline std::string story_map_compatibility_text(std::string_view text, std::size_t columns) {
    std::string result;
    for(char c : text) {
        if((u8(c) & 0xc0u) == 0x80u) continue; // One replacement per UTF-8 scalar.
        if(result.size() == columns) {
            if(columns >= 3) result.replace(columns-3, 3, "...");
            break;
        }
        if(c >= 'a' && c <= 'z') c = char(c-'a'+'A');
        if(c == '_') c = '-';
        result += TextRenderer::covers(c) ? c : '?';
    }
    return result;
}

class StoryMapView {
public:
    unsigned draw(RenderEngine& render, const StoryMapLayout& layout, const StoryMapProjection& map,
                  const StoryMapFocus& focus, u32 art = 0, bool campaign = false, std::string_view status = {}) {
        if(!layout.ready) return 0;
        const auto saved = render.state();
        render.state() = {};
        render.state().depth_test = render.state().depth_write = false;
        render.state().cull_backface = false;
        render.state().blend = true;
        render.state().blend_src = 5;
        render.state().blend_dst = 6;
        constexpr f32 identity[]{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};
        render.upload_matrix(0, identity);
        const auto s = layout.scale;
        unsigned draws = 0;
        auto fill = [&](StoryMapRect rect, Tint color, u32 texture = 0) {
            if(rect.w <= 0 || rect.h <= 0) return;
            quads_.clear();
            ScreenDraw::textured_quad(quads_, f32(rect.x), f32(rect.y), f32(rect.x+rect.w), f32(rect.y+rect.h),
                0.0f, 0.0f, 0.0f, 1.0f, 1.0f, color);
            render.state().texture = texture;
            const auto first = render.append_vertices(quads_.data(), quads_.size());
            draws += render.draw(Primitive::Triangles, first, u32(quads_.size())) ? 1 : 0;
        };
        auto label = [&](std::string_view value, StoryMapRect area, s32 dy, Tint color, s32 font_scale) {
            if(dy+7*font_scale > area.h) return;
            const auto room = std::size_t(std::max(0, area.w)/(6*font_scale));
            const TextRenderer::Style style{color.r, color.g, color.b, color.a, font_scale};
            draws += text_.draw(render, story_map_compatibility_text(value, room), area.x, area.y+dy, style);
        };
        constexpr Tint heading{240, 248, 249, 255}, muted{176, 209, 215, 255}, accent{255, 211, 122, 255};
        label(map.name, layout.header, 0, heading, 2*s);
        label(campaign ? "STORY MAP / IN-MEMORY RUN" : "STORY MAP / READ ONLY", layout.header, 20*s, muted, s);
        fill(layout.map_panel, {21, 31, 43, 255});
        fill(layout.map, {36, 58, 69, 255});
        if(art) fill(layout.map, {255, 255, 255, 255}, art);
        const auto focused = focus.index(map);
        auto draw_node = [&](std::size_t i) {
            const auto& node = map.nodes[i];
            const Tint color = node.selected ? accent : node.available ? Tint{124, 235, 204, 255} : muted;
            fill(layout.marker(node, (i == focused ? 8 : 5)*s), i == focused ? heading : Tint{20, 31, 41, 255});
            fill(layout.marker(node, (i == focused ? 5 : 3)*s), color);
        };
        for(std::size_t i = 0; i < map.nodes.size(); ++i) if(i != focused) draw_node(i);
        fill(layout.details, {30, 44, 59, 255});
        if(focused < map.nodes.size()) {
            draw_node(focused);
            const auto& node = map.nodes[focused];
            const StoryMapRect inset{layout.details.x+6*s, layout.details.y+4*s,
                layout.details.w-12*s, layout.details.h-4*s};
            label("NODE " + std::to_string(focused+1) + "/" + std::to_string(map.nodes.size()), inset, 0, heading, s);
            label("KEY: " + node.label_key, inset, 12*s, muted, s);
            label(node.available ? (campaign ? "ENTER / A TO PLAY" : "AVAILABLE TO CHECK") :
                node.selected ? "CURRENT NODE" : "INSPECT ONLY",
                inset, 24*s, node.selected ? accent : muted, s);
            if(!status.empty()) label(status, inset, 36*s, muted, s);
            else if(!map.map_art.empty() && !art) label("MAP ART UNAVAILABLE", inset, 36*s, accent, s);
        }
        label("ARROWS / CLICK NODE   ESC CLOSE", layout.footer, 0, muted, s);
        render.state() = saved;
        return draws;
    }
private:
    TextRenderer text_;
    std::vector<Vertex> quads_;
};

} // namespace gf
