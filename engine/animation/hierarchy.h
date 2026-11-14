#ifndef GF_ANIMATION_HIERARCHY_H
#define GF_ANIMATION_HIERARCHY_H
#include <stdint.h>
#include <string.h>

// Generic native forest traversal. Layout/flag meanings and compatibility
// scheduler weights belong to the caller's reviewed private binding, not here.
enum { GF_HIERARCHY_CAPACITY=1024, GF_HIERARCHY_HASH_SIZE=2048 };
struct gf_hierarchy_node { uint32_t key, flags, child, sibling; };
typedef int (*gf_hierarchy_read)(void*,uint32_t,struct gf_hierarchy_node*);
struct gf_hierarchy_policy {
  uint32_t dirty, independent, stop_children, skip_child;
  unsigned inline_depth;
  unsigned root_cost, list_cost, visit_cost, next_cost, inline_check_cost;
  unsigned normal_check_cost, true_cost, mark_cost, delegated_check_cost, delegated_call_cost;
};
struct gf_hierarchy_write { uint32_t key, flags; };
struct gf_hierarchy_frame { uint32_t next; unsigned phase; };
struct gf_hierarchy_workspace {
  struct gf_hierarchy_write writes[GF_HIERARCHY_CAPACITY+1];
  struct gf_hierarchy_frame stack[GF_HIERARCHY_CAPACITY];
  uint32_t keys[GF_HIERARCHY_HASH_SIZE], stamps[GF_HIERARCHY_HASH_SIZE], epoch;
  unsigned count, visited;
  uint64_t charge;
};

static inline int gf_hierarchy_dirty(uint32_t flags,const struct gf_hierarchy_policy* p) {
  return !(flags&p->independent) && (flags&p->dirty)!=0;
}

static inline int gf_hierarchy_seen(struct gf_hierarchy_workspace* w,uint32_t key) {
  unsigned slot=((key>>2)*2654435761u)&(GF_HIERARCHY_HASH_SIZE-1u);
  for(unsigned i=0;i<GF_HIERARCHY_HASH_SIZE;++i) {
    if(w->stamps[slot]!=w->epoch) { w->keys[slot]=key;w->stamps[slot]=w->epoch;return 0; }
    if(w->keys[slot]==key) return 1;
    slot=(slot+1u)&(GF_HIERARCHY_HASH_SIZE-1u);
  }
  return 1;
}

// Workspace must be zero-initialized before first use. Build a preorder write
// plan without mutating scene state. Reject cycles,
// shared nodes, invalid reads and capacity overruns before the first write.
static inline int gf_hierarchy_plan(uint32_t root,const struct gf_hierarchy_policy* p,
                                    gf_hierarchy_read read,void* user,
                                    struct gf_hierarchy_workspace* w) {
  w->count=w->visited=0;w->charge=0;
  if(++w->epoch==0) { memset(w->stamps,0,sizeof(w->stamps));w->epoch=1; }
  if(!root || !p->inline_depth || p->inline_depth>32) return 0;
  struct gf_hierarchy_node node;
  if(!read(user,root,&node) || !node.key || gf_hierarchy_seen(w,node.key)) return 0;
  ++w->visited;
  w->writes[w->count].key=node.key;
  w->writes[w->count++].flags=node.flags|p->dirty;
  w->charge=p->root_cost;
  unsigned depth=0;
  if(!(node.flags&p->stop_children)) {
    w->charge+=p->list_cost;
    w->stack[depth].next=node.child;w->stack[depth++].phase=1;
  }
  while(depth) {
    struct gf_hierarchy_frame* frame=w->stack+depth-1;
    if(!frame->next) { --depth;continue; }
    if(w->visited>=GF_HIERARCHY_CAPACITY || !read(user,frame->next,&node) ||
       !node.key || gf_hierarchy_seen(w,node.key)) return 0;
    ++w->visited;
    unsigned phase=frame->phase;
    frame->next=node.sibling;
    w->charge+=p->visit_cost+p->next_cost;
    if(node.flags&p->skip_child) continue;
    int blocked=gf_hierarchy_dirty(node.flags,p);
    w->charge+=phase<=p->inline_depth ? p->inline_check_cost : p->delegated_check_cost;
    if(!(node.flags&p->independent)) {
      w->charge+=p->normal_check_cost;
      if(blocked) w->charge+=p->true_cost;
    }
    if(blocked) continue;
    if(w->count>=GF_HIERARCHY_CAPACITY) return 0;
    w->writes[w->count].key=node.key;
    w->writes[w->count++].flags=node.flags|p->dirty;
    if(phase>p->inline_depth) {
      w->charge+=p->delegated_call_cost+p->root_cost;
      phase=0;
    } else w->charge+=p->mark_cost;
    if(!(node.flags&p->stop_children)) {
      if(depth>=GF_HIERARCHY_CAPACITY) return 0;
      w->charge+=p->list_cost;
      w->stack[depth].next=node.child;w->stack[depth++].phase=phase+1;
    }
  }
  return 1;
}
#endif
