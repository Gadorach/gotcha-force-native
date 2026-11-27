#include <cassert>
#include "engine/gameplay/borg_lab_picker.hpp"

int main(){
    using gf::BorgLabPicker;
    using C=BorgLabPicker::Command;
    BorgLabPicker empty({});
    assert(!empty.navigate(C::Confirm)&&!empty.open());
    BorgLabPicker picker({{"a","",""},{"b","",""},{"c","",""},{"d","",""},{"e","",""}},2);
    assert(!picker.navigate(C::Confirm)&&picker.open()&&picker.focused_row()==0);
    picker.navigate(C::Up);
    assert(picker.focused_row()==0);
    picker.navigate(C::Down);
    picker.navigate(C::NextPage);
    assert(picker.focused_row()==3);
    picker.navigate(C::NextPage);
    picker.navigate(C::NextPage);
    picker.navigate(C::Down);
    assert(picker.focused_row()==4);
    picker.fit_height(140);
    assert(picker.rows_per_page()==3&&picker.focused_row()==4);
    assert(picker.navigate(C::Confirm)==4&&!picker.open()&&picker.focused_row()==0);
    picker.activate(4);
    picker.navigate(C::PreviousPage);
    assert(picker.open()&&picker.focused_row()==1);
    picker.navigate(C::Cancel);
    assert(!picker.open()&&picker.focused_row()==4);

    gf::PointerState click;
    click.within_window=click.left_pressed=true;
    click.x=BorgLabPicker::left+1;click.y=BorgLabPicker::top+1;
    assert(!picker.step(click,480)&&picker.open());
    click.y+=3*BorgLabPicker::row_height; // Prev, after two visible rows
    assert(!picker.step(click,480)&&picker.open()&&picker.focused_row()==0);
    picker.navigate(C::Down);
    assert(picker.focused_row()==1&&picker.navigate(C::Confirm)==1);
}
