$fn=100;

mm = 1;
inch = 25.4 * mm;

LENGTH = .75 * inch;
EDGE_WIDTH = .5*inch;
THICKNESS = .2 * inch;
OFFSET = .4 * inch;

HEX_R = 6.1 * mm / 2;
HEX_r = 5.6 * mm / 2;
HEX_THICKNESS = 2 * mm; 

//HEX_THICKNESS = 1.5 * mm; // for thin version
//THICKNESS = 3 * mm; // for thin version

SCREW_R = 1.5 * mm;
module hex(){
  difference(){
    cylinder(r=HEX_R, h=HEX_THICKNESS);
    for(i=[0, 1, 2, 3, 4, 5]){
      rotate(a=60 * i, v=[0, 0, 1])
      translate([HEX_r, -5, 5])rotate(a=90, v=[0, 1, 0])cube([10, 10, HEX_THICKNESS]);
    }
  }
}

TAB_HEIGHT = .5*mm;
TAB_R = 5*mm;
// tab to prevent rollup
module tab(x, y){
  translate([x, y])cylinder(r=TAB_R, h=TAB_HEIGHT);
}

module edge(){
  difference(){
    union(){
      difference(){
	cube([OFFSET, EDGE_WIDTH, OFFSET]);
	translate([THICKNESS, -.01 * EDGE_WIDTH/2, THICKNESS])scale([1, 1.01, 1])cube([LENGTH, EDGE_WIDTH, LENGTH]);
      }
      union(){
	translate([OFFSET, EDGE_WIDTH/2, 0])cylinder(r=EDGE_WIDTH / 2, h=THICKNESS);
	translate([0, EDGE_WIDTH/2, OFFSET])rotate(v=[0, 1, 0], a=90)cylinder(r=EDGE_WIDTH / 2, h=THICKNESS);
      }
    }
    translate([OFFSET, EDGE_WIDTH/2, 0])cylinder(r=SCREW_R, h=THICKNESS * 1.1);
    translate([-.01, EDGE_WIDTH/2, OFFSET])rotate(v=[0, 1, 0], a=90)cylinder(r=SCREW_R, h=THICKNESS * 10);
  }
}

module inside_edge(){
  difference(){
    edge();
    translate([OFFSET, EDGE_WIDTH/2, THICKNESS - HEX_THICKNESS])scale([1, 1, 1.01])hex();
    translate([THICKNESS - HEX_THICKNESS, EDGE_WIDTH/2, OFFSET])rotate(v=[0, 1, 0], a=90)scale([1, 1, 1.01])hex();
  }
}
module outside_edge(){
  difference(){
    edge();
    translate([OFFSET, EDGE_WIDTH/2, 0])scale([1, 1, 1.01])cylinder(r=HEX_R * 1.1, h=HEX_THICKNESS);
    translate([0, EDGE_WIDTH/2, OFFSET])scale([1, 1, 1.01])rotate(v=[0, 1, 0], a=90)cylinder(r=HEX_R * 1.1, h=HEX_THICKNESS);
  }
}

module corner(){
  union(){
    //tab(0, 0);
    //tab(LENGTH, 0);
    //tab(0, LENGTH);

    intersection(){
      difference(){
	cube(LENGTH);
	translate([THICKNESS, THICKNESS, THICKNESS])
	  cube(LENGTH);
	translate([OFFSET, OFFSET, 0])
	  translate([0, 0, -1])
	  cylinder(r=SCREW_R, h=2 * THICKNESS);
	translate([OFFSET, 0, OFFSET])
	  rotate(v=[-1, 0, 0], a=90) 
	  translate([0, 0, -1])
	  cylinder(r=SCREW_R, h=2 * THICKNESS);
	translate([0, OFFSET, OFFSET])
	  rotate(v=[0, 1, 0], a=90)
	  translate([0, 0, -1])
	  cylinder(r=SCREW_R, h=2 * THICKNESS);
      }
      
      union(){
	rotate(v=[0, 0, -1], a=0)cylinder(r=LENGTH*1.04, h=THICKNESS);
	rotate(v=[-1,0, 0], a=90)cylinder(r=LENGTH*1.04, h=THICKNESS);
	rotate(v=[0, 1, 0], a=90)cylinder(r=LENGTH*1.04 , h=THICKNESS);
      }
      // sphere(r=LENGTH);
    }
  }
}
module inside_corner(){
  difference(){
    corner();
    translate([OFFSET, OFFSET, THICKNESS - HEX_THICKNESS])scale([1, 1, 100])hex();
    translate([OFFSET, THICKNESS - HEX_THICKNESS,  OFFSET])
      rotate(v=[-1, 0, 0], a=90)
      scale([1, 1, 100])
      hex();
    translate([THICKNESS - HEX_THICKNESS, OFFSET,  OFFSET])
      rotate(v=[0, 1, 0], a=90)
      scale([1, 1, 100])
      hex();
  }
}
module outside_corner(){
  difference(){
    corner();
  translate([OFFSET,  HEX_THICKNESS, OFFSET])
    rotate(v=[1, 0, 0], a=90)
    rotate(v=[0, 0, 1], a=180)
    scale([1, 1, 100])
    cylinder(r=HEX_R * 1.1);
  translate([HEX_THICKNESS, OFFSET, OFFSET])
    rotate(v=[0, -1, 0], a=90)
    scale([1, 1, 100])
    cylinder(r=HEX_R * 1.1);
  translate([OFFSET,  OFFSET, HEX_THICKNESS, ])
    rotate(v=[0, 1, 0], a=180)
    scale([1, 1, 100])
    cylinder(r=HEX_R * 1.1);
  }
}

//corner();
//translate([2 * inch, 0, 0])outside_corner();
//translate([-2 * inch, 0, 0])
// inside_corner();
inside_edge();

