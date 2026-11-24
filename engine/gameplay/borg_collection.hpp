#pragma once
#include "engine/gameplay/borg_library.hpp"

namespace gf {
enum class BorgCollectionSort {Identity,Name};
enum class BorgCollectionFilter {All,Exact};

// The cursor is presentation state only. Identity is retained separately
// from the filtered order and never inferred from a position in that order.
class BorgCollection {
public:
    explicit BorgCollection(BorgLibrary library):library_(std::move(library)){rebuild();}
    bool refresh(BorgLibrary library,const PackageBorgRoster* roster=nullptr){
        std::optional<std::vector<BorgLibraryOwnedRow>> owned;
        if(roster){
            owned=library.project_owned(*roster);
            if(!owned)return false;
        }
        auto current=selected_key();
        if(is_owned()!=owned.has_value()){preferred_.reset();current.reset();}
        library_=std::move(library);owned_=std::move(owned);rebuild(current);return true;
    }
    bool is_owned()const{return owned_.has_value();}
    const BorgLibrary& library()const{return library_;}
    std::size_t size()const{return order_.size();}
    std::size_t selected()const{return selected_;}
    BorgCollectionSort sort()const{return sort_;}
    BorgCollectionFilter filter()const{return filter_;}
    const BorgLibraryRow* installed_row(std::size_t index)const{
        if(index>=size())return nullptr;
        if(const auto* owned=owned_row(index))return owned->installed?&*owned->installed:nullptr;
        return &library_.rows()[order_[index]];
    }
    const BorgLibraryOwnedRow* owned_row(std::size_t index)const{
        return owned_&&index<size()?&(*owned_)[order_[index]]:nullptr;
    }
    void configure(BorgCollectionSort sort,BorgCollectionFilter filter){
        const auto current=selected_key();
        sort_=sort;filter_=filter;rebuild(current);
    }
    bool select(std::size_t index){
        if(index>=size())return false;
        selected_=index;preferred_=key(order_[index]);return true;
    }
    bool select_instance(u64 instance){
        if(!owned_||!instance)return false;
        for(std::size_t i=0;i<size();++i)if(owned_row(i)->saved.instance==instance)return select(i);
        return false;
    }
    bool move(bool back,std::size_t count=1){
        if(size()<2)return false;
        count=std::min(count,size()-1);
        return count&&select(back?(selected_+size()-count)%size():(selected_+count)%size());
    }
    std::pair<std::size_t,std::size_t> visible_range(std::size_t capacity)const{
        capacity=std::min(capacity,size());
        if(!capacity)return {};
        const auto first=std::min(selected_>=capacity?selected_-capacity+1:0,size()-capacity);
        return {first,first+capacity};
    }
private:
    struct Key {
        BorgLibraryKey installed; // Empty variant for a diagnostic-only package.
        u64 instance=0;
    };
    Key key(std::size_t i)const{
        if(owned_)return {{},(*owned_)[i].saved.instance};
        const auto& row=library_.rows()[i];
        return {row.key?*row.key:BorgLibraryKey{row.identity.id,{}},0};
    }
    std::optional<Key> selected_key()const{
        return size()?std::optional<Key>{key(order_[selected_])}:std::nullopt;
    }
    bool matches(std::size_t i,const Key& candidate)const{
        if(owned_)return candidate.instance==(*owned_)[i].saved.instance;
        const auto& row=library_.rows()[i];
        return candidate.installed.package_id==row.identity.id&&
            (row.key?candidate.installed.variant_id==row.key->variant_id:candidate.installed.variant_id.empty());
    }
    bool exact(std::size_t i)const{
        if(!owned_)return library_.rows()[i].key.has_value();
        const auto& row=(*owned_)[i];
        return row.installed.has_value()&&!row.saved.disabled;
    }
    std::string_view name(std::size_t i)const{return owned_?(*owned_)[i].saved.identity.name:library_.rows()[i].identity.name;}
    void rebuild(const std::optional<Key>& current={}){
        order_.clear();selected_=0;
        const auto total=owned_?owned_->size():library_.rows().size();
        for(std::size_t i=0;i<total;++i)if(filter_==BorgCollectionFilter::All||exact(i))order_.push_back(i);
        if(sort_==BorgCollectionSort::Name)
            std::sort(order_.begin(),order_.end(),[&](std::size_t a,std::size_t b){
                return name(a)==name(b)?a<b:name(a)<name(b);
            });
        if(preferred_){
            std::optional<std::size_t> visible_fallback;
            for(std::size_t i=0;i<size();++i){
                if(matches(order_[i],*preferred_)){selected_=i;return;}
                if(current&&matches(order_[i],*current))visible_fallback=i;
            }
            if(visible_fallback)selected_=*visible_fallback;
        }else if(size())preferred_=key(order_[0]);
    }
    BorgLibrary library_;
    std::optional<std::vector<BorgLibraryOwnedRow>> owned_;
    std::vector<std::size_t> order_;
    std::optional<Key> preferred_;
    std::size_t selected_=0;
    BorgCollectionSort sort_=BorgCollectionSort::Identity;
    BorgCollectionFilter filter_=BorgCollectionFilter::All;
};

// A uniform integer scale suits the compatibility bitmap font. The viewport
// is physical pixels; layout coordinates are logical (with a narrow stack).
struct BorgCollectionLayout {
    s32 width=0,height=0,scale=1;
    bool stacked=false,show_details=false,show_footer=false;
    s32 panel_top=0,list_left=0,list_width=0,list_y=0,row_height=24,detail_left=0,detail_y=0,detail_width=0,footer_y=0;
    std::size_t rows=0;

