//############################################################
//         Afterburner 3.X case (ver. 2c)
//############################################################

//Export Top case
enableTop = true;

//Export Bottom case
enableBottom = true;

//Export power switch
enablePowerSwitch = true;

//Disaply Arduino
enableArduino = true;

//VPP via MT3608 module (On) or via discrete componets (Off)
vppModule = true;

//Hole for VPP measurement header
vppMeasurementHole = true;

//Hole for VPP trimmer
vppTrimmerHole = true;

//Depressions for anti-slippery pads on the bottom of the case
enableAntislipperyRings = true;

//Size of the USB hole
powerType = 0; // [0: USB-B for Uno R3, 10: USB-C for Uno-R4] 

//Screw inserts hole shape
screwInsertsType = 0; // [0: FDM - heat inserts, 10: SLA - hot glue inserts]

//Draw text logo
enableTexts = true;

//Distance between the top and the bottom case
caseDistance = 200; //[32:200]

//Overall Case scale - options for print services
caseScale = 1000; // [1000: 100% Exact printer, 996: 99.6% JLCPCB]

//################################################################

module __Customizer_Limit__ () {} 
//Rendering details
details = 120; // [60:Preview-Quick, 360:Export-Slow]

$fn = details;

//Arduino model
include <arduino.scad>


// Case edge radius
edgeR = 2;

// Case Wall thickness
wallT = 2;

// space for the case mounting holes
spaceD = 8;

caseW = 96;
caseD = 58 + (2 * spaceD); //58
caseH = 2;

useUsbC = powerType > 1;
useFdmInserts = screwInsertsType < 1;

module outline(wall = 1) {
  difference() {
    offset( 0) children();
    offset(-wall) children();
  }
}

// Afterburner shield
module afterburner() {
    pcbHeight = 1.6;
    difference() {
        union() {
            translate([0,-9,0]) linear_extrude(pcbHeight) square([54, 85]);
            //place components
            translate([0,0, pcbHeight]) {
                // Ziff socket
                translate([5.3, 15.6, 0]) linear_extrude(12) square([15, 46]);

                // On/Off switch
                translate([23, 34, 0]) {
                    translate([0, 0.7, 0]) linear_extrude(7) square([7, 7]);
                    // On/Off switch lever
                    translate([2, 2.8, 5]) linear_extrude(6) square([3, 3]);
                    // On/Off switch lever - measure pivot
                    //#translate([2, 2.8, 5]) linear_extrude(116) square([3, 3]);
                }

                //LED
                #translate([27, 20.5, 0]) linear_extrude(14) circle(1.5);


                //vpp module
                if (vppModule)
                translate([30.5, 5.5, 3.3]) {
                    // VPP supply - pcb
                    translate([0, 0, 0]) linear_extrude(1.2) square([17.4, 37.2]);
                    // VPP supply - trimmer
                    translate([5, 7.5, 1.2]) linear_extrude(5) square([10, 9.5]);
                    // VPP supply - trimmer knob
                    translate([15, 8.7, 1.2 + 3.3]) rotate([0,90,0]) linear_extrude(2) circle(1.2);

                    // screwdriver shaft
                    translate([18, 8.7, 1.2 + 3.3]) rotate([0,90,0]) linear_extrude(35 + spaceD) circle(1.2);
                }

                //VPP measurement header
                if (vppMeasurementHole) {
                    translate([17, -5, 0]) linear_extrude(12.2) square([5.4, 2.7]);
                }
                
                //VPP trimmer
                translate([6, -0.5, 0]) {
                    linear_extrude(6) square([6.2, 6.2]);
                    if (vppTrimmerHole) {
                        translate([3.1, 3.1, 3.3]) rotate([0,0,0]) linear_extrude(35 + spaceD) circle(1.2);
                    }
                }
            }
        }
        union() {
            translate([9.6, 66.8, -2.1])  linear_extrude(height = 5) circle(1.6);
        }
    }
}

