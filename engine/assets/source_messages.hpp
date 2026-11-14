#pragma once
#include "engine/assets/source_font.hpp"
namespace gf {
// GFTX contains explicit encoded lines exported from bounded source message
// tokens. No runtime MDT relocation or unsupported dynamic substitution.
class SourceMessageBank {
public:
    using Lines=std::vector<std::string>;
    bool load(ByteView b,const SourceFont& font,std::string* error){
        groups_.clear();hash_=0;auto fail=[&](){if(error)*error="source messages: extent/encoding";return false;};
        if(!font.ready()||!b.data||b.size<12||b.size>(1u<<20)||b.be32(0)!=0x47465458u||b.be32(4)!=1||!b.be16(8)||b.be16(8)>16||b.be16(10))return fail();
        std::size_t at=12;std::vector<std::vector<Lines>> groups;
        for(unsigned g=0;g<b.be16(8);++g){if(!b.contains(at,2))return fail();const auto count=b.be16(at);at+=2;if(!count||count>4096)return fail();std::vector<Lines> group;
            for(unsigned m=0;m<count;++m){if(!b.contains(at,2))return fail();const auto count_lines=b.be16(at);at+=2;if(!count_lines||count_lines>32)return fail();Lines lines;
                for(unsigned l=0;l<count_lines;++l){if(!b.contains(at,2))return fail();const auto size=b.be16(at);at+=2;if(size>1024||size%2||!b.contains(at,size))return fail();
                    for(unsigned i=0;i<size;i+=2){const auto code=b.be16(at+i);if(code!=font.blank()&&font.glyph(code)<0)return fail();}
                    lines.emplace_back(reinterpret_cast<const char*>(b.data+at),size);at+=size;
                }group.push_back(std::move(lines));
            }groups.push_back(std::move(group));
        }
        if(at!=b.size)return fail();
        groups_=std::move(groups);hash_=2166136261u;for(std::size_t i=0;i<b.size;++i)hash_=(hash_^b.data[i])*16777619u;if(error)error->clear();return true;
    }
    const Lines* message(unsigned group,unsigned index)const{return group<groups_.size()&&index<groups_[group].size()?&groups_[group][index]:nullptr;}
    u32 hash()const{return hash_;}unsigned groups()const{return unsigned(groups_.size());}
private:
    std::vector<std::vector<Lines>> groups_;u32 hash_=0;
};
} // namespace gf
