/* OLED dimensions. 
 * see https://www.aliexpress.com/item/1005006969695993.html?spm=a2g0o.productlist.main.19.78144067Mgld7O&algo_pvid=971f9259-3a85-4a97-a5de-a915efe05848&algo_exp_id=971f9259-3a85-4a97-a5de-a915efe05848-9&pdp_npi=4%40dis%21AUD%2128.02%2117.37%21%21%21133.50%2182.77%21%4021413d5a17177373837866259eb257%2112000038892733587%21sea%21AU%21195567365%21&curPageLogUid=skBgWphW3XTy&utparam-url=scene%3Asearch%7Cquery_from%3A 
 */
 pcb_width = 26.5;
 pcb_height = 27.5;
 hole_diameter = 2.8;
 hole_radius = hole_diameter / 2;
 hole_spacing_width = 21;
 hole_spacing_height = 21;
 panel_width = 24.7;
 panel_height = 13;
 cover_thickness = 2;
 frame_thickness = 3;
 
 /* base plate generates a plate with a width equal to that of the PCB. 
    for the cover there is an extension to the plate on either side to accomodate the edge of the screen
    */
 module base_plate(thickness, extension_width) {
     cube( 
            size=[ pcb_width + extension_width, pcb_height, thickness ] );
 }
 
 /*
    a simple throughhole in the frame
  */
 module through_hole() {
     cylinder( frame_thickness + 1, hole_radius, hole_radius, $fn=20);
 }
 
 /* 
  This is a base plate with four holes and an optional extension to the width of the frame, but maintaining the position of the holes. That is, the extension to the width is added on past the holes, not into the middle 
  */
 module holed_frame(layer_thickness, frame_extension)
 {
     hole_offset_x = (pcb_width - hole_spacing_width) / 2;
     hole_offset_y = (pcb_height - hole_spacing_height) / 2;
     frame_extension_offset = frame_extension / 2;
     
     bottom_left_hole_offset = [
        hole_offset_x + frame_extension_offset,
        hole_offset_y,
        0];
     
     bottom_right_hole_offset = [
        (hole_offset_x + hole_spacing_width + frame_extension_offset),
        hole_offset_y,
        0];
     
     top_left_hole_offset = [
        hole_offset_x + frame_extension_offset,
        hole_offset_y + hole_spacing_height,
        0];
     
     top_right_hole_offset = [
        (hole_offset_x + hole_spacing_width+ frame_extension_offset),
        hole_offset_y + hole_spacing_height,
        0];
        
     // subtract holes from base plate
     difference () {
        base_plate(layer_thickness, frame_extension);
        translate (bottom_left_hole_offset) 
            through_hole();
        translate (bottom_right_hole_offset) 
            through_hole();
        translate (top_left_hole_offset)
            through_hole();
        
        translate (top_right_hole_offset)
            through_hole();
     }
 }
 
 /* 
  the bottom frame (or base) is just plastic with four holes
  */
 module oled_bottom_frame()
 {
     holed_frame(frame_thickness, 0);
 }
 
 /* 
  this is cut out of the screen panel. To be differenced from the panels
  */
 module panel_cutout() {
     cube ( size= [ panel_width, panel_height, frame_thickness ]);
 }
 
 /* the cover should just let the panel be shown, but cover up the other bits
  */
 module oled_cover_frame() {
     // extend it by the thickness width.
     panel_extension = cover_thickness;
     panel_offset_x = (pcb_width - panel_width) /2 + panel_extension / 2;
     panel_offset_y = (pcb_height - panel_height) / 2;
     
     difference () {
        holed_frame(cover_thickness, panel_extension);
        // cut out lcd panel
        translate( [panel_offset_x, panel_offset_y, 0]) 
         panel_cutout();
     }
 }
 
 
 module oled_frames_assembly() {    
    oled_bottom_frame();
    translate ([pcb_width + frame_thickness,0,0]) oled_cover_frame();
     
 }
 
 oled_frames_assembly();
 // ole d_cover_frame();