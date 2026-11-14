#pragma once
#include <algorithm>
#include <span>
#include <string_view>
#include "engine/assets/tpl.hpp"
namespace gf {
// Explicit encoded-character map plus original atlas. No font substitution or
// host locale dependency. Unknown glyphs refuse; source blank advances only.
class SourceFont {
public:
    bool load(ByteView profile,ByteView texture,std::string* error){
        ready_=false;codes_.clear();image_={};auto fail=[&](const char* why){if(error)*error=std::string("source font: ")+why;return false;};
        if(!profile.data||profile.size<24||profile.be32(0)!=0x4746464eu||profile.be32(4)!=1)return fail("GFFN header");
        const auto count=profile.be16(12),cell=profile.be16(14),columns=profile.be16(16),rows=profile.be16(18),blank=profile.be16(20);
        if(!count||count>1024||cell<1||cell>128||!columns||!rows||u32(columns)*rows<count||profile.be16(22)||profile.size!=24+std::size_t(count)*2)return fail("glyph geometry/counts");
        if(!texture.data||texture.size>(16u<<20))return fail("texture budget");
        u32 hash=2166136261u;for(std::size_t i=0;i<texture.size;++i)hash=(hash^texture.data[i])*16777619u;
        if(hash!=profile.be32(8))return fail("paired atlas consistency");
        std::vector<u16> codes;for(unsigned i=0;i<count;++i){const auto code=profile.be16(24+i*2);const auto high=code>>8;if(!((high>=0x80&&high<=0x9f)||high>=0xe0)||std::find(codes.begin(),codes.end(),code)!=codes.end())return fail("encoded glyph identity");codes.push_back(code);}
        TplImage image;if(!tpl_decode(texture,0,&image,error)||image.image_count!=1||image.width!=u32(cell)*columns||image.height!=u32(cell)*rows)return fail("atlas dimensions");
        image_=std::move(image);codes_=std::move(codes);cell_=cell;columns_=columns;blank_=blank;ready_=true;if(error)error->clear();return true;
    }
    bool ready()const{return ready_;}const TplImage& image()const{return image_;}
    u16 cell()const{return cell_;}u16 columns()const{return columns_;}u16 blank()const{return blank_;}
    std::span<const u16> codes()const{return codes_;}
    int glyph(u16 code)const{if(!ready_)return -1;for(unsigned i=0;i<codes_.size();++i)if(codes_[i]==code)return int(i);return -1;}
    // Keyboard ASCII is explicitly mapped to the supported encoded characters.
    // This is not a general Shift-JIS conversion or an arbitrary-byte name API.
    bool encode_ascii(std::string_view text,std::string* out)const{
        if(!out)return false;
        out->clear();if(!ready_||text.size()>512)return false;std::string result;
        for(unsigned char c:text){u16 code=0;if(c==' ')code=blank_;else if(c>='0'&&c<='9')code=u16(0x824f+c-'0');else if(c>='A'&&c<='Z')code=u16(0x8260+c-'A');else if(c>='a'&&c<='z')code=u16(0x8281+c-'a');else{switch(c){case '.':code=0x8144;break;case ',':code=0x8143;break;case '!':code=0x8149;break;case '?':code=0x8148;break;case ':':code=0x8146;break;case '-':code=0x817c;break;case '/':code=0x815e;break;case '\'':code=0x8166;break;default:return false;}}
            if(code!=blank_&&glyph(code)<0)return false;
            result+=char(code>>8);result+=char(code);
        }
        *out=std::move(result);return true;
    }
private:
    bool ready_=false;u16 cell_=0,columns_=0,blank_=0;std::vector<u16> codes_;TplImage image_;
};
} // namespace gf
