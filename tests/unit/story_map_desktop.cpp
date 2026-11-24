#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include "engine/core/headless_backends.hpp"
#include "platform/pc/portable_story_map.hpp"

namespace {
namespace fs = std::filesystem;
using namespace gf;

void check(bool ok, const char* why) {
    if(!ok) {
        std::fprintf(stderr, "Story map desktop: %s\n", why);
        std::abort();
    }
}

constexpr std::string_view graph =
    "GFSTORY 1\nid=example:map\nname=Workshop \xe5\xb7\xa5\xe6\x88\xbf long untranslated title\n"
    "start=center\nmap-art=art/map.tpl\nnode=center|text.a-very-long-unlocalized-label-key|500|500|example:missing|round\n"
    "node=corner|text.corner|1000|0|none|none\nedge=center|win|corner\n";

struct Recorder final : IRenderBackend {
    std::vector<Vertex> vertices;
    std::vector<DrawCall> calls;
    bool drawable = false;
    u32 texture = 0;
    unsigned uploads = 0, releases = 0;
    bool initialize(const RenderConfig&, IWindowBackend*, std::string*) override {return true;}
    void shutdown() override {}
    RenderCapabilities capabilities() const override {return {4096, false, false, "story map test"};}
    bool upload_vertices(const Vertex* data, std::size_t count) override {
        vertices.clear();
        if(count) vertices.assign(data, data+count);
        return true;
    }
    bool upload_texture(u32 handle, const TextureDesc& desc) override {
        check(handle && desc.width == 8 && desc.height == 4 && desc.pixels, "decoded map art upload");
        texture = handle;
        ++uploads;
        return true;
    }
    bool release_texture(u32 handle) override {
        check(handle && handle == texture, "only release owned map texture");
        texture = 0;
        ++releases;
        return true;
    }
    bool upload_matrix(u32, const f32*) override {return true;}
    void begin_frame(const FrameDesc& frame) override {calls.clear(); drawable = frame.drawable_coordinates;}
    void submit(const DrawCall& call) override {calls.push_back(call);}
    void end_frame() override {}
    void present() override {}
};

#if defined(__linux__)
struct Temp {
    fs::path path = fs::temp_directory_path() /
        ("gf-story-map-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    Temp() {check(fs::create_directory(path), "create test root");}
    ~Temp() {std::error_code ignored; fs::remove_all(path, ignored);}
};

void write(const fs::path& path, std::string_view text) {
    fs::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary);
    check(bool(file.write(text.data(), std::streamsize(text.size()))), "write test fixture");
}

std::vector<u8> map_art() {
    std::vector<u8> bytes(56+128, 255);
    store_be32(bytes.data(), kTplMagic);
    store_be32(bytes.data()+4, 1);
    store_be32(bytes.data()+8, 12);
    store_be32(bytes.data()+12, 20);
    store_be32(bytes.data()+16, 0);
    store_be16(bytes.data()+20, 4);
    store_be16(bytes.data()+22, 8);
    store_be32(bytes.data()+24, 6);
    store_be32(bytes.data()+28, 56);
    return bytes;
}
#endif
}

