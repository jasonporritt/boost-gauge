$fa = 1;
$fs = 0.5;

ring_inner_diameter = 52.324;
ring_outer_diameter = 65.532;
ring_depth = 3;

gauge_flange_diameter = 72;
gauge_body_diameter = 68;

display_board_height = 42;
display_board_width = 43.17;
display_board_depth = 5.42;
display_board_protrusion = 1;
display_board_shelf_width = 2;
display_board_ring_clearance = 2;

clearance = 0.2;
part_depth = 10;

module display_board_pcb(height=1, width=1, depth=1) {
    intersection() {
        translate([width/-2, height/-2, 0])
            cube([width, height, depth]);
        translate([0, 0, 1])
            cylinder(d=ring_inner_diameter - 2*clearance - display_board_ring_clearance, h=depth);
    }
}
union() {
    difference() {
        cylinder(d=gauge_flange_diameter, h=part_depth, center=true);
        union() {
            cylinder(d=ring_outer_diameter + 2*clearance, h=part_depth + 2, center = true);
            translate([0, 0, -0.9*part_depth]) {
                difference() {
                    cylinder(d=ring_outer_diameter + 10, h=part_depth+2);
                    translate([0,0,-1])
                        cylinder(d=gauge_body_diameter, h=part_depth + 4);
                }
            }
        }
    }
    difference() {
        cylinder(d=ring_inner_diameter - 2*clearance, h=part_depth, center=true);
        union() {
            translate([0, 0, part_depth/2 - display_board_depth + display_board_protrusion])
                display_board_pcb(height=display_board_height + 2*clearance, width=display_board_width + 2*clearance, depth=part_depth);
            translate([0, 0, -2*part_depth])
                display_board_pcb(height=display_board_height + 2*clearance, width=display_board_width + 2*clearance - 2*display_board_shelf_width, depth=part_depth*4);
            
        }
    }
}

translate([0, 0, -110]) {
    difference() {
        cylinder(d=gauge_flange_diameter, h=100);
        translate([0, 0, -1]) {
            cylinder(d=gauge_body_diameter, h=102);
        }
    }
}

%translate([53.34/-2, -15, -110]) {
    rotate([90, 0, 0]) {
        cube([53.34, 86.36, 4.76]);
    }
}

module led_segment_separator() {
    // let it a bit inside the main body
    in_by = 0.5; // 1 mm
    // set it up on the rocket's perimeter
    translate([ring_inner_diameter/2 - in_by,0,part_depth/-2 + ring_depth])
        // set some width and center it
        cube([(ring_outer_diameter - ring_inner_diameter)/2 + 2*in_by, 1, part_depth - ring_depth]);
}
many=24;
for (i = [0:many-1])
   rotate([0,0,360/many*i])
       led_segment_separator();