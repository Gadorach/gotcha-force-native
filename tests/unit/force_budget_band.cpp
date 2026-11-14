#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/force_budget_band.hpp"
namespace {unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"budget band: %s\n",why);std::abort();}}}
int main(){
    using namespace gf;ForceBudgetBandArt art;art.pulse_step=2048;art.angle_unit=0.000095873801969f;for(unsigned i=0;i<11;++i)art.parts.push_back({s16(i*100),u16(i)});std::vector<ForceBudgetBandSample> result;
    check(force_budget_band_samples(art,-1,0,0,&result)&&result.empty(),"unlimited source prefix has no band");check(force_budget_band_samples(art,0,0,0,&result)&&result.empty(),"zero admitted slots");
    check(force_budget_band_samples(art,1,0,0,&result)&&result.size()==1&&result[0].slot==0&&result[0].frame==3,"prefix excludes equality boundary and wraps left edge");
    for(unsigned turn=0;turn<30;++turn)for(s32 prefix=0;prefix<=30;++prefix){check(force_budget_band_samples(art,prefix,turn,7,&result),"finite settled turn");std::vector<u8> expected;for(unsigned segment=0;segment<11;++segment){const u32 slot=(turn+segment+27)%30,x=u32(prefix)^slot;const s32 difference=s32(x>>1)-s32(x&u32(prefix));if(difference<0)expected.push_back(u8(slot));}check(result.size()==expected.size(),"raw source signed comparison count");for(unsigned i=0;i<expected.size();++i)check(result[i].slot==expected[i]&&result[i].material>=0&&result[i].material<=1,"source order/visibility/pulse domain");}
    check(!force_budget_band_samples(art,31,0,0,&result)&&result.empty()&&!force_budget_band_samples(art,1,30,0,&result),"invalid prefix/turn refused");
    std::vector<ForceBudgetBandSample> first,last;check(force_budget_band_samples(art,30,0,0,&first)&&force_budget_band_samples(art,30,0,32,&last)&&first[0].material==last[0].material,"signed16 phase cycle retained");
    std::printf("budget band: %u owned checks PASS\n",checks);
}
