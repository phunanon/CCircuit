#ifndef DISPLAY_INCLUDE
#define DISPLAY_INCLUDE

#include <iostream>     //For output to the terminal
#include <stdio.h>      //For output to the terminal: getchar; system ()
#include <sys/ioctl.h>  //For getting terminal size
#include <unistd.h>     //"

#include "globals.hpp"

extern bool to_crosshairs;
void display ();

#endif
