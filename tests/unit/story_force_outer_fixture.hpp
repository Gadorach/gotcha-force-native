#pragma once
#include <fstream>
#include <iterator>
#include <map>
#include <stdexcept>
#include "engine/gameplay/frame_digest.hpp"
#include "engine/gameplay/story_force_outer_controller.hpp"
namespace story_force_outer_fixture {
using namespace gf;
struct Files final:IFileSystem {
    std::map<std::string,std::vector<u8>> data;unsigned ranges=0,whole=0,writes=0;
    bool read(const std::string&,std::vector<u8>&)override{++whole;return false;}
    bool file_size(const std::string& p,u64* out)override{if(!out||!data.contains(p))return false;*out=data.at(p).size();return true;}
    bool read_range(const std::string& p,u64 at,std::size_t n,std::vector<u8>& out)override{++ranges;out.clear();if(!data.contains(p)||at||n!=data.at(p).size())return false;out=data.at(p);return true;}
    bool write(const std::string&,const u8*,std::size_t)override{++writes;return false;}bool exists(const std::string& p)override{return data.contains(p);}bool erase(const std::string&)override{++writes;return false;}
};
struct Save final:ISaveBackend {unsigned calls=0;bool load(u32,std::vector<u8>&)override{++calls;return false;}bool store(u32,const u8*,std::size_t)override{++calls;return false;}bool erase(u32)override{++calls;return false;}};
inline ForceSupplement names(const ForceCatalog& catalog){ForceSupplement n;std::array<std::string,10> digits;for(unsigned i=0;i<10;++i)digits[i]=std::to_string(i);std::string error;if(!n.assign(catalog,{},"PART ",{"N","A","G","S","C","B"},digits,{"A"},&error))throw std::runtime_error(error);return n;}
inline StoryForceSetupFacts facts(){StoryForceSetupFacts f;f.active_event_row=2;f.source50=1;f.source78=500;if(!StoryReplayContext::from_declared_index(5,&f.replay))throw std::runtime_error("replay");return f;}
inline std::vector<u8> read(const char* p){std::ifstream file(p,std::ios::binary);if(!file)throw std::runtime_error("native fixture file");return {std::istreambuf_iterator<char>(file),{}};}
inline u32 hash(const std::vector<u8>& b){u32 h=2166136261;for(u8 x:b)h=(h^x)*16777619u;return h;}
inline void field_words(const StoryForceOuterFields& f,std::vector<u32>* out){*out={f.major,f.minor,f.phase2,f.entry_flag3,f.return_major6,f.last_map_minor7,f.port3c,f.reason44,f.force4c,f.source54,f.completed68};}
}
