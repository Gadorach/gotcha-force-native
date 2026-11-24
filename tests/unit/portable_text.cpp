#include <cstdio>
#include <cstdlib>
#include "engine/graphics/portable_text.hpp"

namespace {
using namespace gf;
constexpr std::string_view kManifest=
    "GFFONT 1\r\n# Made-up fixture atlas \xC3\xA9\r\natlas=art/ui.a8\r\nsize=16|16\r\n"
    "line-height=10\r\nascent=7\r\n"
    "glyph=U+FFFD|8|8|3|4|0|4|4\r\n"
    "glyph=U+0020|0|0|0|0|0|0|3\r\n"
    "glyph=U+0041|1|1|3|5|-1|5|4\r\n"
    "glyph=U+00E9|5|1|2|4|0|4|3\r\n"
    "glyph=U+754C|8|1|4|5|1|6|6\r\n"
    "glyph=U+1F642|1|8|4|4|0|4|5\r\n"
    "glyph=U+0301|13|1|1|1|-1|7|0\r\n";
constexpr std::string_view kLabel="A\xC3\xA9\n\xE7\x95\x8C";
void check(bool ok,const char* what){
    if(!ok){std::fprintf(stderr,"portable text: %s\n",what);std::abort();}
}
std::string replace(std::string_view old,std::string_view value){
    std::string next(kManifest);const auto at=next.find(old);
    check(at!=std::string::npos,"fixture replacement matches text");
    next.replace(at,old.size(),value);return next;
}
class RecordingRender final:public IRenderBackend {
public:
    std::vector<Vertex> vertices;
    std::vector<DrawCall> calls;
    std::vector<u8> pixels;
    unsigned uploads=0,matrices=0,cap=1024;
    bool fail_upload=false;
    bool initialize(const RenderConfig&,IWindowBackend*,std::string*)override{return true;}
    void shutdown()override{pixels.clear();}
    RenderCapabilities capabilities()const override{return {cap,false,false,"portable text fixture"};}
    bool upload_vertices(const Vertex* v,std::size_t count)override{
        if(!count)vertices.clear();else vertices.assign(v,v+count);
        return true;
    }
    bool upload_texture(u32 id,const TextureDesc& desc)override{
        if(fail_upload)return false;
        check(id==71&&desc.width==16&&desc.height==16&&!desc.linear&&!desc.wrap_s&&!desc.wrap_t,"shared atlas dimensions, clamp and nearest");
        ++uploads;pixels.assign(desc.pixels,desc.pixels+std::size_t(desc.width)*desc.height*4);return true;
    }
    bool upload_matrix(u32,const f32*)override{++matrices;return true;}
    void begin_frame(const FrameDesc&)override{calls.clear();vertices.clear();}
    void submit(const DrawCall& draw)override{calls.push_back(draw);}
    void end_frame()override{}
    void present()override{}
};
}