int main() {
    using namespace gf;
    StoryManifest manifest;
    check(story_read_manifest(graph, &manifest), "parse fixture");
    const auto projection = story_map_preview(manifest);
    StoryMapFocus focus;
    check(projection.nodes.size() == 2 && projection.nodes[0].selected &&
          !projection.nodes[0].available && focus.index(projection) == 0, "preview neither resolves nor admits encounter");
    check(focus.step(projection, false) && focus.index(projection) == 1 && projection.nodes[0].selected,
          "inspect different node without moving progression");
    auto reordered = projection;
    std::reverse(reordered.nodes.begin(), reordered.nodes.end());
    check(focus.index(reordered) == 0 && focus.index(projection) == 1, "focus remains bound to stable node ID");
    check(story_map_compatibility_text("A \xc3\x89 \xe5\xb7\xa5 long", 80) == "A ? ? LONG" &&
          story_map_compatibility_text("long label", 7) == "LONG...", "UTF-8 scalar replacement and visible clipping");

    StoryMapView view;
    Recorder backend;
    RenderEngine render(backend);
    check(render.initialize({}, nullptr, nullptr), "initialize recorder");
    const std::array<std::array<unsigned, 4>, 6> sizes{{
        {640, 480, 640, 480}, {1280, 720, 1280, 720}, {2560, 720, 2560, 720},
        {280, 540, 280, 540}, {1280, 960, 640, 480}, {960, 720, 640, 480}}};
    for(const auto& [pw, ph, ww, wh] : sizes) {
        const auto layout = StoryMapLayout::fit(pw, ph, ww, wh, 16, 9);
        check(layout.ready && layout.stacked == (ww == 280) &&
              layout.scale == (pw==2*ww ? 2 : 1), "wide/narrow reflow and uniform HiDPI font scale");
        check(std::abs(layout.map.w*9-layout.map.h*16) <= 16, "map preserves aspect to physical-pixel rounding");
        const auto [x, y] = layout.position(projection.nodes[0]);
        const s32 wx = s32(std::round(x*ww/pw)), wy = s32(std::round(y*wh/ph));
        check(layout.hit_node(wx, wy, projection, focus) == std::optional<std::size_t>{0},
              "draw center agrees with pointer across viewport and framebuffer scales");
        check(!layout.hit_node(-1, wy, projection, focus) && !layout.hit_node(ww, wy, projection, focus),
              "pointer outside window rejected");
        const auto [cx, cy] = layout.position(projection.nodes[1]);
        check(layout.hit_node(s32(std::floor((cx-2)*ww/pw)), s32(std::ceil((cy+2)*wh/ph)), projection, focus) ==
              std::optional<std::size_t>{1}, "clipped marker on top-right boundary still hit inside");
        if(layout.map.y > layout.map_panel.y)
            check(!layout.hit_node(s32(cx*ww/pw)-1, s32((cy-2)*wh/ph), projection, focus), "letterbox bars are not targets");

        render.begin_frame({0, 0.0f, 0.0f, 0.0f, true});
        render.reset_arena();
        render.state().depth_write = true;
        check(view.draw(render, layout, projection, focus, 77) > 6 && render.state().depth_write && render.end_frame(),
              "shared renderer draws and restores state");
        const auto art = std::find_if(backend.calls.begin(), backend.calls.end(), [](const auto& c){return c.texture == 77;});
        check(backend.drawable && art != backend.calls.end() && art->vertex_count == 6 &&
              backend.vertices[art->vertex_first].x == layout.map.x && backend.vertices[art->vertex_first].y == layout.map.y &&
              backend.vertices[art->vertex_first+2].x == layout.map.x+layout.map.w &&
              backend.vertices[art->vertex_first+2].y == layout.map.y+layout.map.h,
              "art uses the exact fitted rectangle");
        for(const auto& vertex : backend.vertices)
            check(vertex.x >= 0 && vertex.x <= pw && vertex.y >= 0 && vertex.y <= ph, "text and geometry stay in drawable");
    }
    check(!StoryMapLayout::fit(0, 0, 500, 300).ready && !StoryMapLayout::fit(640, 480, 0, 0).ready,
          "minimized or unavailable window does not hit or draw");

#if defined(__linux__)
    Temp temp;
    const auto root = temp.path/"stories", other = temp.path/"more-stories";
    check(pc::scan_portable_stories(root).status == PortableStoryScanStatus::NoDirectory, "absent root is not failure");
    write(root/"map"/"story.manifest", graph);
    fs::create_directories(root/"missing");
    write(root/"oversize"/"story.manifest", std::string(kStoryManifestBytes+1, 'x'));
    fs::create_directory_symlink(root/"map", root/"linked-package");
    fs::create_directories(root/"linked-file");
    fs::create_symlink(root/"map"/"story.manifest", root/"linked-file"/"story.manifest");
    fs::create_directories(root/"hardlinked-file");
    write(temp.path/"external-manifest", graph);
    fs::create_hard_link(temp.path/"external-manifest", root/"hardlinked-file"/"story.manifest");
    const auto catalog = pc::scan_portable_stories(root);
    check(catalog.status == PortableStoryScanStatus::Complete && catalog.discovered.size() == 1 &&
          catalog.issues.size() == 5 && catalog.find("example:map"), "bounded manifest discovery rejects links and oversize input");
    std::string error;
    TplImage image;
    const auto* entry = catalog.find("example:map");
    const std::array roots{root, other};
    check(!pc::load_portable_story_map_art(roots, *entry, &image, &error), "missing optional art does not hide graph");
    const auto bytes = map_art();
    const auto art_path = root/"map"/"art"/"map.tpl";
    write(temp.path/"external-map", {reinterpret_cast<const char*>(bytes.data()), bytes.size()});
    fs::create_directories(art_path.parent_path());
    fs::create_symlink(temp.path/"external-map", art_path);
    check(!pc::load_portable_story_map_art(roots, *entry, &image, &error), "linked art refused on demand");
    fs::remove(art_path);
    write(art_path, {reinterpret_cast<const char*>(bytes.data()), bytes.size()});
    check(pc::load_portable_story_map_art(roots, *entry, &image, &error) && image.width == 8 && image.height == 4,
          "valid map art uses shared TPL decoder");
    auto changed = std::string(graph);
    changed.replace(changed.find("Workshop"), 8, "Renamed");
    write(root/"map"/"story.manifest", changed);
    check(!pc::load_portable_story_map_art(roots, *entry, &image, &error) && image.width == 8,
          "changed graph refuses art and leaves prior output intact");
    write(root/"map"/"story.manifest", graph);
    write(other/"duplicates"/"story.manifest", graph);
    const auto duplicates = pc::scan_portable_stories(roots);
    check(!duplicates.find("example:map") && duplicates.issues.size() == 7, "duplicate ID refused across roots");
    const std::array<fs::path, pc::kPortableStoryScanRoots+1> over_budget{};
    check(pc::scan_portable_stories(over_budget).status == PortableStoryScanStatus::EntryBudget, "root budget");

    headless::Bundle headless;
    auto backends = headless.get();
    backends.save = nullptr; // Browsing must never consult the save layer.
    Context context(backends, 60);
    unsigned captures = 0;
    check(pc::run_portable_story_map(context, render, root, "example:map", {true, 2, 0},
              [&](unsigned) {++captures;}) == 0 && captures == 2 && backend.uploads == 1 && backend.releases == 1,
          "headless host draw/texture lifecycle and read-only inspection without a save backend");
#endif
    render.shutdown();
}
