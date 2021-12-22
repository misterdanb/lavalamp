// facet number
$fn = 200;

// radius of the glass cylinder
glass_cylinder_height = 20;
glass_cylinder_top_height = 2.5;
glass_cylinder_radius = 5 - 0.05;
glass_cylinder_strength = 0.2;

// base outer ring
outer_ring_height = 4.5;
outer_ring_strength = 0.2;
outer_ring_radius = glass_cylinder_radius + outer_ring_strength;

// base inner ring
inner_ring_height = 4;
inner_ring_strength = -0.1;
inner_ring_radius = glass_cylinder_radius - glass_cylinder_strength;

// base cylinder
base_cylinder_height = 0.5;
outer_base_cylinder_height = 3.5;

// cable tunnel
cable_tunnel_radius = 0.35;
cable_tunnel_height = 1;

// button hole
button_hole_radius = 0.8;

// leds cylinder
leds_cylinder_radius = 1.75;
leds_cylinder_height = glass_cylinder_height + outer_base_cylinder_height - glass_cylinder_top_height;
leds_width = 1.1;
leds_gap_depth = 0.25;

// board mount
board_length = 5.7;
board_width = 3.04;
board_height = 1.3;
board_pcb_height = 0.16;
board_mount_radius = 0.15;
board_mount_extent = 0.15    ;
board_mount_height = 0.7;
board_mount_strength = 0.2;
board_mount_hole_distance = board_length - 2 * board_mount_radius - 0.2;

module glass_cylinder_base() {
    difference() {
        union() {
            difference() {
                cylinder(h=outer_ring_height, r=outer_ring_radius);
                translate([0, 0, -1]) cylinder(h=outer_ring_height + 2, r=outer_ring_radius - outer_ring_strength);
            };
            difference() {
                cylinder(h=inner_ring_height, r=inner_ring_radius);
                translate([0, 0, -1]) cylinder(h=inner_ring_height + 2, r=inner_ring_radius - inner_ring_strength);
            };
            cylinder(h=base_cylinder_height, r=outer_ring_radius);
            difference() {
                cylinder(h=outer_base_cylinder_height, r=outer_ring_radius);
                translate([0, 0, -1]) cylinder(h=outer_base_cylinder_height + 2, r=inner_ring_radius);
            };
            translate([-(outer_ring_radius - outer_ring_strength), 0, outer_ring_height / 2]) rotate([0, -90, 0]) cylinder(h=outer_ring_strength, r=button_hole_radius + 0.4);
        }

        translate([inner_ring_radius - inner_ring_strength - 1, 0, 0]) union() {
            translate([0, 0, cable_tunnel_height]) rotate([0, 90, 0]) cylinder(h=outer_ring_radius + 1, r=cable_tunnel_radius);
            translate([0, -cable_tunnel_radius, -1]) cube([outer_ring_radius + 1, 2 * cable_tunnel_radius, cable_tunnel_height + 1]);
        }
        
        translate([-(inner_ring_radius - inner_ring_strength - 1), 0, outer_ring_height / 2]) rotate([0, -90, 0]) cylinder(h=outer_ring_radius - inner_ring_radius + inner_ring_strength + 2, r=button_hole_radius);
        translate([-(inner_ring_radius - inner_ring_strength - 1.2), 0, outer_ring_height / 2]) rotate([0, -90, 0]) cylinder(h=outer_ring_radius - 1.05 * inner_ring_radius + 1, r=button_hole_radius + 0.4);
    }
}

module leds_cylinder() {
    difference() {
        cylinder(h=leds_cylinder_height, r=leds_cylinder_radius);
        for (i = [0 : 7]) {
            rotate([0, 0, i * 360 / 8]) translate([leds_cylinder_radius - leds_gap_depth, -leds_width / 2, -1]) cube([1, leds_width, leds_cylinder_height + 2]);
        };
        translate([0, 0, -1]) cylinder(h=leds_cylinder_height + 2, r=leds_cylinder_radius - 0.5);
        //rotate([0, 0, 4 * 360 / 8]) translate([0, 0, base_cylinder_height]) rotate([0, 25, 0]) translate([0, -2, 0]) cube([4, 4, 4]);
    }
}

module board_mount_element() {
    difference() {
        union() {
            translate([0, -(board_mount_radius + board_mount_extent), 0]) cube([board_mount_height, 2 * (board_mount_radius + board_mount_extent), board_mount_strength]);
            translate([board_mount_height, 0, 0]) cylinder(h=board_mount_strength, r=board_mount_radius + board_mount_extent);
        };
        translate([board_mount_height, 0, -1]) cylinder(h=board_mount_strength + 2, r=board_mount_radius);
    }
}

module board_mount_pair() {
    rotate([0, -90, 0]) union() {
        board_mount_element();
        translate([0, board_mount_hole_distance, 0]) board_mount_element();
    }
}

module board_mount() {
    board_mount_pair();
    translate([board_mount_strength + board_pcb_height, 0, 0]) board_mount_pair();
    translate([-3 * board_mount_strength, -(board_mount_radius + board_mount_extent), 0]) cube([board_mount_strength, 2 * (board_mount_radius + board_mount_extent), board_mount_height - board_mount_radius]);
    translate([-3 * board_mount_strength, board_mount_hole_distance - (board_mount_radius + board_mount_extent), 0]) cube([board_mount_strength, 2 * (board_mount_radius + board_mount_extent), board_mount_height - board_mount_radius]);
}

union() {
    glass_cylinder_base();
    leds_cylinder();
    rotate([0, 0, -4 * 360 / 8]) translate([leds_cylinder_radius + 0.5, -board_mount_hole_distance / 2 - 1.2, base_cylinder_height]) board_mount();
}