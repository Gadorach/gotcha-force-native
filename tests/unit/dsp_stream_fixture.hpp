#pragma once
#include "engine/audio/dsp_stream.hpp"
namespace dsp_stream_fixture {
inline std::vector<gf::u8> header(unsigned frames){using namespace gf;std::vector<u8> b(96);store_be32(b.data(),frames);store_be32(b.data()+4,(frames/14)*16+(frames%14?frames%14+2:0));store_be32(b.data()+8,32000);store_be32(b.data()+16,2);store_be32(b.data()+20,((frames-1)/14)*16+(frames-1)%14+2);store_be32(b.data()+24,2);return b;}
inline std::vector<gf::u8> payload(unsigned frames){std::vector<gf::u8> b(((frames+13)/14)*8,0x12);for(unsigned i=0;i<b.size();i+=8)b[i]=0;return b;}
} // namespace dsp_stream_fixture
