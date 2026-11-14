#include "engine/animation/hierarchy.h"
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <limits>

#define CHECK(x) do { if(!(x)) { std::fprintf(stderr,"hierarchy property failure line %u\n",__LINE__);std::abort(); } } while(false)
static int read_node(void* user,uint32_t id,gf_hierarchy_node* out) {
  auto& nodes=*static_cast<std::vector<gf_hierarchy_node>*>(user);
  if(!id || id>=nodes.size()) return 0;
  *out=nodes[id];return 1;
}
static void reference(std::vector<gf_hierarchy_node>& n,unsigned id,const gf_hierarchy_policy& p,std::vector<unsigned>& order) {
  n[id].flags|=p.dirty;order.push_back(id);
  if(n[id].flags&p.stop_children) return;
  for(unsigned child=n[id].child;child;child=n[child].sibling)
    if(!(n[child].flags&p.skip_child) && !gf_hierarchy_dirty(n[child].flags,&p)) reference(n,child,p,order);
}
static uint32_t seed=0x28371;
static unsigned random32() { seed=seed*1664525u+1013904223u;return seed; }
int main() {
  gf_hierarchy_policy p{1,2,4,8,6,1,1,1,1,1,1,1,1,1,1};
  gf_hierarchy_workspace workspace{};
  for(unsigned trial=0;trial<1000;++trial) {
    unsigned count=trial==0 ? 96 : 1+random32()%64;
    std::vector<gf_hierarchy_node> nodes(count+1);
    for(unsigned i=1;i<=count;++i) {
      nodes[i]={i,trial==0 ? 0u : random32()%16,0,0};
      if(i>1) {
        unsigned parent=trial==0 ? i-1 : 1+random32()%(i-1);
        nodes[i].sibling=nodes[parent].child;nodes[parent].child=i;
      }
    }
    auto original=nodes,expected=nodes;
    std::vector<unsigned> order;
    reference(expected,1,p,order);
    CHECK(gf_hierarchy_plan(1,&p,read_node,&nodes,&workspace));
    CHECK(workspace.count==order.size());
    for(unsigned i=0;i<workspace.count;++i) {
      CHECK(workspace.writes[i].key==order[i]);
      nodes[workspace.writes[i].key].flags=workspace.writes[i].flags;
    }
    for(unsigned i=1;i<=count;++i) CHECK(nodes[i].flags==expected[i].flags);
    nodes=original;
    workspace.epoch=std::numeric_limits<uint32_t>::max();
    CHECK(gf_hierarchy_plan(1,&p,read_node,&nodes,&workspace));
  }
  std::vector<gf_hierarchy_node> bad{{},{1,0,2,0},{2,0,1,0}};
  CHECK(!gf_hierarchy_plan(1,&p,read_node,&bad,&workspace));
  CHECK(bad[1].flags==0 && bad[2].flags==0);
  bad[2].child=0;bad[2].sibling=2;
  CHECK(!gf_hierarchy_plan(1,&p,read_node,&bad,&workspace));
  bad[2].sibling=999;
  CHECK(!gf_hierarchy_plan(1,&p,read_node,&bad,&workspace));
  bad.resize(GF_HIERARCHY_CAPACITY+2);
  for(unsigned i=1;i<bad.size();++i) bad[i]={i,0,i+1<bad.size() ? i+1 : 0,0};
  CHECK(!gf_hierarchy_plan(1,&p,read_node,&bad,&workspace));
  for(const auto& node:bad) CHECK(node.flags==0);
  std::puts("{\"hierarchy_property_cases\":1000,\"deep_tree\":true,\"epoch_wrap\":true,\"cycle_bounds_capacity_controls\":4,\"all_passed\":true}");
}
