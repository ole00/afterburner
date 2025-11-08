/*

 EXERCISER : a script runner that exercises IC pins
 
 part of Afterburner GAL project

*/

#pragma once

#define exerciseCheckFile(B,S) exerciseFile(B,S,1)
#define exerciseRunFile(B,S) exerciseFile(B,S,0)


int exerciseGetPulseDuration(void);
int exerciseGetPinCount(void);
void exerciseSetVerbose(char verbose);
int exerciseFile(char* buffer, int bufSize, int check);
