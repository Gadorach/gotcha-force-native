#pragma once
#include "engine/assets/force_catalog.hpp"
namespace gf {
// GFSD: offline authored display fields and source80066198 normal-domain HP.
// These are UI data, not live combat stats or proof of an initialized actor.
struct ForceSpecMetadata {
    u16 model=0,number=0;
    std::array<u8,13> authored{}; // character-data1A0..1AC, signed sentinels retained
    std::array<std::array<s16,10>,6> hp{};
};
struct ForceSpecMove {u8 source_slot=0,button=0,activation=0,icon=0;u16 text_index=0;};
enum class ForceSpecMoveAccess {Unknown,Unavailable,Available};
struct ForceSpecAuthoredView {
    u16 model=0,number=0;u8 variant=0,level=0,tribe=0,page_mask=0;
    s16 cost=0,rarity=0,hp=0;std::array<u8,4> ratings{};
    u8 jump_type=0,jump_count=0; s8 feature=-1;
    std::array<ForceSpecMove,5> moves{};u8 move_count=0;
};
class ForceSpecificationCatalog {
public:
    bool load(ByteView b,const ForceCatalog& catalog,std::string* error){
        ready_=false;rows_.clear();key_=0;
        auto fail=[&](const char* why){if(error)*error=std::string("force specification: ")+why;return false;};
        if(!catalog.ready()||!b.data||b.size<26||b.size>65536||b.be32(0)!=0x47465344u||b.be32(4)!=1||b.be32(8)!=catalog.fingerprint())return fail("GFSD header/catalog");
        const auto count=b.be16(12);if(!count||count!=catalog.entries().size()||b.be16(14)!=138||b.size!=26+std::size_t(count)*138)return fail("record count/extent");
        std::array<u8,5> buttons{},activation{};for(unsigned i=0;i<5;++i){buttons[i]=b.data[16+i];activation[i]=b.data[21+i];if(buttons[i]>1||activation[i]>3)return fail("move display selectors");}
        std::vector<ForceSpecMetadata> rows;
        for(unsigned i=0;i<count;++i){const auto at=26+std::size_t(i)*138;ForceSpecMetadata row;row.model=b.be16(at);row.number=b.be16(at+2);const auto& bound=catalog.entries()[i];
            if(row.model!=bound.model||row.number!=bound.number||b.data[at+4])return fail("model identity/order/reserved");
            std::copy_n(b.data+at+5,13,row.authored.begin());
            const auto& a=row.authored;if(a[0]>4||a[1]>6||a[2]>1||(a[3]!=255&&a[3]>4))return fail("jump/feature selectors");
            for(unsigned j=4;j<8;++j)if(a[j]>10)return fail("rating selector");
            for(unsigned j=8;j<13;++j)if(a[j]!=255&&a[j]>26)return fail("move icon selector");
            for(unsigned v=0;v<6;++v)for(unsigned level=0;level<10;++level){const auto hp=std::bit_cast<s16>(b.be16(at+18+(v*10+level)*2));if(hp<=0)return fail("normal-domain HP");row.hp[v][level]=hp;}
            rows.push_back(row);
        }
        rows_=std::move(rows);buttons_=buttons;activation_=activation;key_=catalog.fingerprint();ready_=true;if(error)error->clear();return true;
    }
    bool for_catalog(const ForceCatalog& catalog)const{return ready_&&catalog.ready()&&catalog.fingerprint()==key_;}
    const ForceSpecMetadata* find(u16 model)const{
        if(!ready_)return nullptr;
        const auto it=std::lower_bound(rows_.begin(),rows_.end(),model,[](const auto& a,u16 b){return a.model<b;});return it!=rows_.end()&&it->model==model?&*it:nullptr;
    }
    //801DD298/3E8 readout over supplied authored character-data fields. The
    // eventual actor owner must qualify its4AC binding after56CE0 before joining
    // this view to the full 3D specification. No actor-init completion here.
    //803BFE78[number] is runtime collection state, not an initialized DOL table.
    // Its availability observation must be supplied explicitly; unknown refuses.
    bool authored_view(const ForceCatalog& catalog,u16 model,unsigned variant,unsigned level,ForceSpecMoveAccess access,ForceSpecAuthoredView* out)const{
        if(!out)return false;
        *out={};if(!for_catalog(catalog)||variant>=6||level>=10||(access!=ForceSpecMoveAccess::Available&&access!=ForceSpecMoveAccess::Unavailable))return false;
        const auto* row=find(model);const auto* metadata=catalog.find(model);if(!row||!metadata)return false;
        ForceSpecAuthoredView v;v.model=model;v.number=row->number;v.variant=u8(variant);v.level=u8(level+1);v.tribe=u8(metadata->tribe);v.cost=metadata->cost[variant];v.rarity=metadata->rarity[variant];v.hp=row->hp[variant][level];
        const auto& a=row->authored;v.jump_type=a[0];v.jump_count=a[2]?6:a[1];v.feature=std::bit_cast<s8>(a[3]);std::copy_n(a.begin()+4,4,v.ratings.begin());
        for(unsigned i=0;i<5;++i)if(a[8+i]!=255){v.moves[v.move_count++]={u8(i),buttons_[i],activation_[i],a[8+i],u16(row->number*5+i)};}
        v.page_mask=u8(access==ForceSpecMoveAccess::Available&&v.move_count?7:3);*out=v;return true;
    }
private:
    bool ready_=false;u32 key_=0;std::array<u8,5> buttons_{},activation_{};std::vector<ForceSpecMetadata> rows_;
};
} // namespace gf
