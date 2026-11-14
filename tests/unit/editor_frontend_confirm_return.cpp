#include <cstdio>
#include <cstdlib>
#include "tests/unit/editor_review_corrections_fixture.hpp"
namespace {using namespace gf;using namespace editor_review_corrections_fixture;unsigned checks=0;void check(bool x,const char* why){++checks;if(!x){std::fprintf(stderr,"MAIN confirm return: %s\n",why);std::abort();}}}
int main(){using namespace gf;using namespace editor_review_corrections_fixture;
    for(unsigned port=0;port<4;++port){Fixture f;EditorFrontend host;check(f.load()&&f.store.load(f.catalog,&f.owner)==ForceInventoryLoad::Missing&&f.open(host,4),"fresh MAIN/editor fixture with explicit Missing save read");std::array<GamePadState,4> pads{};pads[0]=pad(0x100);check(host.update(pads,{},0,0)&&host.view()==EditorFrontendView::Editor,"normal menu activation");pads={};check(host.update(pads,{},0,0),"release confirm");pads[port]=pad(0x1000);check(host.update(pads,{},0,0x80)&&host.view()==EditorFrontendView::Main&&f.save.writes[0]==1,"Start exits editor to MAIN once");
        //The physical Enter key used Start in editor but A on MAIN in the host.
        for(unsigned tick=0;tick<70;++tick){pads[port]=pad(tick&1?0x100:0x1000);check(host.update(pads,{},0,0)&&host.view()==EditorFrontendView::Main&&f.save.writes[0]==1,"held A/Start alias cannot immediately reopen menu after return");}
        pads={};check(host.update(pads,{},0,0)&&host.view()==EditorFrontendView::Main,"physical release clears quarantine without activating");pads[port]=pad(0x1000);check(host.update(pads,{},0,0)&&host.view()==EditorFrontendView::Editor,"new Start edge works after real release on same port");
    }
    {Fixture f;EditorFrontend host;check(f.load()&&f.open(host,3),"unavailable destination prompt fixture");std::array<GamePadState,4> pads{};pads[0]=pad(0x100);check(host.update(pads,{},0,0)&&host.view()==EditorFrontendView::Unavailable,"enter unavailable prompt");pads={};check(host.update(pads,{},0,0),"release entry");pads[0]=pad(0x100);check(host.update(pads,{},0,0)&&host.view()==EditorFrontendView::Main,"acknowledge prompt");pads[0]=pad(0x1000);check(host.update(pads,{},0,0)&&host.view()==EditorFrontendView::Main,"prompt A→Start alias also consumed");pads={};check(host.update(pads,{},0,0),"release prompt confirm");pads[0]=pad(0x100);check(host.update(pads,{},0,0)&&host.view()==EditorFrontendView::Unavailable,"prompt destination can be entered again with fresh confirm");}
    std::printf("MAIN return confirm quarantine: %u checks PASS; all ports/A-Start alias/release\n",checks);return 0;
}
