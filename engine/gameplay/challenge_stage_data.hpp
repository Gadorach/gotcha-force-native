#pragma once
#include <memory>
#include <exception>
#include "engine/assets/challenge_finalization.hpp"
#include "engine/core/backends.hpp"
namespace gf {
class ChallengeFinalSetup;class ChallengeStageDataDriver;
namespace challenge_stage_data_detail {struct Request {ChallengeStagePayloadRow row;bool live=true,claimed=false;};}
class ChallengeStageDataRequest {
public:
    bool valid()const{const auto s=scope_.lock();return s&&s->live;}
private:
    friend class ChallengeFinalSetup;friend class ChallengeStageDataDriver;
    std::weak_ptr<challenge_stage_data_detail::Request> scope_;
};
class ChallengeNativeStageData {
public:
    const ChallengeStagePayloadRow& selection()const{return row_;}
    ByteView slot(unsigned bundle,unsigned index)const{if(bundle>=2||bytes_[bundle].empty())return {};const ByteView b{bytes_[bundle].data(),bytes_[bundle].size()};if(index>=b.be32(8))return {};std::size_t at=16+8*b.be32(8);for(unsigned i=0;i<index;++i)at+=b.be32(20+8*i);return {b.data+at,b.be32(20+8*index)};}
    unsigned slots(unsigned bundle)const{return bundle<2&&!bytes_[bundle].empty()?ByteView{bytes_[bundle].data(),bytes_[bundle].size()}.be32(8):0;}
    std::size_t bytes()const{return bytes_[0].size()+bytes_[1].size();}
private:
    friend class ChallengeStageDataDriver;
    ChallengeStagePayloadRow row_;std::array<std::vector<u8>,2> bytes_;
};
class ChallengeStageDataReceipt {
public:
    bool valid()const{const auto s=request_.lock();return !failed_&&s&&s->live&&s->claimed&&!data_.expired();}
    bool failed()const{const auto s=request_.lock();return failed_&&s&&s->live&&s->claimed;}
private:
    friend class ChallengeFinalSetup;friend class ChallengeStageDataDriver;
    std::weak_ptr<challenge_stage_data_detail::Request> request_;std::weak_ptr<const ChallengeNativeStageData> data_;bool failed_=false;
};
//One claimed native read of each selected exported bundle. No write, retry,
//source PZZ/AFS/DOL interpretation, GPU upload or original async/DMA ACK.
class ChallengeStageDataDriver {
public:
    bool begin(const ChallengeStageDataRequest& request,IFileSystem& files,const std::string& directory,std::string* error){
        auto reject=[&](const char* why){if(error)*error=std::string("Challenge stage data: ")+why;return false;};
        const auto s=request.scope_.lock();if(claimed_||!s||!s->live||s->claimed||directory.empty())return reject("fresh live unclaimed request/native directory required");
        s->claimed=true;claimed_=true;request_=s;auto data=std::make_shared<ChallengeNativeStageData>();data->row_=s->row;
        auto fail=[&](const char* why){failed_=true;return reject(why);};
        try{for(unsigned i=0;i<2;++i){const auto& spec=s->row.payloads[i];if(spec.id==0xffffffffu)continue;const auto path=directory+"/"+std::to_string(spec.id)+".gfnb";u64 size=0;if(!files.file_size(path,&size)||size!=spec.size||!files.read(path,data->bytes_[i])||data->bytes_[i].size()!=spec.size)return fail("missing/short native payload; request not replayed");const ByteView b{data->bytes_[i].data(),data->bytes_[i].size()};if(challenge_payload_hash(b)!=spec.hash||!challenge_native_bundle_valid(b))return fail("native payload identity/extent; request not replayed");}}
        catch(const std::exception&){return fail("uncertain native read; request not replayed");}
        if(!s->live)return fail("request retired during read");
        data_=std::move(data);if(error)error->clear();return true;
    }
    ChallengeStageDataReceipt receipt()const{ChallengeStageDataReceipt r;if(claimed_){r.request_=request_;r.data_=data_;r.failed_=failed_;}return r;}
private:
    std::weak_ptr<challenge_stage_data_detail::Request> request_;std::shared_ptr<const ChallengeNativeStageData> data_;bool claimed_=false,failed_=false;
};
} // namespace gf