module nutHolder(wallH) {
    boxH = 7.5;
    size = spaceD + 1;
    center = size / 2;
    difference() {
        union() {
            // main box
            linear_extrude(height = boxH) square([size, size]);
            if (useFdmInserts) {
                //support bellow holder
                translate([0, 0, boxH - 0.05]) {
                    linear_extrude(height = wallH - boxH + 0.2) square([size, size]);
                }
            }
        }
        // holes
        {
            if (useFdmInserts) {
                translate([center, center, -0.01]) {
                    //central hole
                    linear_extrude(height = 10.5) circle(2);
                    //hole chamfer
                    cylinder(r1 = 2.6, r2 = 2, h = 1, center = true, $fn=32);
                }
            } else {
                //central hole
                translate([center, center, -0.01])  linear_extrude(height = 10.5) circle(2.5);                
                //retention dip 1
                translate([center - 1, center - (center/2) - 1.2, 1.5])  linear_extrude(height = 3.5) square([2.4, 6.5]);
                //retention dip 2
                translate([center - (center/2) - 1, center - 1.2, 2.0])  linear_extrude(height = 3.4) square([6.5, 2.4]);
            }
        }
    }
    // M3 screw mockup
    %translate([center, center, -12])  linear_extrude(height = 16) circle(1.3);                
    
}

