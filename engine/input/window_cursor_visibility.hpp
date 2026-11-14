#pragma once
namespace gf {
//Small native window-ownership policy, independent of SDL. Read returns -1 for
//unknown,0 hidden,1 visible; Set performs a checked, process-local cursor change.
//The platform adapter must establish that this window owns pointer+keyboard focus.
class WindowCursorVisibility {
public:
    template<class Read,class Set>bool update(bool owned_focus,Read read,Set set){
        if(!owned_focus)return restore(read,set);
        const int current=read();if(current<0||current>1)return false;
        if(!captured_){previous_=current;captured_=true;}
        return current==0||set(false);
    }
    template<class Read,class Set>bool restore(Read read,Set set){
        if(!captured_)return true;
        const int current=read();if(current<0||current>1)return false;
        //Do not overwrite another component's subsequent visible-cursor choice.
        if(current==0&&previous_==1&&!set(true))return false;
        captured_=false;return true;
    }
    bool owns_request()const{return captured_;}
private:
    int previous_=1;bool captured_=false;
};
} //namespace gf
