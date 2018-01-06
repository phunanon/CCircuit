#ifndef FILEIO_INCLUDE
#define FILEIO_INCLUDE

#include <fstream>      //For reading/writing
#include <string>       //For use of strings
#include <iostream>     //For output to the terminal
#include <thread>       //For threads, and sleeping
#include <chrono>       //For thread sleeping

#include "globals.hpp"
#include "electrify.hpp"

extern void saveBoard (std::string, bool = false);
extern void loadBoard (std::string, bool = false);

#endif
