#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include "tests/unit/adx_fixture.hpp"
#include "engine/assets/afs.hpp"
namespace {unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"adx: %s\n",why);std::abort();}}}
int main(){
    auto b=owned_adx::file();gf::AdxAudio audio;std::string error;
    auto decode=[&](const auto& bytes){return gf::adx_decode({bytes.data(),bytes.size()},&audio,&error);};
    check(decode(b)&&audio.frames==4&&audio.rate==8000&&audio.channels==2,"owned v3 header");
    check(audio.samples==std::vector<gf::s16>({1,1,4,4,6,6,6,6}),"hand-calculated second-order predictor, residual signs, channel interleave");
    auto mono=owned_adx::file(4,1);check(decode(mono)&&audio.samples==std::vector<gf::s16>({1,4,6,6}),"mono shape");
    auto independent=b;independent[84]=0xfe;independent[85]=0x12;
    check(decode(independent)&&audio.samples==std::vector<gf::s16>({1,-1,4,-4,6,-6,6,-6}),"channel histories remain independent");
    auto clip=b;clip[64]=0x7f;clip[65]=0xff;clip[66]=0x78;clip[67]=0x78;
    check(decode(clip)&&audio.samples[0]==32767&&audio.samples[2]==-32768&&audio.samples[4]==32767,"prediction uses clipped histories without overflow");
    auto zero=b;zero[64]=zero[65]=0;check(decode(zero)&&audio.samples[0]==0&&audio.samples[2]==0,"stored scale zero is zero, not an invented plus-one scale");
    auto loop=owned_adx::file(64,2,true);check(decode(loop)&&audio.loop&&audio.loop_start==0&&audio.loop_end==64,"loop sample/byte metadata");
    for(unsigned kind=0;kind<16;++kind){auto bad=b;
        switch(kind){
        case 0:bad[0]=0;break;case 1:bad[4]=2;break;case 2:bad[5]=17;break;case 3:bad[6]=8;break;
        case 4:bad[7]=3;break;case 5:bad[18]=4;break;case 6:bad[19]=8;break;case 7:gf::store_be32(bad.data()+8,0);break;
        case 8:gf::store_be32(bad.data()+12,0);break;case 9:gf::store_be32(bad.data()+12,gf::kMaxAdxFrames+1);break;
        case 10:bad[58]=0;break;case 11:bad.pop_back();break;case 12:bad[64]=0x80;break;case 13:bad[82]=0x80;break;
        case 14:gf::store_be32(bad.data()+24,2);break;case 15:gf::store_be32(bad.data()+16,0xffff0300);break;
        }
        check(!decode(bad)&&audio.samples.empty()&&audio.frames==0,"malformed/unsupported data refuses without partial PCM");
    }
    for(unsigned at:{28u,32u,36u,40u}){auto bad=loop;bad[at]^=1;check(!decode(bad)&&audio.samples.empty(),"incoherent loop samples/offsets refuse");}
    check(!gf::adx_decode({b.data(),20},&audio,&error)&&!gf::adx_decode({b.data(),b.size()},nullptr,&error),"short header/missing output");
    check(gf::adx_detail::nearest_even(2.5f)==2&&gf::adx_detail::nearest_even(3.5f)==4&&gf::adx_detail::nearest_even(-2.5f)==-2,"predictor coefficient tie-to-even is explicit");
    bool real=false,equal=false;std::ifstream file("original/extracted/files/poq_adx_usa.afs",std::ios::binary);
    if(file){
        gf::u8 entry[8];file.seekg(8+32*8);file.read(reinterpret_cast<char*>(entry),8);check(bool(file),"real archive directory entry");
        const auto offset=gf::load_le32(entry),size=gf::load_le32(entry+4);check(size<32u<<20,"real member budget");
        std::vector<gf::u8> bytes(size);file.seekg(offset);file.read(reinterpret_cast<char*>(bytes.data()),size);check(bool(file)&&decode(bytes),"real source-qualified member32 decodes");
        check(audio.rate==44100&&audio.channels==2&&audio.frames==774151&&audio.loop_start==59968&&audio.loop_end==765594,"real stream metadata");
        std::ifstream ref("local/recomp-evidence/native-menu-audio-20261108/ffmpeg.pcm",std::ios::binary);
        if(ref){std::vector<gf::u8> r((std::istreambuf_iterator<char>(ref)),{});check(r.size()>=audio.samples.size()*2,"reference contains declared sample count");
            for(std::size_t i=0;i<audio.samples.size();++i){const gf::u16 value=gf::u16(audio.samples[i]);if(r[i*2]!=gf::u8(value)||r[i*2+1]!=gf::u8(value>>8))check(false,"independent reference PCM mismatch");}
            check(true,"all declared real PCM samples match FFmpeg exactly; extra final block padding excluded");equal=true;}
        real=true;
    }
    std::printf("{\"adx_checks\":%u,\"real_present\":%s,\"reference_pcm_equal\":%s,\"all_passed\":true}\n",checks,real?"true":"false",equal?"true":"false");
}
