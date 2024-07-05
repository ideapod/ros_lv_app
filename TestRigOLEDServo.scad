include  <./Hobby-Servo-Brackets/mini/servo_bracket_modules.scad>
include <ssd1603-128x64-oled.scad>

bracket_width = 30;
bracket_height = 23;
bracket_depth = 10;
pcb_width = 24.7;
pcb_height = 27;
frame_thickness = 3;

translate ([0, bracket_width,bracket_height]) rotate( [ -90, 0,0]) servo_bracket();
translate ( 
    [ 0, 0, 2 ] )
    oled_bottom_frame();

/*
translate (
    [ 0, -30, 0 ]) oled_cover_frame();
    */