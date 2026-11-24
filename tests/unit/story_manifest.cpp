#include <cstdio>
#include <cstdlib>
#include "engine/assets/story_manifest.hpp"

namespace {
void check(bool ok, const char* why) {
    if(!ok) {
        std::fprintf(stderr, "Story manifest: %s\n", why);
        std::abort();
    }
}

constexpr std::string_view header =
    "GFSTORY 1\nid=example:workshop-story\nname=Workshop \xc3\x89lite\nstart=arrival\n";
constexpr std::string_view graph =
    "edge=arrival|win|ending\n"
    "node=ending|text.ending|1000|0|none|none\n"
    "node=arrival|text.arrival|240|370|example:workshop-trials|round-one\n";
}

int main() {
    using namespace gf;
    const auto valid = std::string(header) + "map-art=art/map.tpl\n# \xc3\x89\n" + std::string(graph);
    StoryManifest manifest;
    std::string error;
    check(story_read_manifest(valid, &manifest, &error) && error.empty(), "ordered forward references and UTF-8 name");
    check(manifest.id == "example:workshop-story" && manifest.map_art == "art/map.tpl" &&
          manifest.nodes.size() == 2 && manifest.nodes[0].id == "ending" && !manifest.nodes[0].encounter &&
          manifest.nodes[1].x == 240 && manifest.nodes[1].encounter &&
          manifest.nodes[1].encounter->encounter_id == "round-one", "graph preserves independent declaration order");

    auto reject = [&](const std::string& text, const char* why) {
        check(!story_read_manifest(text, &manifest, &error) && !error.empty() &&
              manifest.id == "example:workshop-story" && manifest.nodes.size() == 2, why);
    };
    for(const auto* row : {
        "id=example:duplicate\n", "name=Too many\n", "start=ending\n", "map-art=another.tpl\n", "callback=80170000\n",
        "node=arrival|text.duplicate|0|0|none|none\n", "node=lost|text.lost|0|0|none|none\n",
        "edge=arrival|win|arrival\n", "edge=unknown|win|arrival\n", "edge=arrival|loss|missing\n",
        "edge=ending|win|arrival\n", "edge=arrival|cancel|ending\n", "# \xc0\x80\n", "# \xed\xa0\x80\n",
        "node=extra|text.extra|0|1001|none|none\n", "node=extra|text.extra|0|-1|none|none\n",
        "node=extra|text.extra|0|+1|none|none\n", "node=extra|text.extra|0|1x|none|none\n",
        "node=extra|text.extra|0|99999999999999999|none|none\n", "node=extra||0|0|none|none\n",
        "node=extra|text.extra|0|1|none|round-one\n", "node=extra|text.extra|0|1|example:trials|\n",
        "node=extra|text.extra|0|1|example:trials|round-one|extra\n"})
        reject(valid + row, row);
    for(const auto* row : {"map-art=../map.tpl\n", "map-art=/map.tpl\n", "map-art=art\\map.tpl\n", "map-art=\n"})
        reject(std::string(header) + row + std::string(graph), row);
    auto wrong_start = valid;
    wrong_start.replace(wrong_start.find("start=arrival"), sizeof("start=arrival") - 1, "start=missing");
    reject(wrong_start, "start must resolve");
    reject("GFSTORY 2\n" + valid.substr(sizeof("GFSTORY 1\n") - 1), "unsupported schema");

    StoryManifest alternate;
    check(story_read_manifest(
        "GFSTORY 1\r\nid=example:splash\r\nname=Splash\r\nstart=end\r\nnode=end|text.end|0|1000|none|none",
        &alternate) && alternate.map_art.empty(), "CRLF, terminal start, no optional art, no final newline");
    const auto at_limit = valid + "#" + std::string(kStoryManifestBytes - valid.size() - 1, 'a');
    check(story_read_manifest(at_limit, &alternate), "precise byte ceiling");
    reject(at_limit + "a", "oversize manifest");

    std::string dense = "GFSTORY 1\nid=example:dense\nname=Dense graph\nstart=n0\n";
    for(std::size_t i=0; i<kStoryNodes; ++i) {
        const auto here = "n" + std::to_string(i);
        const auto next = "n" + std::to_string((i + 1) % kStoryNodes);
        dense += "node=" + here + "|text.node|0|0|example:trials|round-one\n";
        dense += "edge=" + here + "|win|" + next + "\nedge=" + here + "|loss|" + here + "\n";
    }
    check(story_read_manifest(dense, &alternate) && alternate.nodes.size() == kStoryNodes &&
          alternate.edges.size() == kStoryEdges, "bounded graph admits cycles, self-loops and both outcomes");
    reject(dense + "node=extra|text.extra|0|0|none|none\n", "node count bound");
    reject(dense + "edge=n0|win|n0\n", "edge count bound");
    auto programmatic = manifest;
    programmatic.edges[0].outcome = static_cast<StoryEdgeOutcome>(255);
    check(!story_validate_manifest(programmatic), "reject invalid in-memory outcome");
}
