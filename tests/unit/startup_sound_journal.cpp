#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/startup_sound_journal.hpp"
namespace {
unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"startup sound journal: %s\n",why);std::abort();}}
}
int main(){
 using namespace gf;std::string error;auto scope=std::make_shared<const StartupSoundCaptureScope>();StartupSoundJournal journal;StartupSoundCaptureFrame frame;StartupSoundCaptureContext context;frame.scope=context.scope=scope;frame.frame=context.frame=1;context.commands={true,true,0,3};
 StartupSoundIntent cue;cue.origin=StartupSoundIntentOrigin::Director;cue.opcode=21;cue.operand=380;cue.call={StartupSoundCallKind::StartDirect,380,0,0};frame.intents.push_back(cue);frame.ui.emit(UiSoundOperationKind::MainNavigate,2);
 check(!journal.view().live()&&!journal.view().find(1)&&!journal.frames(),"empty journal has no fabricated frame/scope");
 check(journal.append(frame,context,&error)&&journal.frames()==1,"first accepted frame copied with settings");const auto first=journal.view();check(first.live()&&first.frames()==1,"published prefix has independent bound view");auto data=first.find(1);check(data&&data->capture.intents.size()==1&&data->context.commands.sfx_index==3,"captured cue/settings available as immutable DATA");
 frame.intents[0].call.cue=119;frame.ui.operations[0].item=6;context.commands.sfx_index=0;check(data->capture.intents[0].call.cue==380&&data->capture.ui.operations[0].item==2&&data->context.commands.sfx_index==3,"caller mutations cannot rewrite accepted event/settings snapshot");
 check(!journal.append(frame,context,&error)&&journal.frames()==1,"duplicate frame refuses without advancing");frame.frame=context.frame=3;check(!journal.append(frame,context,&error)&&journal.frames()==1,"skipped accepted frame refuses");frame.frame=2;context.frame=3;check(!journal.append(frame,context,&error),"mixed frame settings refuses");context.frame=2;
 auto neighbor=std::make_shared<const StartupSoundCaptureScope>();frame.scope=context.scope=neighbor;check(!journal.append(frame,context,&error),"foreign source scope cannot extend prefix");frame.scope=scope;check(!journal.append(frame,context,&error),"capture/context source mismatch refuses");context.scope=scope;
 frame.intents.clear();frame.ui={};check(journal.append(frame,context,&error),"empty accepted game frames retain order, no invented sound");const auto second=journal.view();check(second.frames()==2&&second.find(1)&&second.find(2)&&!first.find(2),"old published prefix is stable after new append");check(!second.find(0)&&!second.find(3),"lookup bounds do not wrap or consume");
 frame.frame=context.frame=3;frame.intents.resize(49);check(!journal.append(frame,context,&error)&&journal.frames()==2,"intent overflow refuses atomically");frame.intents.clear();frame.ui.overflow=true;check(!journal.append(frame,context,&error),"UI overflow refuses");frame.ui={};frame.ui.count=3;check(!journal.append(frame,context,&error),"UI count budget refuses");frame.ui={};
 StartupSoundJournal moved=std::move(journal);check(second.live()&&moved.frames()==2&&second.find(2),"moving owner preserves logical journal and immutable views");
 for(unsigned i=3;i<=8192;++i){frame.frame=context.frame=i;check(moved.append(frame,context,&error),"native scene frame budget prefix");}
 auto full=moved.view();check(full.frames()==8192&&full.find(1)&&full.find(8192),"full bounded prefix is readable without controller work");frame.frame=context.frame=8193;check(!moved.append(frame,context,&error)&&moved.frames()==8192,"same8192 scene guard, no forced rollover");
 scope.reset();check(!full.live()&&!full.find(1),"source failure/reload expiry invalidates lookup");moved.reset();check(!second.live()&&!first.live()&&!moved.frames(),"explicit reset invalidates old views, no replay authority");
 frame.scope=context.scope=neighbor;frame.frame=context.frame=1;check(moved.append(frame,context,&error)&&moved.view().find(1),"explicit fresh owner can begin new source scope");check(!first.find(1)&&data->capture.intents[0].call.cue==380,"retained old DATA stays immutable but old view remains inadmissible");
 std::printf("{\"startup_sound_journal_owned_checks\":%u,\"budget_prefix_checks\":8190,\"all_passed\":true}\n",checks);
}
