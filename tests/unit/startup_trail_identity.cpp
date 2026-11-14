// New regression for source1AFE/FF payload identity, NOT lookup1AFC/FD.
#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/startup_trail_scene.hpp"
namespace {unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"trail identity: %s\n",why);std::abort();}}}
int main(){
    using namespace gf;StartupTrailState s;s.motion_bank=0;s.motion_index=7;s.captured94=42;StartupTrailParentFields f;f.declared=true;f.identity94=f.identity3e8=42;CharacterProgramLane lane;lane.descriptor_group=5;lane.descriptor_choice=7;lane.metadata[0]=0;lane.metadata[1]=7;
    for(unsigned i=0;i<256;++i){lane.descriptor_group=u8(i);lane.descriptor_choice=u8(255-i);check(startup_trail_parent_matches(s,f,lane),"lookup descriptor alias does not retire same source motion payload");}
    for(unsigned i=0;i<256;++i){lane.metadata[1]=u8(i);check(startup_trail_parent_matches(s,f,lane)==(i==7),"raw motion index identity controls lifetime");}lane.metadata[1]=7;
    for(unsigned i=0;i<256;++i){f.field_af=std::bit_cast<s8>(u8(i));check(startup_trail_parent_matches(s,f,lane)==(i!=255&&i!=126),"all source AF bytes");}f.field_af=0;
    for(unsigned i=0;i<256;++i){f.phase18=std::bit_cast<s8>(u8(i));check(startup_trail_parent_matches(s,f,lane)==(f.phase18<2),"signed parent phase predicate");}f.phase18=0;
    f.selector3ec=2;check(!startup_trail_parent_matches(s,f,lane),"unported parent style cannot inherit the default atlas/colour");f.selector3ec=0;f.identity3e8=43;check(!startup_trail_parent_matches(s,f,lane),"source identity mismatch");f.identity3e8=42;f.declared=false;check(!startup_trail_parent_matches(s,f,lane),"no invented lifecycle fields");
    // Independent dense row-elimination oracle (not the source Thomas solver).
    for(unsigned n=2;n<=16;++n){std::vector<HistoryPoint> p(n);for(unsigned i=0;i<n;++i)for(unsigned k=0;k<3;++k)p[i][k]=std::sin(f32(i*3+k))*(f32(i)+2);std::vector<HistoryCubic> c;check(history_natural_cubic(p,&c),"bounded nonlinear history");
        for(unsigned k=0;k<3;++k){double a[16][17]={};for(unsigned i=0;i<n;++i){a[i][i]=(i==0||i+1==n)?2:4;if(i)a[i][i-1]=1;if(i+1<n)a[i][i+1]=1;a[i][n]=3*(i==0?double(p[1][k])-p[0][k]:i+1==n?double(p[n-1][k])-p[n-2][k]:double(p[i+1][k])-p[i-1][k]);}for(unsigned col=0;col<n;++col){const double pivot=a[col][col];for(unsigned j=col;j<=n;++j)a[col][j]/=pivot;for(unsigned i=0;i<n;++i)if(i!=col){const double factor=a[i][col];for(unsigned j=col;j<=n;++j)a[i][j]-=factor*a[col][j];}}
            for(unsigned i=0;i+1<n;++i)for(double u:{0.,.125,.5,.875,1.}){const double d0=a[i][n],d1=a[i+1][n],delta=double(p[i+1][k])-p[i][k],expected=(((-2*delta+d0+d1)*u+(3*delta-2*d0-d1))*u+d0)*u+p[i][k];const auto got=history_cubic_sample(c[i],f32(u));check(std::abs(double(got[k])-expected)<.0001+std::abs(expected)*.00001,"native cubic agrees with independent dense derivative solve");}}
    }
    std::printf("{\"trail_identity_and_independent_cubic_checks\":%u,\"all_passed\":true}\n",checks);
}