module caseTop() {
    ziffW = 47; //42
    wall2H = 0;
    wallH = 22.0;
    skirtT = 1.8; //skirt thickness
    skirt1 = [0.3, 0.3, 0.9]; //blue
    skirt2 = [0.3, 0.8, 0.9]; //cyan
    
    
    difference() {
        union() {
            // base shape
            translate([0,0,0]) linear_extrude(height = caseH) offset(edgeR) square([caseW, caseD]);
            
            //base wall
            translate([0,0,-wallH]) {
                linear_extrude(wallH) {
                    outline(wallT) offset(edgeR) square([caseW, caseD]);
                }
                
                //wall skirt small and closer to wall
                color (skirt1) translate([0,0, -wall2H - 0.005]) {
                    linear_extrude(2) {
                        outline(wallT) offset(edgeR-0.5) square([caseW-0.5, caseD-0.5]);
                    }
                }
                color (skirt2) translate([0.25, 0.25, -wall2H -2]) {
                    linear_extrude(3.5) {
                        outline(skirtT) offset(edgeR - wallT - 0.1) square([caseW-0.5, caseD-0.5]);
                    }
                }
                
                //wall skirt print supports
                translate([9, 0.5,0.4]) rotate([0,90, 0]) rotate([0,0,120]) linear_extrude(caseW - 17) square([1.9, 5]);
                translate([9, caseD - 2.15,1.4]) rotate([0,90, 0]) rotate([0,0,60]) linear_extrude(caseW - 17) square([1.9, 5]);
                translate([2.15, 9,1.4]) rotate([0,90, 90]) rotate([0,0,60]) linear_extrude(caseD - 17) square([1.9, 5]);
                translate([caseW - 0.5, 9,0.4]) rotate([0,90, 90]) rotate([0,0,120]) linear_extrude(caseD - 17) square([1.9, 5]);
            }
            // case mounting holes with nuts
            translate([0,0, -wallH]) {
                holderSize = spaceD + 1;
                nutHolder(wallH);
                translate([caseW - holderSize, 0, 0]) nutHolder(wallH);
                translate([0, caseD - holderSize, 0]) nutHolder(wallH);
                translate([caseW - holderSize, caseD - holderSize, 0]) nutHolder(wallH);
            }
            
            translate([0, spaceD ,0]) {
                //ziff lever box (+)
                translate([78, 46, -7.3]) linear_extrude(height = 8) square([19, 21]);
                
                //ziff skirt - v2b
                translate([32, 33,-2]) linear_extrude(height = 3.5) square([ziffW + 18, 20.5]);
                
                // VPP trimmer access box (+)
                if (vppModule) {
                    translate([22.5, -1 - spaceD, -7]) linear_extrude(height = 9) square([18, 6 + spaceD]);
                }
                
                // LED hole retention (bottom side)
                //    translate([37.25, 25.8, -2])  linear_extrude(height = 2.3) square([6,6]);
                
            }            
        }
        // case cut-throughs
        {
            translate([0, spaceD ,0]) {
                // USB socket hole
                if (useUsbC) {
                    translate([-3, 35,-wallH + 3.5]) rotate([0,90,0]) linear_extrude(height = 9) offset(2) square([5.5, 11]);
                } else {
                    translate([-3, 34,-wallH - 3.7]) linear_extrude(height = 15.5) square([6, 14]);
                }

                // Power jack hole
                translate([-3, 5.6,-wallH - 3.7]) linear_extrude(height = 15.5) square([6.01, 10]);

                // texts
                if (enableTexts) {
                    translate([0, -spaceD + caseD / 2, -0.79]) {
                        translate([caseW / 2, -19, caseH]) scale([1,1.2,1]) linear_extrude(height=1) text("Afterburner", size=7.5, font="Liberation Mono:style=Bold Italic", halign="center");
                        translate([70, -3.0, caseH]) linear_extrude(height=1.3) text("ON", size=5, font="DejaVu Sans Mono:style=Bold", halign="center");
                    }
                }

                union() {
                    // Ziff socket hole
                    translate([33,35,-9.5]) linear_extrude(height = 12) square([ziffW, 16.5]);

                    //ziff lever box (-)
                    translate([ziffW + 32.95, 47, -6]) linear_extrude(height = 9) square([26, 26]);
                    translate([81, 25, -8]) linear_extrude(height = 8) square([15, 21]);
                    
                    if (vppModule) translate([26.5, -13, -5]) {
                        // VPP trimmer access box (-)
                        linear_extrude(height = 9) offset(2) square([10, 6 + spaceD]);

                        // VPP trimmer access box  hole 
                        translate([5.4, 24, 3]) rotate([90,0,0]) linear_extrude(height = 10) circle(2);
                    }

                    //LED hole - use light pipe - v2a
                    //translate([35.6, 31.5, 1.01])  linear_extrude(height = 1) circle(2);
                    //translate([40, 29, 1.01])  linear_extrude(height = 1) circle(2);
                    translate([38, 29, -2.1])  linear_extrude(height = 5) circle(1.7);

                    //Power switch hole - v2b
                    translate([56.3, 29.2, -5]) linear_extrude(height = 10) circle(4.2);
                    difference() {
                        translate([56.4, 29.2, -9.2]) linear_extrude(height = 10) circle(7);
                        translate([45.8, 33, -9.2]) linear_extrude(height = 10) square([20,5]);
                    }
                    
                    //VPP trimmer hole - v2b
                    if (vppTrimmerHole) {
                        translate([20, 46.7, -2.1])  linear_extrude(height = 5) circle(2);
                    }
                    
                    //VPP measurement header hole
                    if (vppMeasurementHole) {
                        translate([13.5, 34.0, -5]) linear_extrude(height = 10) offset(1) square([1.7, 4.2]);
                    }
                }
            }
        }
    }
}

module antislipperyRing() {
    translate([0,0,-1.5])linear_extrude(2.7) circle(6.3);
    //% translate([0,0,-4.02]) linear_extrude(4) circle(6.1);
}

module caseBottom() {
    wallH = 7.5;
    holderSize = spaceD + 1;
    center = holderSize / 2;