    static BorgCollectionLayout fit(unsigned pixel_width,unsigned pixel_height,unsigned logical_width,unsigned logical_height){
        BorgCollectionLayout layout;
        layout.pixel_width_=pixel_width;layout.pixel_height_=pixel_height;
        layout.logical_width_=logical_width;layout.logical_height_=logical_height;
        layout.scale=s32(std::clamp(std::min(pixel_width/std::max(1u,logical_width),pixel_height/std::max(1u,logical_height)),1u,3u));
        layout.width=s32(pixel_width)/layout.scale;layout.height=s32(pixel_height)/layout.scale;
        const auto w=layout.width,h=layout.height;
        layout.stacked=w<480;
        layout.show_footer=h>=95;
        layout.footer_y=layout.show_footer?h-29:h;
        layout.panel_top=h>=116?57:h>=60?27:2;
        layout.list_y=layout.panel_top+(h>=116?19:0);
        layout.list_left=w>=100?8:2;
        layout.list_width=std::max(0,w-2*layout.list_left);
        layout.detail_left=layout.list_left;
        layout.detail_width=layout.list_width;
        layout.show_details=h>=(layout.stacked?233:195);
        if(!layout.stacked&&layout.show_details){
            layout.list_width=std::clamp(w*46/100,210,540);
            layout.detail_left=layout.list_left+layout.list_width+8;
            layout.detail_width=w-layout.detail_left-layout.list_left;
        }
        const s32 detail_reserve=std::clamp(h/3,98,210);
        const s32 list_bottom=layout.stacked&&layout.show_details?layout.footer_y-detail_reserve:layout.footer_y;
        if(list_bottom>layout.list_y){
            layout.row_height=std::min(layout.row_height,list_bottom-layout.list_y);
            layout.rows=std::size_t((list_bottom-layout.list_y)/layout.row_height);
        }
        layout.detail_y=layout.stacked?list_bottom+5:layout.list_y;
        return layout;
    }
    std::optional<std::size_t> hit_row(s32 window_x,s32 window_y,const BorgCollection& collection)const{
        if(window_x<0||window_y<0||unsigned(window_x)>=logical_width_||unsigned(window_y)>=logical_height_)return {};
        const auto x=s32((u64(window_x)*pixel_width_/logical_width_)/unsigned(scale));
        const auto y=s32((u64(window_y)*pixel_height_/logical_height_)/unsigned(scale));
        if(x<list_left+2||x>=list_left+list_width-2||y<list_y)return {};
        const auto [first,last]=collection.visible_range(rows);
        const auto index=first+std::size_t((y-list_y)/row_height);
        return index<last?std::optional<std::size_t>{index}:std::nullopt;
    }
private:
    unsigned pixel_width_=0,pixel_height_=0,logical_width_=0,logical_height_=0;
};
}