int main(){
    PortableFontMetrics metrics;
    std::string error;
    check(PortableFontMetrics::parse(kManifest,&metrics,&error)&&error.empty()&&metrics.ready()&&
        metrics.atlas_path()=="art/ui.a8"&&metrics.atlas_bytes()==256&&metrics.ascent()==7&&metrics.line_height()==10&&
        metrics.find(0x1f642)&&metrics.find(0x301)->advance==0&&!metrics.find(0x1f643),"strict metrics accept arbitrary order and full Unicode scalar range");
    const std::string invalid[]{
        replace("GFFONT 1","GFFONT 2"),replace("art/ui.a8","../ui.a8"),replace("art/ui.a8","/ui.a8"),
        replace("size=16|16","size=1025|16"),replace("size=16|16","size=16|0"),replace("size=16|16","size=16|16|16"),
        replace("line-height=10","line-height=0"),replace("ascent=7","ascent=11"),replace("ascent=7\r\n",""),
        replace("U+1F642","U+01F642"),replace("U+1F642","U+1f642"),replace("U+1F642","U+110000"),
        replace("U+1F642","U+0000"),replace("U+1F642","U+000A"),replace("U+1F642","U+0085"),replace("U+1F642","U+D800"),
        replace("8|8|3|4|0|4|4","16|8|3|4|0|4|4"),replace("8|8|3|4|0|4|4","8|8|0|4|0|4|4"),
        replace("8|8|3|4|0|4|4","8|8|3|4|0|4|-1"),replace("8|8|3|4|0|4|4","8|8|3|4|0|4|0"),
        replace("3|5|-1|5|4","3|5|-257|5|4"),replace("3|5|-1|5|4","3|5|-1|5|257"),
        replace("U+0020|0|0|0|0|0|0|3","U+0020|1|0|0|0|0|0|3"),replace("U+0020|0|0|0|0|0|0|3","U+0020|0|0|0|0|0|0|0"),
        replace("U+FFFD","U+FFFE"),replace("\xC3\xA9","\xED\xA0\x80"),
        std::string(kManifest)+"scale=2\n",std::string(kManifest)+"ascent=0\n",std::string(kManifest)+"glyph=U+0041|0|0|0|0|0|0|1\n",
        std::string(kManifest)+std::string(kPortableFontManifestBytes,'#'),
    };
    for(const auto& text:invalid)
        check(!PortableFontMetrics::parse(text,&metrics,&error)&&!error.empty()&&metrics.find(0x1f642),
            "bad manifests are rejected without changing the previously validated metric set");
    std::string full(kManifest);
    for(unsigned i=0;i<kPortableFontMaxGlyphs-7;++i){
        char code[16];std::snprintf(code,sizeof code,"U+%04X",0x2000u+i);
        full+="glyph="+std::string(code)+"|0|0|0|0|0|0|1\n";
    }
    PortableFontMetrics largest;
    check(PortableFontMetrics::parse(full,&largest,&error)&&
        !PortableFontMetrics::parse(full+"glyph=U+5000|0|0|0|0|0|0|1\n",&largest,&error),"4096-glyph ceiling enforced");

    RecordingRender device;
    RenderEngine render(device);
    check(render.initialize({},nullptr,&error),"initialize shared renderer");
    std::array<u8,256> alpha{};
    alpha[17]=255;alpha[18]=128;alpha[21]=160;alpha[24]=250;alpha[29]=190;alpha[129]=125;alpha[136]=220;
    PortableFont font;
    check(!font.bind(render,71,metrics,{alpha.data(),alpha.size()-1},&error)&&!font.available(render),"short image refused");
    std::array<u8,256> transparent{};
    check(!font.bind(render,71,metrics,transparent,&error)&&!device.uploads,"replacement rectangle needs visible coverage");
    device.cap=8;
    check(!font.bind(render,71,metrics,alpha,&error)&&!device.uploads,"renderer capability bound checked before upload");
    device.cap=1024;
    device.fail_upload=true;
    check(!font.bind(render,71,metrics,alpha,&error)&&!font.available(render)&&!render.shared_texture_upload_failed(),
        "failed optional upload leaves fallback without invalidating unrelated texture work");
    device.fail_upload=false;
    check(font.bind(render,71,metrics,alpha,&error)&&font.available(render)&&error.empty()&&device.uploads==1&&
        device.pixels[17*4]==255&&device.pixels[18*4+3]==128&&device.pixels[136*4+3]==220&&device.pixels[3]==0,
        "alpha-coverage bytes expand to unpremultiplied white RGBA");
    check(font.bind(render,71,metrics,alpha)&&device.uploads==1,"byte-identical shared binding needs no new upload");
    auto changed=alpha;changed[17]=1;
    check(!font.bind(render,71,metrics,changed,&error)&&font.available(render)&&device.uploads==1,"collision leaves prior binding valid");
    RecordingRender unrelated_device;
    RenderEngine unrelated(unrelated_device);
    PortableFont other;
    check(other.bind(unrelated,71,metrics,alpha)&&!font.available(unrelated)&&!other.available(render),"same handle cannot cross renderer scopes");

    PortableTextRenderer text;
    PortableTextStyle style;style.scale=2;
    style.tint={17,42,89,130};
    const auto measured=text.measure(render,&font,kLabel,style);
    check(measured.width==16&&measured.height==40&&measured.lines==2&&!measured.clipped,"advances, ink bearing, multiline and scale agree");
    check(text.measure(render,&font,"\xF0\x9F\x99\x82",style).width==10,"a supplementary scalar is one glyph");
    check(text.measure(render,&font,"A\xCC\x81",style).width==10,"zero advance mark receives its authored placement without shaping");
    check(text.measure(render,&font,"\xC0\xAF" "\xE2\x98\x83" "\xF4\x90\x80\x80",style).width==56,
        "each malformed byte and each unmapped scalar uses one replacement");
    check(text.measure(render,&font,std::string_view("\0",1),style).width==8,"runtime control uses a replacement");
    std::string long_text;
    for(unsigned i=0;i<kPortableTextMaxUnits;++i)long_text+="\xF0\x9F\x99\x82";
    check(!text.measure(render,&font,long_text,style).clipped,"exact byte and scalar ceilings are accepted");
    long_text+='A';
    check(text.measure(render,&font,long_text,style).clipped,"text beyond ceilings clips deterministically");

    RenderState world;
    world.depth_write=true;world.texture=80;world.vertex_format=7;world.position_matrix=6;
    world.colour_write=false;world.cull_frontface=true;world.alpha_test=true;world.fog={true,1,3,{0,0,0}};
    render.state()=world;render.begin_frame({1});render.reset_arena();
    check(!font.bind(render,71,metrics,alpha,&error)&&font.available(render),"frame-active font bind refused atomically");
    check(text.draw(render,&font,kLabel,20,10,style)==1&&render.end_frame()&&device.calls.size()==1&&device.vertices.size()==18,
        "one shared draw and one quad per visible atlas glyph");
    const auto& call=device.calls[0];
    check(call.texture==71&&call.position_matrix==kIdentityPositionMatrix&&!call.depth_test&&!call.depth_write&&
        !call.cull_frontface&&!call.alpha_test&&!call.fog.enabled&&call.colour_write&&call.blend&&call.blend_src==5&&call.blend_dst==6&&
        render.state().texture==80&&render.state().vertex_format==7&&render.state().position_matrix==6&&
        !render.state().colour_write&&render.state().alpha_test&&render.state().fog==world.fog,"screen state is isolated then restored");
    check(device.vertices[0].x==20&&device.vertices[0].y==14&&device.vertices[0].u==1.f/16&&device.vertices[0].v==1.f/16&&
        device.vertices[2].x==26&&device.vertices[2].y==24&&device.vertices[0].r==17&&device.vertices[0].a==130&&
        device.vertices[6].x==30&&device.vertices[12].x==22&&device.vertices[12].y==32,
        "UVs, scale, baseline, signed bearing and tint reach the shared geometry");
    style.max_width=12;
    const auto clipped=text.measure(render,&font,kLabel,style);
    render.begin_frame({2});render.reset_arena();
    check(clipped.width==12&&clipped.lines==2&&clipped.clipped&&text.draw(render,&font,kLabel,20,10,style)==1&&
        render.end_frame()&&device.vertices.size()==12&&device.vertices[6].y==32,
        "width clipping drops whole glyphs and resumes on the next explicit line");
    render.begin_frame({3});render.reset_arena();
    check(text.draw(render,&font,"  \n",20,10,style)==0&&render.end_frame()&&device.calls.empty(),"whitespace measures without ink submissions");

    PortableTextStyle fallback;
    const std::string_view compatibility="a\xC3\xA9_\xF0\x9F\x99\x82\nA";
    const auto bitmap=text.measure(render,nullptr,compatibility,fallback);
    check(bitmap.width==23&&bitmap.height==18&&!bitmap.clipped,"fallback normalizes one visible substitute per unsupported scalar");
    fallback.max_width=11;
    render.begin_frame({4});render.reset_arena();
    check(text.measure(unrelated,&font,compatibility,fallback).width==11&&text.draw(render,nullptr,compatibility,1,2,fallback)==1&&
        render.end_frame()&&device.calls.size()==1&&device.calls[0].texture==0&&
        device.calls[0].position_matrix==kIdentityPositionMatrix&&!device.matrices,
        "fallback uses existing bitmap text and clipping without rewriting matrix palette slots");
    check(text.measure(render,nullptr,std::string(500,'A')).clipped,"legacy vertex budget clips on a whole glyph");
    render.shutdown();
    check(!font.available(render)&&text.measure(render,&font,compatibility).width==23,"renderer shutdown safely falls back");
    check(render.initialize({},nullptr,&error)&&other.bind(render,71,metrics,alpha)&&!font.available(render),
        "new session with the same slot cannot revive a stale binding");
    std::array<u8,1024> rgba{};
    check(render.upload_texture(71,{16,16,rgba.data(),0,0,false})&&!other.available(render)&&
        font.bind(render,71,metrics,alpha)&&!other.available(render),"replacing a handle never aliases a previous binding");
    std::puts("portable font/UTF-8 text: bounds, texture lifetime, measured atlas drawing and legacy fallback PASS");
}