    difference() {
        union() {
            // base shape
            translate([0,0,0]) linear_extrude(height = caseH) offset(edgeR) square([caseW, caseD]);
            
            //base wall
            translate([0,0,caseH]) {
                linear_extrude(wallH) {
                    outline(wallT) offset(edgeR) square([caseW, caseD]);
                }
            }
            translate([2.2,56 + spaceD,1]) rotate([0, 0, -90]) {
                    //Arduino standoffs
                    standoffs(height = 7, holeRadius = 1.35, bottomRadius = 4, topRadius = 3.5);
                    //top-left standoff rod
                    translate([2.4, 15.2, 0]) linear_extrude(9) circle(1.4);
                    //place Arduino UNO on top of the standoffs
                    if (enableArduino) {
                        % translate([0,0,7.01]) {
                            arduino(boardType = UNO, useColors = false, useUsbC = useUsbC);
                            translate([0,15.5, 12.6]) afterburner();
                        }
                    }
            }
            // mounting holes extra wall
            {
                translate([center, center, 0.95])  linear_extrude(height = 3) circle(5); //top thick
                translate([center + caseW - holderSize, center, 0.95]) linear_extrude(height = 3) circle(5);
                translate([center, center + caseD - holderSize, 0.95]) linear_extrude(height = 3) circle(5);
                translate([center + caseW - holderSize, center + caseD - holderSize, 0.95]) linear_extrude(height = 3) circle(5);
                
            }
            
            // extra standoff for afterburner PCB -v2c
            translate([84.5, 54.7, 2.2])
            difference() {
                cylinder(r1 = 6, r2 = 4, h = 17.5, $fn=32);
                translate([0,0,12]) cylinder(r =  1.6, h = 15, center = true, $fn=32);
            }

        }
        //mounting holes
        {
            // case mounting holes
            translate([center, center, -2]) {
                linear_extrude(height = 6) circle(1.7);
                linear_extrude(height = 4.3) circle(3.5); //recess
            }
            
            translate([center + caseW - holderSize, center, -2]) {
                linear_extrude(height = 6) circle(1.7);
                linear_extrude(height = 4.3) circle(3.5); //recess
            }

            translate([center, center + caseD - holderSize, -2]) {
                linear_extrude(height = 6) circle(1.7);
                linear_extrude(height = 4.3) circle(3.5); //recess
            }

            translate([center + caseW - holderSize, center + caseD - holderSize, -2]) {
                linear_extrude(height = 6) circle(1.7);
                linear_extrude(height = 4.3) circle(3.5); //recess
            }
            
            // top left post - dips for soldered pins
            translate([17.4, 58, 5]) linear_extrude(height=6) circle(1);
            translate([20.8, 61.5, 5]) linear_extrude(height=6) circle(1);
            
            //anti-slippery rings 
            if (enableAntislipperyRings) {
                centerR = 14;
                translate([centerR, centerR]) antislipperyRing();
                translate([caseW - centerR, centerR]) antislipperyRing();
                translate([centerR, caseD - centerR]) antislipperyRing();
                translate([caseW - centerR, caseD - centerR]) antislipperyRing();
            }
            
            // hole around USB-C connector
            if (useUsbC) {
                translate([-3, 43,-wallH + 21]) rotate([0,90,0]) linear_extrude(height = 4) offset(2) square([4, 11]);

            }
        }
    }
    
}

module powerSwitch() {
    translate([1.4,0,5])
    difference() {
        union() {
             {
                difference() {
                    translate([8, 3])linear_extrude(1) circle(5.5);
                    translate([0, 7, -0.4]) linear_extrude(2) square([16,7]);
                }
                translate([8, 3.8]) {
                       linear_extrude(4) circle(3.6);
                       translate([0,0,4]) cylinder(r1 = 3.5, r2 = 3.2, h = 1);
                }
            }
        }
        {
            translate([0,0,0]) {
                translate([6.4, 2.7, -0.01])linear_extrude(3.5) square([3.2, 2.1]);
            }
        }
    }
}
// allows to fine-tune the overall size before export
worldScale = caseScale / 1000;

scale(worldScale) {
    if (enableBottom) caseBottom();
    if (enableTop) translate([0,0, caseDistance + 0.2]) caseTop();
    if (enablePowerSwitch) translate([47, spaceD + 25.4, caseDistance - 4.6]) powerSwitch();
}
