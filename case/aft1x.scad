//############################################################
//         Afterburner 1.X case (ver. 2)
//############################################################

//Rendering details
details = 60; // [60:Preview-Quick, 360:Export-Slow]

//Export Top case
enableTop = true;

//Export Bottom case
enableBottom = true;

//Export power switch
enablePowerSwitch = true;

//Disaply Arduino
enableArduino = true;

//Draw text logo
enableTexts = true;

//Distance between the top and the bottom case
caseDistance = 32; //[32:200]

//Overall Case scale - options for print services
caseScale = 1000; // [1000: 100% Exact printer, 996: 99.6% JLCPCB]

//################################################################
module __Customizer_Limit__ () {} 
$fn = details;

//Arduino model
include <arduino.scad>


// Case edge radius
edgeR = 2;

// Case Wall thickness
wallT = 2;

// space for the case mounting holes
spaceD = 8;

caseW = 76;
caseD = 58 + (2 * spaceD); //58
caseH = 2;


module outline(wall = 1) {
  difference() {
    offset( 0) children();
    offset(-wall) children();
  }
}

// Afterburner shield
module afterburner() {
    pcbHeight = 1.6;
    linear_extrude(pcbHeight) square([54, 55.4]);
    //place components
    translate([0,0, pcbHeight]) {
        // Ziff socket
        translate([5.3, 15.6, 0]) linear_extrude(12) square([15, 41]);

        // On/Off switch
        translate([25.7, 36.3, 0]) linear_extrude(5) square([4, 8.7]);
        // On/Off switch lever
        translate([26.7, 38.6, 5]) linear_extrude(6) square([2, 4]);

        //LED
        translate([24.2, 18.5, 0]) linear_extrude(4) circle(2);


        // VPP supply - pcb
        translate([30.5, 13.5, 3.3]) linear_extrude(1.2) square([17.4, 37.2]);
        // VPP supply - trimmer
        translate([35.5, 21, 3.3 + 1.2]) linear_extrude(5) square([10, 9.5]);
        // VPP supply - trimmer knob
        translate([45.5, 22.2, 3.3 + 1.2 + 3.3]) rotate([0,90,0]) linear_extrude(2) circle(1.2);

        // screwdriver shaft
        translate([48.5, 22.2, 3.3 + 1.2 + 3.3]) rotate([0,90,0]) linear_extrude(35 + spaceD) circle(1.2);

        //VPP measurement header
        translate([25.8, 11.5, 0]) linear_extrude(12.2) square([2.7, 5.4]);
        
    }
}

module nutHolder() {
    size = spaceD + 1;
    center = size / 2;
    difference() {
        // main box
        linear_extrude(height = 7.5) square([size, size]);
        // holes
        {
            //central hole
            translate([center, center, -0.01])  linear_extrude(height = 10.5) circle(2.5);                
            //retention dip 1
            translate([center - 1, center - (center/2) - 1.2, 1.5])  linear_extrude(height = 3.5) square([2.4, 6.5]);
            //retention dip 2
            translate([center - (center/2) - 1, center - 1.2, 2.0])  linear_extrude(height = 3.4) square([6.5, 2.4]);
        }
    }
    // M3 screw mockup
    %translate([center, center, -12])  linear_extrude(height = 16) circle(1.3);                
    
}

module caseTop() {
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
                color (skirt1) translate([0,0, -wall2H + 0.001]) {
                    linear_extrude(2) {
                        outline(wallT) offset(edgeR-0.5) square([caseW-0.5, caseD-0.5]);
                    }
                }
                color (skirt2) translate([0.25, 0.25, -wall2H -2]) {
                    linear_extrude(3.5) {
                        outline(skirtT) offset(edgeR - wallT - 0.1) square([caseW-0.5, caseD-0.5]);
                    }
                }
            }
            // case mounting holes with nuts
            translate([0,0, -wallH]) {
                holderSize = spaceD + 1;
                nutHolder();
                translate([caseW - holderSize, 0, 0]) nutHolder();
                translate([0, caseD - holderSize, 0]) nutHolder();
                translate([caseW - holderSize, caseD - holderSize, 0]) nutHolder();
            }
            
            translate([0, spaceD ,0]) {
                //ziff lever box (+)
                translate([74, 42, -9]) linear_extrude(height = 10) square([3, 12]);
                
                //ziff skirt
                translate([31, 33,-4.5]) linear_extrude(height = 6) square([44.5, 20.5]);
                
                // VPP trimmer access box (+)
                translate([30.5, -1 - spaceD, -7]) linear_extrude(height = 9) square([18, 6 + spaceD]);
                
                // LED hole retention (bottom side)
                translate([37.25, 25.8, -2])  linear_extrude(height = 2.3) square([6,6]);
                
            }            
        }
        // case cut-throughs
        {
            translate([0, spaceD ,0]) {
                // USB socket hole
                translate([-3, 34,-wallH - 3.7]) linear_extrude(height = 15.5) square([6, 14]);

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
                    translate([33,35,-9.5]) linear_extrude(height = 12) square([42, 16.5]);

                    //ziff lever box (-)
                    translate([69, 45.5, -7]) linear_extrude(height = 10) square([9.1, 6]);
                    
                    // VPP trimmer access box (-)
                    translate([34.5, -13, -5]) linear_extrude(height = 9) offset(2) square([10, 6 + spaceD]);

                    // VPP trimmer access box  hole 
                    translate([39.9, 11, -2]) rotate([90,0,0]) linear_extrude(height = 10) circle(2);

                    //LED hole - use light pipe
                    //translate([35.6, 31.5, 1.01])  linear_extrude(height = 1) circle(2);
                    //translate([40, 29, 1.01])  linear_extrude(height = 1) circle(2);
                    translate([40, 29, -2.1])  linear_extrude(height = 5) circle(1.7);

                    //Power switch hole
                    translate([53.2, 25.7, -5]) linear_extrude(height = 10) square([9.5, 5]);
                    
                    //VPP measurement header hole
                    translate([29.8, 28.0, -5]) linear_extrude(height = 10) offset(1) square([4.2, 1.7]);
                }
            }
        }
    }
}

module antislipperyRing() {
    translate([0,0,-2.7])linear_extrude(2.7) outline(1) circle(7.3);
    % translate([0,0,-4.02]) linear_extrude(4) circle(6.1);
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
                            arduino(boardType = UNO, useColors = false);
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
            //anti-slippery rings 
            {
                centerR = 14;
                translate([centerR, centerR]) antislipperyRing();
                translate([caseW - centerR, centerR]) antislipperyRing();
                translate([centerR, caseD - centerR]) antislipperyRing();
                translate([caseW - centerR, caseD - centerR]) antislipperyRing();
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
        }
    }
    
}

module powerSwitch() {
    difference() {
        union() {
            linear_extrude(4.5) square([18,6]);
            translate([7.5, 1.6])linear_extrude(10) offset(1) square([3, 2.4]);
        }
        translate([8.15, 2.15, -0.01])linear_extrude(5.2) square([1.7, 1.7]);
    }
}
// allows to fine-tune the overall size before export
worldScale = caseScale / 1000;

scale(worldScale) {
    if (enableBottom) caseBottom();
    if (enableTop) translate([0,0, caseDistance + 0.2]) caseTop();
    if (enablePowerSwitch) translate([47, spaceD + 25.4, caseDistance - 4.6]) powerSwitch();
}
