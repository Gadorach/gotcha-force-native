#pragma once
#include "engine/assets/force_catalog.hpp"
namespace gf {
struct ForcePartRecipe {u16 model=0;u8 components=1;};
// Source reward-table order, component counts and encoded part-name fragments.
// Remaining reward-table columns are deliberately not interpreted as formulas.
class ForceSupplement {
public:
    bool assign(const ForceCatalog& catalog,std::vector<ForcePartRecipe> recipes,
                std::string prefix,std::array<std::string,6> variants,
                std::array<std::string,10> digits,std::vector<std::string> components,
                std::string* error){
        ready_=false;auto fail=[&](const char* why){if(error)*error=std::string("force supplement: ")+why;return false;};
        if(!catalog.ready()||recipes.size()>256||components.empty()||components.size()>7)return fail("resource/count domain");
        auto valid_text=[](const std::string& x){return !x.empty()&&x.size()<=47&&x.find('\0')==std::string::npos;};
        if(!valid_text(prefix))return fail("name prefix");
        std::size_t a=0,b=0,c=0;
        for(const auto& x:variants){if(!valid_text(x))return fail("variant text");a=std::max(a,x.size());}
        for(const auto& x:digits){if(!valid_text(x))return fail("digit text");b=std::max(b,x.size());}
        for(const auto& x:components){if(!valid_text(x))return fail("component text");c=std::max(c,x.size());}
        if(prefix.size()+a+3*b+c>47)return fail("source name-buffer domain");
        for(unsigned i=0;i<recipes.size();++i){const auto& r=recipes[i];if(!catalog.find(r.model)||!r.components||r.components>components.size())return fail("recipe model/count");for(unsigned j=0;j<i;++j)if(recipes[j].model==r.model)return fail("duplicate recipe");}
        key_=catalog.fingerprint();recipes_=std::move(recipes);prefix_=std::move(prefix);variants_=std::move(variants);digits_=std::move(digits);components_=std::move(components);ready_=true;if(error)error->clear();return true;
    }
    bool load(ByteView data,const ForceCatalog& catalog,std::string* error){
        ready_=false;auto fail=[&](const char* why){if(error)*error=std::string("force supplement: ")+why;return false;};
        if(!data.data||data.size<16||data.size>65536||data.be32(0)!=0x47464941u||data.be32(4)!=1||data.be32(8)!=catalog.fingerprint())return fail("GFIA resource/header");
        const auto n=data.be16(12),parts=data.be16(14);if(n>256||!parts||parts>7||!data.contains(16,std::size_t(n)*4))return fail("GFIA counts");
        std::size_t at=16;std::vector<ForcePartRecipe> recipes;
        for(unsigned i=0;i<n;++i){if(data.data[at+3])return fail("reserved recipe byte");recipes.push_back({data.be16(at),data.data[at+2]});at+=4;}
        auto read_text=[&](std::string& s){if(!data.contains(at,2))return false;const auto len=data.be16(at);at+=2;if(!len||len>47||!data.contains(at,len))return false;s.assign(reinterpret_cast<const char*>(data.data+at),len);at+=len;return true;};
        std::string prefix;std::array<std::string,6> variants;std::array<std::string,10> digits;std::vector<std::string> components(parts);
        if(!read_text(prefix))return fail("prefix extent");
        for(auto& x:variants)if(!read_text(x))return fail("variant extent");
        for(auto& x:digits)if(!read_text(x))return fail("digit extent");
        for(auto& x:components)if(!read_text(x))return fail("component extent");
        if(at!=data.size)return fail("trailing data");
        return assign(catalog,std::move(recipes),std::move(prefix),std::move(variants),std::move(digits),std::move(components),error);
    }
    bool for_catalog(const ForceCatalog& catalog)const{return ready_&&catalog.ready()&&key_==catalog.fingerprint();}
    std::span<const ForcePartRecipe> recipes()const{return recipes_;}
    unsigned required(u16 model)const{for(const auto& r:recipes_)if(r.model==model)return r.components;return 1;}
    bool part_name(const ForceCatalog& catalog,u16 model,u8 variant,u8 component,std::string* out)const{
        const auto* metadata=catalog.find(model);if(!out||!for_catalog(catalog)||!metadata||variant>=variants_.size()||component>=components_.size())return false;
        const auto n=metadata->number+1;*out=prefix_+variants_[variant]+digits_[(n/100)%10]+digits_[(n/10)%10]+digits_[n%10]+components_[component];return true;
    }
private:
    bool ready_=false;u32 key_=0;std::vector<ForcePartRecipe> recipes_;
    std::string prefix_;std::array<std::string,6> variants_;std::array<std::string,10> digits_;std::vector<std::string> components_;
};
} // namespace gf
