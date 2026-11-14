#pragma once
#include <limits>
#include "engine/graphics/source_fixed_text_metrics.hpp"
#include "engine/graphics/alpha_text_region.hpp"
namespace gf {
struct FixedParagraphLayout {std::vector<std::string> text;std::vector<CenteredTextLine> lines;bool reflowed=false;};
//Native owner-requested paragraph layout. Preserve source line breaks if they
//fit. Otherwise choose a word-preserving balanced two-line split at unchanged
//font extent/advance; never shrink the font, change wording, or clip overflow.
inline bool layout_fixed_paragraph(std::span<const std::string> source,u16 blank,
                                   const SourceFixedTextMetrics& metrics,const AlphaTextRegion& region,
                                   f32 extent,f32 advance,f32 step,f32 sx,f32 sy,f32 inset,FixedParagraphLayout* out){
    if(out)*out={};
    if(!out||source.empty()||source.size()>2)return false;
    auto place=[&](const std::vector<std::string>& text,std::vector<CenteredTextLine>* lines){
        std::vector<SourceTextMeasure> measures(text.size());
        for(unsigned i=0;i<text.size();++i)if(!metrics.measure(text[i],extent,advance,&measures[i]))return false;
        return region.center(measures,step,sx,sy,inset,lines);
    };
    FixedParagraphLayout next;next.text.assign(source.begin(),source.end());
    if(place(next.text,&next.lines)){*out=std::move(next);return true;}
    std::vector<std::string> words;std::string word;std::size_t total=0;
    for(const auto& line:source){if(line.size()%2||line.size()>1024)return false;total+=line.size()+2;if(total>1024)return false;
        for(unsigned at=0;at<line.size();at+=2){const u16 code=u16(u16(u8(line[at]))<<8|u8(line[at+1]));
            if(code==blank){if(!word.empty()){words.push_back(std::move(word));word.clear();}}
            else word.append(line,at,2);
        }
        if(!word.empty()){words.push_back(std::move(word));word.clear();}
    }
    if(words.size()<2||words.size()>64)return false;
    const std::string space{char(blank>>8),char(blank)};
    auto join=[&](unsigned first,unsigned last){std::string text;for(unsigned i=first;i<last;++i){if(i!=first)text+=space;text+=words[i];}return text;};
    bool found=false;f32 best=std::numeric_limits<f32>::infinity(),difference=best;
    for(unsigned split=1;split<words.size();++split){
        FixedParagraphLayout candidate;candidate.text={join(0,split),join(split,unsigned(words.size()))};
        if(!place(candidate.text,&candidate.lines)||candidate.lines.size()!=2)continue;
        const f32 a=candidate.lines[0].ink.right-candidate.lines[0].ink.left,b=candidate.lines[1].ink.right-candidate.lines[1].ink.left;
        const f32 width=std::max(a,b),delta=std::abs(a-b);
        if(!found||width<best||(width==best&&delta<difference)){found=true;best=width;difference=delta;candidate.reflowed=true;next=std::move(candidate);}
    }
    if(!found)return false;
    *out=std::move(next);return true;
}
} //namespace gf
