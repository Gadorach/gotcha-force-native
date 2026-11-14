#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include "engine/assets/dsp_adpcm.hpp"
namespace {
unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"DSP ADPCM: %s\n",why);std::abort();}}
std::vector<gf::u8> header(unsigned n){std::vector<gf::u8> b(96);gf::store_be32(b.data(),n);
    const unsigned nibs=(n/14)*16+(n%14?n%14+2:0);gf::store_be32(b.data()+4,nibs);gf::store_be32(b.data()+8,44100);
    gf::store_be32(b.data()+16,2);gf::store_be32(b.data()+20,((n-1)/14)*16+(n-1)%14+2);gf::store_be32(b.data()+24,2);return b;}
std::vector<gf::u8> read(const char* path){std::ifstream f(path,std::ios::binary);return {(std::istreambuf_iterator<char>(f)),{}};}
}
int main(){
    auto h=header(14);std::vector<gf::u8> block={0,0x01,0x23,0x45,0x67,0x89,0xab,0xcd};gf::DspAudio audio;std::string error;
    auto decode=[&](const auto& hd,const auto& pcm){return gf::dsp_adpcm_decode({hd.data(),hd.size()},{pcm.data(),pcm.size()},&audio,&error);};
    check(decode(h,block)&&audio.samples==std::vector<gf::s16>({0,1,2,3,4,5,6,7,-8,-7,-6,-5,-4,-3}),"high-first signed nibbles with zero predictor");
    gf::store_be16(h.data()+28,2048);check(decode(h,block)&&audio.samples==std::vector<gf::s16>({0,1,3,6,10,15,21,28,20,13,7,2,-2,-5}),"coefficient one / clipped sample history recurrence");
    gf::store_be16(h.data()+28,1024);gf::store_be16(h.data()+64,1);block.assign(8,0);
    check(decode(h,block)&&audio.samples==std::vector<gf::s16>(14,1),"half-up predictor rounding, not nearest-even");
    gf::store_be16(h.data()+64,65535);check(decode(h,block)&&audio.samples==std::vector<gf::s16>(14,0),"negative half rounds via explicit floor with bias");
    h=header(28);gf::store_be16(h.data()+28,2048);block.assign(16,0x11);block[0]=block[8]=0;
    check(decode(h,block)&&audio.samples[13]==14&&audio.samples[14]==15&&audio.samples[27]==28,"predictor histories continue across blocks");
    for(unsigned predictor=0;predictor<8;++predictor){h=header(14);block.assign(8,0x11);block[0]=gf::u8(predictor<<4);gf::store_be16(h.data()+62,predictor<<4);
        check(decode(h,block)&&audio.samples==std::vector<gf::s16>(14,1),"all eight coefficient-pair selectors");}
    h=header(14);gf::store_be16(h.data()+62,15);block.assign(8,0x7f);block[0]=15;
    check(decode(h,block)&&audio.samples[0]==32767&&audio.samples[1]==-32768,"large scales saturate both signs without signed-shift/overflow assumptions");
    for(unsigned n:{1u,13u,14u,15u,28u,29u}){
        h=header(n);std::vector<gf::u8> b(((n+13)/14)*8,0x11);for(unsigned at=0;at<b.size();at+=8)b[at]=0;
        check(decode(h,b)&&audio.samples==std::vector<gf::s16>(n,1),"partial/complete/multiple blocks honor declared samples");
        if(n%14==0){gf::store_be32(h.data()+4,(n/14)*16+2);b.resize(b.size()+8,0);
            check(decode(h,b)&&audio.samples.size()==n,"terminal header nibbles are not extra decoded samples");}
    }
    h=header(28);block.assign(16,0);gf::store_be16(h.data()+12,1);gf::store_be32(h.data()+16,18);
    gf::store_be16(h.data()+70,123);gf::store_be16(h.data()+72,65534);
    check(decode(h,block)&&audio.info.loop_start==14&&audio.info.loop_end==28&&audio.info.loop_h1==123&&audio.info.loop_h2==-2,"compressed-loop context retained, not substituted by prefix state");
    auto bad=h;gf::store_be16(bad.data()+68,1);check(!decode(bad,block)&&audio.samples.empty(),"mismatched loop predictor refuses atomically");
    h=header(28);block[8]=0x80;check(!decode(h,block)&&audio.samples.empty()&&audio.info.frames==0,"invalid later predictor publishes no prefix");
    block[8]=0;block[0]=1;check(!decode(h,block),"initial predictor mismatch");block[0]=0;
    for(unsigned at:{0u,4u,8u,12u,14u,16u,20u,24u,60u,62u}){auto bytes=h;bytes[at]^=0x80;check(!decode(bytes,block)&&audio.samples.empty(),"invalid header metadata/format/range/context refuses");}
    auto short_header=h;short_header.pop_back();check(!decode(short_header,block),"short header refuses");
    auto short_block=block;short_block.pop_back();check(!decode(h,short_block),"incomplete last block refuses");
    auto extra=block;extra.push_back(0);check(!decode(h,extra),"unexpected unbounded payload refuses");
    check(!gf::dsp_adpcm_decode({h.data(),h.size()},{nullptr,16},&audio,&error)&&!gf::dsp_adpcm_decode({h.data(),h.size()},{block.data(),block.size()},nullptr,&error),"null inputs/destination refuse");
    const auto chd=read("local/recomp-evidence/native-menu-sfx-20261109/snd_com01.chd"),dpk=read("local/recomp-evidence/native-menu-sfx-20261109/snd_com01.dpk");unsigned real=0,compared=0;
    if(!chd.empty()&&!dpk.empty()){
        gf::ByteView bank{chd.data(),chd.size()};const auto at=bank.be32(28),count=bank.be32(at+8)+1;
        for(unsigned i=0;i<count;++i){const auto pos=at+16+i*100;gf::DspAdpcmInfo info;
            check(gf::dsp_adpcm_header({chd.data()+pos+4,96},&info,&error),"actual common-bank DSP header");
            const auto offset=bank.be32(pos);const auto bytes=gf::dsp_detail::payload_bytes(info.nibbles);
            check(gf::dsp_adpcm_decode({chd.data()+pos+4,96},{dpk.data()+offset,std::size_t(bytes)},&audio,&error)&&audio.samples.size()==info.frames,"actual common-bank DSP declared pass");++real;
            const auto path="local/recomp-evidence/native-menu-sfx-20261109/vgmstream-oracle/sample-"+std::to_string(i)+".pcm";auto ref=read(path.c_str());
            if(!ref.empty()){
                check(ref.size()>=audio.samples.size()*2,"independent declared sample extent");
                for(unsigned j=0;j<audio.samples.size();++j){const auto s=gf::u16(audio.samples[j]);if(ref[j*2]!=gf::u8(s)||ref[j*2+1]!=gf::u8(s>>8)){
                    std::fprintf(stderr,"DSP sample=%u frame=%u native=%d reference=%d\n",i,j,int(audio.samples[j]),int(gf::dsp_detail::signed16(gf::u16(ref[j*2])|gf::u16(ref[j*2+1])<<8)));
                    check(false,"pinned vgmstream DSP sample mismatch");}}
                ++compared;
            }
        }
    }
    std::printf("{\"dsp_adpcm_checks\":%u,\"real_decoded_records\":%u,\"independent_compared_records\":%u,\"all_passed\":true}\n",checks,real,compared);
}
