//TODO
//Swap y and x throughout code
//Move variables into local scope
//Don't allow movement out of bounds when pasting
#include <iostream>     //For output to the terminal
#include <stdio.h>      //For output to the terminal: getchar; system ()
#include <string>       //For use of strings
#include <thread>       //For threads, and sleeping
#include <chrono>       //For thread sleeping
#include <ctime>        //For time
#include <fstream>      //For reading/writing
#include <sys/ioctl.h>  //For getting terminal size
#include <unistd.h>     //"
#include <vector>       //For use of vectors
#include "keypresses.c" //For detecting keypresses: kbhit (), pressedCh

const std::string SYSPATH = ""; //"/system/xbin"; //For BusyBox on Android

std::string projName = "untitled";
bool run = true;

const uint32_t boardW = 4096;
const uint32_t boardH = 4096;
const uint32_t boardWh = boardW / 2;
const uint32_t boardHh = boardH / 2;
char board[boardH][boardW];
int cursorX = 16;
int cursorY = 8;
const uint8_t UNTILHYPER = 20;
const uint8_t HYPER = 2;

uint8_t switches[10];
bool placedSwitches[10];
uint8_t switch_num;

int elecY = boardH, elecX = boardW, elecY2 = 0, elecX2 = 0; //Electric bounds

const uint32_t UNDOS = 512;
uint32_t undos[UNDOS][4]; //[u][y, x, was, now]
uint32_t canr = 0;
uint32_t u = 0;

bool copying, dataToPaste;
uint32_t copyY, copyX, copyY2, copyX2, copyYDist, copyXDist;
uint32_t pasteY, pasteX, pasteY2, pasteX2;
std::vector<char> *copyData = new std::vector<char>();

bool autoBridge = false;
bool dirWire = false;
bool slowMo = false;
bool fastMo = false;
bool paused = false;
bool labelMode = false; //Are we typing a label?

uint32_t screenH = 20; //The height of the screen
uint32_t screenW = 60; //The width of the screen
uint32_t screenHh = screenH / 2; //Half of the height of the screen
uint32_t screenWh = screenW / 2; //Half of the width of the screen


void clearScreen () { std::cout << "\033[2J\033[1;1H"; }

int boardCropX;
int boardCropY;
uint32_t boardCropX2;
uint32_t boardCropY2;
void calcBoardCrops () //For calculating the part of the board on the screen
{
    boardCropX = cursorX - screenWh;
    if (boardCropX < 0) { boardCropX = 0; }
    boardCropX2 = boardCropX + screenW;
    if (boardCropX2 > boardW) { boardCropX = boardW - screenW; boardCropX2 = boardW; }
    boardCropY = cursorY - screenHh;
    if (boardCropY < 0) { boardCropY = 0; }
    boardCropY2 = boardCropY + screenH;
    if (boardCropY2 > boardH) { boardCropY = boardH - screenH; boardCropY2 = boardH; }
}

std::pair<uint16_t, uint16_t> getTerminalDimensions ()
{
    std::pair<uint16_t, uint16_t> dim;
    #ifdef TIOCGSIZE
        struct ttysize ts;
        ioctl(STDIN_FILENO, TIOCGSIZE, &ts);
        dim.first = ts.ts_cols;
        dim.second = ts.ts_lines;
    #elif defined(TIOCGWINSZ)
        struct winsize ts;
        ioctl(STDIN_FILENO, TIOCGWINSZ, &ts);
        dim.first = ts.ws_col;
        dim.second = ts.ws_row;
    #endif
}
void resizeScreen ()
{
    auto dim = getTerminalDimensions();
    screenH = (dim.first < boardH ? dim.first: boardH) - 1;
    screenW = (dim.second < boardW ? dim.second : boardW);
    screenHh = screenH / 2;
    screenWh = screenW / 2;
}

#define NODIR       0
#define NORTH       1
#define EAST        2
#define SOUTH       3
#define WEST        4

#define EMPTY       0
#define UN_WIRE     1
#define PW_WIRE     2
#define UN_AND      3
#define UN_NOT      4
#define UN_XOR      5
#define UN_BRIDGE   6
#define PW_BRIDGE   7
#define PW_POWER    8
#define UN_WALL     9
#define UN_S_DIODE  10
#define PW_S_DIODE  11
#define UN_BIT      12
#define PW_BIT      13
#define UN_N_DIODE  14
#define PW_N_DIODE  15
#define UN_E_DIODE  16
#define PW_E_DIODE  17
#define UN_W_DIODE  18
#define PW_W_DIODE  19
#define U1_STRETCH  20
#define P1_STRETCH  21
#define P2_STRETCH  22
#define P3_STRETCH  23
#define UN_DELAY    24
#define PW_DELAY    25
#define UN_H_WIRE   26
#define PW_H_WIRE   27
#define UN_V_WIRE   28
#define PW_V_WIRE   29
#define PW_AND      33
#define PW_NOT      34
#define PW_XOR      35

bool crosshairs = false;
void display ()
{
    resizeScreen();
    calcBoardCrops();
    std::string buffer;
    std::string buff;
    buffer = "";
    clearScreen();
  //Top bar
    uint32_t barOffLen = 0;
    buffer += "\033[0;37;40m"; //bg of bar
    barOffLen += 10;
    buffer += std::to_string(cursorX) + ", " + std::to_string(cursorY);
    buffer += " ";
    for (uint32_t i = 0; i < 10; ++i) {
        buffer += "\033[37;40m " + (switches[i] ? "\033[30;42m" + std::to_string(i) : (placedSwitches[i] ? "\033[30;43m" + std::to_string(i) : "\033[37;40m" + std::to_string(i)));
        barOffLen += 16;
    }
    buffer += "\033[37;40m";
    barOffLen += 8;
    buffer += (!labelMode && !copying ? "  " + std::string(dirWire ? "direct" : "genperp") + " wire" : ""); 
    buffer += (copying ? "  selecting (x complete)" : (dataToPaste ? "  ready (x dismiss, b paste, k cut, K swap, j clear, m paste x-flip, w y-flip)" : ""));
    buffer += (autoBridge ? "  auto bridging" : "");
    buffer += (slowMo ? "  slow-mo" : (fastMo ? "  fast-mo" : ""));
    buffer += (paused ? "  paused" : "");
    buffer += (labelMode ? "  label mode" : "");
    std::string space = "";
    uint32_t sLen = screenW - (buffer.length() - barOffLen) - projName.length();
    for (uint32_t i = 0; i < sLen; ++i) { space += " "; }
    buffer += space + "\033[1;4m" + projName + "\033[0m";
  //Board
    int sy, sx;
    for (int y = boardCropY, sy = 0; y < boardCropY2; ++y, ++sy) {
        buffer += '\n';
        for (int x = boardCropX, sx = 0; x < boardCropX2; ++x, ++sx) {
            buff = " ";
            char *look = &board[y][x];
            switch (*look) {
                case EMPTY:
                    buff = (y >= elecY && y < elecY2 + 1 && x >= elecX && x < elecX2 + 1 ? "\033[0;30;47m " : "\033[0;30;47m."); //Nothing
                    break;
                case UN_WIRE:
                    buff = "\033[0;30;47m#"; //Unpowered Wire
                    break;
                case PW_WIRE:
                    buff = "\033[0;30;42m#"; //Powered Wire
                    break;
                case UN_AND:
                    buff = "\033[0;37;43mA"; //AND
                    break;
                case PW_AND:
                    buff = "\033[1;37;43mA"; //Powered AND
                    break;
                case UN_NOT:
                    buff = "\033[0;37;41mN"; //NOT
                    break;
                case PW_NOT:
                    buff = "\033[1;37;41mN"; //Powered NOT
                    break;
                case UN_XOR:
                    buff = "\033[0;37;45mX"; //XOR
                    break;
                case PW_XOR:
                    buff = "\033[1;37;45mX"; //Powered XOR
                    break;
                case UN_BRIDGE:
                    buff = "\033[0;30;47m+"; //Bridge
                    break;
                case PW_BRIDGE:
                    buff = "\033[0;30;42m+"; //Powered Bridge
                    break;
                case PW_POWER:
                    buff = "\033[1;37;44m@"; //Power
                    break;
                case UN_WALL:
                    buff = "\033[1;30;40m;"; //Wall
                    break;
                case UN_BIT:
                    buff = "\033[1;30;47mB"; //Bit
                    break;
                case PW_BIT:
                    buff = "\033[1;37;44mB"; //Powered Bit
                    break;
                case UN_N_DIODE:
                    buff = "\033[0;30;47m^"; //North Diode
                    break;
                case PW_N_DIODE:
                    buff = "\033[0;30;42m^"; //Powered North Diode
                    break;
                case UN_E_DIODE:
                    buff = "\033[0;30;47m>"; //East Diode
                    break;
                case PW_E_DIODE:
                    buff = "\033[0;30;42m>"; //Powered East Diode
                    break;
                case UN_S_DIODE:
                    buff = "\033[0;30;47mV"; //South Diode
                    break;
                case PW_S_DIODE:
                    buff = "\033[0;30;42mV"; //Powered South Diode
                    break;
                case UN_W_DIODE:
                    buff = "\033[0;30;47m<"; //West Diode
                    break;
                case PW_W_DIODE:
                    buff = "\033[0;30;42m<"; //Powered West Diode
                    break;
                case UN_DELAY:
                    buff = "\033[1;30;47m%"; //Delay
                    break;
                case PW_DELAY:
                    buff = "\033[1;30;42m%"; //Powered Delay
                    break;
                case U1_STRETCH:
                case P1_STRETCH:
                    buff = "\033[1;30;47m$"; //Stretcher
                    break;
                case P2_STRETCH:
                case P3_STRETCH:
                    buff = "\033[1;30;42m$"; //Powered Stretcher
                    break;
                case UN_H_WIRE:
                    buff = "\033[0;30;47m-"; //Horizontal Wire
                    break;
                case PW_H_WIRE:
                    buff = "\033[0;30;42m-"; //Powered Horizontal Wire
                    break;
                case UN_V_WIRE:
                    buff = "\033[0;30;47m|"; //Vertical Wire
                    break;
                case PW_V_WIRE:
                    buff = "\033[0;30;42m|"; //Powered Vertical Wire
                    break;
            }
            
            if (*look >= 50 && *look <= 59) { //Power
                switch_num = *look - 50;
                buff = (switches[switch_num] ? "\033[1;33;44m" + std::to_string(switch_num) : "\033[1;31;47m" + std::to_string(switch_num)) ; //Power switch
            }
            
            if (*look >= 97 && *look <= 122) { //Label
                buff = "\033[4;1;34;47m" + std::string(look, look + 1);
            }

            if (copying) { //Copy box
                if (y == copyY - 1 && x >= copyX - 1 && x <= cursorX)    { buff = "\033[1;30;46mx"; } //North
                if (x == cursorX && y >= copyY - 1 && y <= cursorY)        { buff = "\033[1;30;46mx"; } //East
                if (y == cursorY && x >= copyX - 1 && x <= cursorX)        { buff = "\033[1;30;46mx"; } //South
                if (x == copyX - 1 && y >= copyY - 1 && y <= cursorY)    { buff = "\033[1;30;46mx"; } //West
            }
            if (dataToPaste) {
              //Copy markers
                if ((y == copyY && x == copyX) || (y == copyY && x == copyX2 - 1) || (y == copyY2 - 1 && x == copyX) || (y == copyY2 - 1 && x == copyX2 - 1)) {
                    buff = "\033[30;46m" + buff.substr(buff.length() - 1, buff.length());
                }
              //Pasted markers
                else if ((y == pasteY && x == pasteX) || (y == pasteY && x == pasteX2 - 1) || (y == pasteY2 - 1 && x == pasteX) || (y == pasteY2 - 1 && x == pasteX2 - 1)) {
                    buff = "\033[30;43m" + buff.substr(buff.length() - 1, buff.length());
                }
              //Paste area
                else if ((y >= cursorY && y < cursorY + copyYDist && x >= cursorX && x < cursorX + copyXDist)) {
                    buff = "\033[30;46m" + buff.substr(buff.length() - 1, buff.length());
                }
            }
          //Crosshairs and cursor
            if ((crosshairs && (y == cursorY || x == cursorX)) || (y == cursorY && x == cursorX) || (sy == 0 && x == cursorX) || (sy == screenH - 1 && x == cursorX) || (y == cursorY && sx == 0) || (y == cursorY && sx == screenW - 1)) {
                buff = buff.substr(buff.length() - 1, buff.length());
                buff = "\033[0;37;4" + std::string(dataToPaste ? "6" : "0") + "m" + buff;
            }

            buffer += buff;
        }
    }
    std::cout << buffer;
    fflush(stdout);
}



bool powerAtDir (int y, int x, uint8_t dir, bool dead = false)
{
    if (x < 0 || y < 0 || x >= boardW || y >= boardH) { return false; }
    char look = board[y][x];
    uint8_t diode;
    uint8_t wire;
    char xd = 0;
    char yd = 0;
    switch (dir) {
        case NORTH: diode = PW_S_DIODE; wire = PW_V_WIRE; yd = -1; break; //North: Powered S Diode, V Wire
        case EAST:  diode = PW_W_DIODE; wire = PW_H_WIRE; xd =  1; break; //East:  Powered W Diode, H Wire
        case SOUTH: diode = PW_N_DIODE; wire = PW_V_WIRE; yd =  1; break; //South: Powered N Diode, V Wire
        case WEST:  diode = PW_E_DIODE; wire = PW_H_WIRE; xd = -1; break; //West:  Powered E Diode, H Wire
    }
  //Check for conditions
    if (look == PW_BRIDGE) { //Powered Bridge
      //Check this Powered Bridge is powered in this direction
        x += xd;
        y += yd;
        while (board[y][x] == PW_BRIDGE) { //Seek along the Bridge
            x += xd;
            y += yd;
        }
        //Reached the end of the bridges
    }
    look = board[y][x];
    bool powerPresent = false, powerOn = false;
    if (look >= 50 && look <= 59) {
        powerPresent = true;
        powerOn = switches[look - 50];
    }
  //Return for if checking for dead
    if (dead) {
        --diode;
        --wire;
        if (powerPresent) { return !powerOn; }        //There's a Switch
        if (look == PW_POWER) { return false; }       //There's a Power
        if (look == UN_WIRE                           //
         || look == UN_BRIDGE                         //
         || look == wire                              //
         || look == diode                             //
         || look == UN_BIT                            //
         || (dir == NORTH ? look == UN_DELAY : false) //
         || look == U1_STRETCH                        // There's a dead Wire/Bridge/D Wire/Diode/Bit/Delay/Stretcher
           ) { return true; }
        return false; //Anything else could never power us
    }
  //Return for if checking for alive
    return look == PW_WIRE                      //
        || look == wire                         //
        || look == PW_BIT                       //
        || look == PW_POWER                     //
        || look == diode                        // Powered by Wire/D Wire/Bit/Power/Diode
        || (dir == NORTH ? look == PW_DELAY     //
                        || look == P1_STRETCH   //
                        || look == P2_STRETCH   //
                        || look == P3_STRETCH   //
           : false)                             // Powered by Delay/Stretcher
        || (powerPresent && powerOn);           // Power
}

//Fix bounds checking
uint8_t nextToLives (int y, int x, uint8_t mode) //mode: 0 AND, 1 OR, 2 NOT, 3 XOR, 4 Stretcher
{
    uint8_t lives = 0;
  //Check North
    if      (mode == 0 && powerAtDir(y - 1, x, NORTH, true)) { return 0; }
    else if (powerAtDir(y - 1, x, NORTH)) {
        ++lives;
    }
  //Check East
    if      (mode == 0 && powerAtDir(y, x + 1, EAST, true)) { return 0; }
    else if (powerAtDir(y, x + 1, EAST)) {
        ++lives;
    }
  //Check West
    if      (mode == 0 && powerAtDir(y, x - 1, WEST, true)) { return 0; }
    else if (powerAtDir(y, x - 1, WEST)) {
        ++lives;
    }
  //Return
    return lives;
}

uint8_t betweenLives (int y, int x)
{
    uint8_t lives = 0;
  //Check East
    if (powerAtDir(y, x + 1, EAST)) {
        ++lives;
    }
  //Check West
    if (powerAtDir(y, x - 1, WEST)) {
        ++lives;
    }
  //Return
    return lives;
}

struct Branch
{
    int x, y;
    uint8_t d;
};

Branch branch[2048];
uint32_t branches = 0;
int addBranch (int y, int x, uint8_t prevDir)
{
    branch[branches].y = y;
    branch[branches].x = x;
    branch[branches].d = prevDir; //0: none, NESW
    return branches++;
}

uint32_t nextB;
void elec () //Electrify the board appropriately
{
  //Reset the electrified
    for (int y = elecY; y <= elecY2; ++y) {
        for (int x = elecX; x <= elecX2; ++x) {
            char *look = &board[y][x];
            if      (*look == PW_WIRE)    { *look = UN_WIRE; }    //Powered Wire to Wire
            else if (*look == PW_H_WIRE)  { *look = UN_H_WIRE;  } //Powered H Wire to H Wire
            else if (*look == PW_V_WIRE)  { *look = UN_V_WIRE;  } //Powered V Wire to V Wire
            else if (*look == PW_BRIDGE)  { *look = UN_BRIDGE;  } //Powered Bridge to Bridge
            else if (*look == PW_N_DIODE) { *look = UN_N_DIODE; } //Powered N Diode to N Diode
            else if (*look == PW_E_DIODE) { *look = UN_E_DIODE; } //Powered E Diode to E Diode
            else if (*look == PW_S_DIODE) { *look = UN_S_DIODE; } //Powered S Diode to S Diode
            else if (*look == PW_W_DIODE) { *look = UN_W_DIODE; } //Powered W Diode to W Diode
        }
    }
  //Wires & inline components
    bool moved = true;
    bool skipped = false;
    while (moved) {
        moved = false;
        uint32_t prevB = 0;
        for (uint32_t b = 0; b < branches; ++b) {
            if (b != prevB) { skipped = false; }
            prevB = b;
            char *look = &board[branch[b].y][branch[b].x];
            if (*look == EMPTY || *look == UN_AND || *look == PW_AND|| *look == UN_NOT || *look == PW_NOT || *look == UN_XOR || *look == PW_XOR || *look == UN_WALL || *look == PW_BIT || *look == PW_DELAY) { continue; }
            uint8_t *dir = &branch[b].d;
            if (*look == UN_BIT && (*dir == EAST || *dir == WEST)) { continue; } //Stop at Unpowered Bit
            else if (*look == UN_WIRE)   { *look = PW_WIRE; }    //Electrify wire
            else if (*look == UN_H_WIRE) { *look = PW_H_WIRE; }  //Electrify H Wire
            else if (*look == UN_V_WIRE) { *look = PW_V_WIRE; }  //Electrify V Wire
            else if (*look == UN_BRIDGE || *look == PW_BRIDGE) { //If we've ended up in a bridge, continue in the direction we were going
                *look = PW_BRIDGE; //Electrify
                switch (*dir) {
                    case NODIR: continue; //We never had a direction (Bridge may have been placed on top of Power)
                    case NORTH: --branch[b].y; break;
                    case EAST: ++branch[b].x; break;
                    case SOUTH: ++branch[b].y; break;
                    case WEST: --branch[b].x; break;
                }
                --b;
                skipped = true;
                continue;
            }
            else if (*look == UN_N_DIODE || *look == PW_N_DIODE) { //Upon an N Diode, go North
                if (*dir == SOUTH) { continue; } //Trying to go past the diode
                *look = PW_N_DIODE; //Electrify
                --branch[b].y;
                *dir = NORTH; //Reset prevDir
                --b;
                skipped = true;
                continue;
            }
            else if (*look == UN_E_DIODE ||*look == PW_E_DIODE) { //Upon an E Diode, go East
                if (*dir == WEST) { continue; } //Trying to go past the diode
                *look = PW_E_DIODE; //Electrify
                ++branch[b].x;
                *dir = EAST; //Reset prevDir
                --b;
                skipped = true;
                continue;
            }
            else if (*look == UN_S_DIODE || *look == PW_S_DIODE) { //Upon an S Diode, go South
                if (*dir == NORTH) { continue; } //Trying to go past the diode
                *look = PW_S_DIODE; //Electrify
                ++branch[b].y;
                *dir = SOUTH; //Reset prevDir
                --b;
                skipped = true;
                continue;
            }
            else if (*look == UN_W_DIODE || *look == PW_W_DIODE) { //Upon a W Diode, go West
                if (*dir == EAST) { continue; } //Trying to go past the diode
                *look = PW_W_DIODE; //Electrify
                --branch[b].x;
                *dir = WEST; //Reset prevDir
                --b;
                skipped = true;
                continue;
            }
            else if (*look == UN_BIT) { //If we've ended up in a Bit, activate it
                *look = PW_BIT;
                continue;
            }
            else if (*look == UN_DELAY) { //If we've ended up in a Delay, activate it (if we didn't skip earlier (just been over a Bridge/Diode))
                if (!skipped) { *look = PW_DELAY; }
                continue;
            }
            else if (*look >= U1_STRETCH && *look <= P3_STRETCH) { //Upon a Stretcher, go South
                *look = P3_STRETCH; //Electrify
                ++branch[b].y;
                *dir = SOUTH; //Reset prevDir
                --b;
                skipped = true;
                continue;
            }
          //Wires (electric branches)
            bool canNorth = false, canEast = false, canSouth = false, canWest = false, canDoubleNorth = false, canDoubleEast = false, canDoubleSouth = false, canDoubleWest = false;
            uint8_t routes = 0;
            char ourPos = board[branch[b].y][branch[b].x];
            bool canV = (ourPos != UN_H_WIRE && ourPos != PW_H_WIRE);
            bool canH = (ourPos != UN_V_WIRE && ourPos != PW_V_WIRE);
            if (canV && *dir != SOUTH && branch[b].y > 0)      { //North
                char *look = &board[branch[b].y - 1][branch[b].x];
                if (*look == UN_WIRE || *look == UN_N_DIODE)       { ++routes; canNorth = true; }
                else if (*look == UN_V_WIRE)                       { ++routes; canNorth = true; }
                else if (*look == UN_BRIDGE || *look == PW_BRIDGE) { ++routes; canDoubleNorth = true; *look = PW_BRIDGE; }
            }
            if (canV && *dir != NORTH && branch[b].y < boardH) { //South
                char *look = &board[branch[b].y + 1][branch[b].x];
                if (*look == UN_WIRE || *look == UN_S_DIODE || *look == UN_BIT || *look == U1_STRETCH) { ++routes; canSouth = true; }
                else if (*look == UN_V_WIRE)                       { ++routes; canSouth = true; }
                else if (*look == UN_BRIDGE || *look == PW_BRIDGE) { ++routes; canDoubleSouth = true; *look = PW_BRIDGE; }
            }
            if (canH && *dir != WEST && branch[b].x < boardW) { //East
                char *look = &board[branch[b].y][branch[b].x + 1];
                if (*look == UN_WIRE || *look == UN_E_DIODE)       { ++routes; canEast = true; }
                else if (*look == UN_H_WIRE)                       { ++routes; canEast = true; }
                else if (*look == UN_BRIDGE || *look == PW_BRIDGE) { ++routes; canDoubleEast = true; *look = PW_BRIDGE; }
            }
            if (canH && *dir != EAST && branch[b].x > 0)      { //West
                char *look = &board[branch[b].y][branch[b].x - 1];
                if (*look == UN_WIRE || *look == UN_W_DIODE)       { ++routes; canWest = true; }
                else if (*look == UN_H_WIRE)                       { ++routes; canWest = true; }
                else if (*look == UN_BRIDGE || *look == PW_BRIDGE) { ++routes; canDoubleWest = true; *look = PW_BRIDGE; }
            }

            if (routes == 0) {
                continue;
            } else {
                moved = true;
                do {
                    if (routes == 1) { nextB = b; } else { nextB = addBranch(branch[b].y, branch[b].x, NORTH); }
                  //Move new branch onto next wire
                         if (canNorth)       { --branch[nextB].y;       branch[nextB].d = NORTH;    canNorth = false;       }
                    else if (canEast)        { ++branch[nextB].x;       branch[nextB].d = EAST;     canEast = false;        }
                    else if (canSouth)       { ++branch[nextB].y;       branch[nextB].d = SOUTH;    canSouth = false;       }
                    else if (canWest)        { --branch[nextB].x;       branch[nextB].d = WEST;     canWest = false;        }
                  //Step over bridge
                    else if (canDoubleNorth) { branch[nextB].y -= 2;    branch[nextB].d = NORTH;    canDoubleNorth = false; skipped = true; }
                    else if (canDoubleEast)  { branch[nextB].x += 2;    branch[nextB].d = EAST;     canDoubleEast = false;  skipped = true; }
                    else if (canDoubleSouth) { branch[nextB].y += 2;    branch[nextB].d = SOUTH;    canDoubleSouth = false; skipped = true; }
                    else if (canDoubleWest)  { branch[nextB].x -= 2;    branch[nextB].d = WEST;     canDoubleWest = false;  skipped = true; }
                    --routes;
                } while (routes > 0);
            }
        }
    }
    memset(branch, 0, sizeof(branch)); //Remove all existing branches
    branches = 0;
  //Components
    for (int y = elecY2; y >= elecY; --y) { //Evaluate upwards, so recently changed components aren't re-evaluated
        for (int x = elecX; x <= elecX2; ++x) {
            bool unpowerDelay = true; //Unpower potential Delay below us?
            switch (board[y][x]) {
                case UN_WIRE: case UN_V_WIRE: case UN_BRIDGE: //Unpowered Wire/V Wire/Delay/Bridge
                   //Just here to unpower a Delay
                   break;
                case UN_AND: //AND
                case PW_AND: //Powered AND
                {
                  //Check if we're in a line of AND's, and if they are currently being activated
                    bool leftPresent = false, leftAlive = false, rightPresent = false, rightAlive = false;
                  //Seek horiz RIGHT across a potential line of AND's
                    uint32_t tx = x + 1;
                    while (tx < boardW && board[y][tx] == UN_AND || board[y][tx] == PW_AND) {
                        ++tx;
                    }
                    rightAlive   = powerAtDir(y, tx, EAST); //Is the line powered?
                    rightPresent = powerAtDir(y, tx, EAST, true) || rightAlive; //Is there something which could invalidate the AND (e.g. an off wire)?
                  //Seek horiz LEFT across a potential line of AND's
                    tx = x - 1;
                    while (tx > 0 && board[y][tx] == UN_AND || board[y][tx] == PW_AND) {
                        --tx;
                    }
                    leftAlive   = powerAtDir(y, tx, WEST); //Is the line powered?
                    leftPresent = powerAtDir(y, tx, WEST, true) || leftAlive; //Is there something which could invalidate the AND (e.g. an off wire)?
                  //Check AND conditions
                    board[y][x] = UN_AND;
                    if ((leftPresent || rightPresent) && (leftPresent ? leftAlive : true) && (rightPresent ? rightAlive : true)) { //Are our flanks completely powered?
                        if (powerAtDir(y - 1, x, NORTH) || (!powerAtDir(y - 1, x, NORTH, true) && leftAlive && rightAlive)) { //Are we powered from above; or is there nothing dead above us, and we have both flanks available?
                            board[y][x] = PW_AND;
                            addBranch(y + 1, x, SOUTH);
                            unpowerDelay = false;
                        }
                    }
                    break;
                }
                case UN_NOT: //NOT
                case PW_NOT: //Powered NOT
                    if (!nextToLives(y, x, 2)) {
                        board[y][x] = UN_NOT;
                        addBranch(y + 1, x, SOUTH);
                        unpowerDelay = false;
                    } else {
                        board[y][x] = PW_NOT;
                    }
                    break;
                case UN_XOR: //XOR
                case PW_XOR: //Powered XOR
                    if (nextToLives(y, x, 3) == 1) {
                        board[y][x] = PW_XOR;
                        addBranch(y + 1, x, SOUTH);
                        unpowerDelay = false;
                    } else {
                        board[y][x] = UN_XOR;
                    }
                    break;
                case PW_POWER: //Power
                    addBranch(y, x, NODIR);
                    unpowerDelay = false;
                    break;
                case PW_N_DIODE: //Powered North Diode
                    if (y - 2 < boardH && (board[y - 1][x] == UN_E_DIODE || board[y - 1][x] == PW_E_DIODE || board[y - 1][x] == UN_W_DIODE || board[y - 1][x] == PW_W_DIODE) && (board[y - 2][x] == UN_N_DIODE || board[y - 2][x] == PW_N_DIODE)) { //If there's another North Diode yonder a horizontal Diode, skip to it
                        addBranch(y - 2, x, NORTH);
                    }
                    break;
                case PW_S_DIODE: //Powered South Diode
                    if (y + 2 < boardH && (board[y + 1][x] == UN_E_DIODE || board[y + 1][x] == PW_E_DIODE || board[y + 1][x] == UN_W_DIODE || board[y + 1][x] == PW_W_DIODE) && (board[y + 2][x] == UN_S_DIODE || board[y + 2][x] == PW_S_DIODE)) { //If there's another South Diode yonder a horizontal Diode, skip to it
                        addBranch(y + 2, x, SOUTH);
                    }
                    unpowerDelay = false;
                    break;
                case PW_BIT: //Powered Bit
                {
                  //Check if being reset
                  //Seek horiz RIGHT across a potential line of Bits
                    uint32_t tx = x;
                    while (tx < boardW && board[y][tx] == UN_BIT || board[y][tx] == PW_BIT) {
                        ++tx;
                    }
                    if (powerAtDir(y, tx, EAST)) {
                        board[y][x] = UN_BIT;
                    } else {
                      //Seek horiz LEFT across a potential line of Bits
                        uint32_t tx = x;
                        while (tx > 0 && board[y][tx] == UN_BIT || board[y][tx] == PW_BIT) {
                            --tx;
                        }
                        if (powerAtDir(y, tx, WEST)) {
                            board[y][x] = UN_BIT;
                        } else {
                            if (board[y][x] == PW_BIT) { //If already set
                                addBranch(y + 1, x, SOUTH);
                                unpowerDelay = false;
                            }
                        }
                    }
                    break;
                }
                case UN_DELAY: //Delay
                  //Check if we should be powered
                    if (powerAtDir(y - 1, x, NORTH)) {
                        board[y][x] = PW_DELAY;
                        if (y < boardH && board[y + 1][x] != UN_DELAY) {
                            addBranch(y + 1, x, SOUTH);
                        }
                        unpowerDelay = false;
                    }
                    break;
                case PW_DELAY: //Powered Delay
                  //Check if we should be unpowered
                    if (powerAtDir(y - 1, x, NORTH, true)) {
                        board[y][x] = UN_DELAY;
                    } else {
                        addBranch(y + 1, x, SOUTH);
                    }
                    unpowerDelay = false;
                    break;
                case P1_STRETCH: //
                case P2_STRETCH: //
                case P3_STRETCH: // Powered Stretcher
                  //Check if we should be unpowered
                    if (!powerAtDir(y - 1, x, NORTH)) {
                        --board[y][x];
                    }
                    if (board[y][x] > P1_STRETCH) {
                        addBranch(y + 1, x, SOUTH);
                        unpowerDelay = false;
                    }
                    break;
                default:
                  //There's nothing here which could unpower a Delay
                    unpowerDelay = false;
                  //switches
                    if (board[y][x] >= 50 && board[y][x] <= 59) {
                        if (switches[board[y][x] - 50]) {
                            addBranch(y, x, NODIR);
                            unpowerDelay = false;
                        }
                    }
                    break;
            }
            if (unpowerDelay) {
                if (y + 1 < boardH && board[y + 1][x] == PW_DELAY) { //If there's a Powered Delay beneath us, make it unpowered
                    board[y + 1][x] = UN_DELAY;
                }
            }
        }
    }
}



bool inputted;
std::string getInput (std::string defau = "")
{
    std::string toReturn = defau;
    std::cout << toReturn;
    fflush(stdout);
    while (!inputted) {
        while (kbhit()) {
            pressedCh = getchar(); //Get the key
            std::cout << pressedCh; //Echo it to the user
            if (pressedCh == '\n') {
                inputted = true;
            } else {
                if (pressedCh == 127 && toReturn.length()) {
                    std::cout << "\b \b";
                    toReturn = toReturn.substr(0, toReturn.length() - 1);
                } else {
                    toReturn += pressedCh;
                }
            }
            fflush(stdout);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(64));
    }
    inputted = false;
  //Trim
    while (toReturn.length() > 1 && toReturn.substr(toReturn.length() - 2, toReturn.length() - 1) == " ") { toReturn.substr(0, toReturn.length() - 2); }
    return toReturn;
}


uint32_t startLabelX; //Label's X
char lDirY, lDirX, prevZ, prevMove, prevMoveCount, elecCounter;
uint32_t prevX, prevY;
int main ()
{
  //Load shite to listen to pressed keys
    loadKeyListen();
    std::cout << "Patrick Bowen @phunanon 2016\n"
              << "\nINSTRUCTIONS"
              << "\n[space]\t\tremove anything"
              << "\n.oeu\t\tup, left, down, right"
              << "\n>OEU\t\tpage up, page left, page down, page right"
              << "\nhH\t\tplace wire, toggle auto-bridge for wires and diodes"
              << "\na\t\ttoggle general use wire/directional wire"
              << "\nfF\t\tcrosshairs, goto coord"
              << "\n0-9\t\tplace/toggle switch"
              << "\ngcGCrl\t\tNorth/South/West/East diodes, bridge, switch"
              << "\ndDtns\t\tdelay, stretcher, AND, NOT, XOR"
              << "\nb\t\tplace bit"
              << "\nPpiI\t\tpause, next, slow-motion, fast-motion"
              << "\n;\t\twall"
              << "\nyY\t\tload, save"
              << "\n'\t\ttoggle [lowercase] label mode"
              << "\nzv\t\tundo, redo"
              << "\nxbBkKjJmw\tinitiate/complete/discard selection, paste, paste unelectrified, or move, or swap, or clear area, or paste mask, or paste x flip, or paste y flip"
              << "\nq\t\tquit"
              << std::endl;
    while (!kbhit()) { std::this_thread::sleep_for(std::chrono::milliseconds(50)); }
    std::cout << "Loading board..." << std::endl;
    memset(board, 0, sizeof(board[0][0]) * boardH * boardW); //Set the board Empty
    std::cout << "Board loaded." << std::endl;
    lDirY = 1;
    std::cout << "Loading complete." << std::endl;

    while (run) {
        display();
        if (!paused) {
            if (elecCounter >= (slowMo ? 10 : 1)) { elecCounter = 0; elec(); }
            if (fastMo) { for (uint32_t i = 0; i < 8; ++i) { elec(); } }
            ++elecCounter;
        }
        while (kbhit()) {
            bool toMove = false; //Should we auto-move after pressing?
            char *look = &board[cursorY][cursorX];
            prevY = cursorY;
            prevX = cursorX;
            prevZ = *look;
            pressedCh = getchar(); //Get the key
            if (labelMode && pressedCh >= 97 && pressedCh <= 122) { //Was this a label (and lowercase)?
                *look = pressedCh;
                if (cursorX + 1 < boardW) { ++cursorX; }
                lDirY = lDirX = 0;
                lDirX = 1;
                prevMove = EAST;
            } else {
                bool elecReCalc = false;
                uint8_t moveDist = (prevMoveCount == UNTILHYPER ? HYPER : 1);
                switch (pressedCh) {
                    case '>': //Page up
                        moveDist = screenH / 2;
                    case '.': //Up
                        if (cursorY - moveDist >= 0) { cursorY -= moveDist; }
                        lDirY = lDirX = 0;
                        lDirY = -1;
                        prevMove = NORTH;
                        break;
                    case 'U': //Page right
                        moveDist = screenW / 2;
                    case 'u': //Right
                        if (cursorX + moveDist < boardW) { cursorX += moveDist; }
                        lDirY = lDirX = 0;
                        lDirX = 1;
                        prevMove = EAST;
                        break;
                    case 'E': //Page down
                        moveDist = screenH / 2;
                    case 'e': //Down
                        if (cursorY + moveDist < boardH) { cursorY += moveDist; }
                        lDirY = lDirX = 0;
                        lDirY = 1;
                        prevMove = SOUTH;
                        break;
                    case 'O': //Page right
                        moveDist = screenW / 2;
                    case 'o': //Left
                        if (cursorX - moveDist >= 0) { cursorX -= moveDist; }
                        lDirY = lDirX = 0;
                        lDirX = -1;
                        prevMove = WEST;
                        break;
                    case '\n':
                        if (labelMode) {
                            cursorX = startLabelX;
                            if (cursorY + 1 < boardH) { ++cursorY; }
                        }
                        break;
                    case 127: //Left-remove
                        if (cursorX > 0) { --cursorX; }
                        lDirY = lDirX = 0;
                        lDirX = -1;
                        prevMove = WEST;
                        look = &board[cursorY][cursorX];
                    case ' ': //Remove
                        if (*look == 50 + switch_num) { //If on top of a switch, remove it correctly
                            placedSwitches[switch_num] = false;
                            toMove = true;
                        }
                        *look = EMPTY;
                        toMove = (pressedCh == 127 ? false : true);
                        break;
                    case 'h': //Wire
                        if (autoBridge && (*look == UN_WIRE || *look == PW_WIRE
                                           || *look == UN_N_DIODE || *look == PW_N_DIODE
                                           || *look == UN_E_DIODE || *look == PW_E_DIODE
                                           || *look == UN_S_DIODE || *look == PW_S_DIODE
                                           || *look == UN_W_DIODE || *look == PW_W_DIODE
                                           || *look == UN_H_WIRE  || *look == PW_H_WIRE
                                           || *look == UN_V_WIRE  || *look == PW_V_WIRE
                                           || *look == UN_BRIDGE
                                          )
                           ) { //Auto-bridging
                            *look = UN_BRIDGE;
                        } else {
                            if (dirWire) {
                                if (lDirX) {
                                    *look = UN_H_WIRE;
                                }
                                else if (lDirY) {
                                    *look = UN_V_WIRE;
                                }
                                else { *look = UN_WIRE; }
                            } else {
                                *look = UN_WIRE;
                            }
                        }
                        toMove = true;
                        break;
                    case 'H': //Auto-bridge
                        autoBridge = !autoBridge;
                        break;
                    case 'a': //Toggle wire
                        dirWire = !dirWire;
                        break;
                    case 't': //AND
                        *look = UN_AND;
                        toMove = true;
                        break;
                    case 'n': //NOT
                        *look = UN_NOT;
                        toMove = true;
                        break;
                    case 's': //XOR
                        *look = UN_XOR;
                        toMove = true;
                        break;
                    case 'r': //Bridge
                        *look = UN_BRIDGE;
                        toMove = true;
                        break;
                    case 'l': //Power
                        *look = PW_POWER;
                        toMove = true;
                        break;
                    case ';': //Wall
                        *look = UN_WALL;
                        toMove = true;
                        break;
                    case 'g': //North Diode
                        if (autoBridge && (*look == UN_WIRE || *look == PW_WIRE
                                           || *look == UN_E_DIODE || *look == PW_E_DIODE
                                           || *look == UN_W_DIODE || *look == PW_W_DIODE
                                           || *look == UN_H_WIRE  || *look == PW_H_WIRE
                                           || *look == UN_V_WIRE  || *look == PW_V_WIRE
                                           || *look == UN_BRIDGE
                                          )
                           ) { //Auto-bridging
                            *look = UN_BRIDGE;
                        } else {
                            *look = UN_N_DIODE;
                        }
                        toMove = true;
                        break;
                    case 'C': //East diode
                        if (autoBridge && (*look == UN_WIRE || *look == PW_WIRE
                                           || *look == UN_S_DIODE || *look == PW_S_DIODE
                                           || *look == UN_N_DIODE || *look == PW_N_DIODE
                                           || *look == UN_H_WIRE  || *look == PW_H_WIRE
                                           || *look == UN_V_WIRE  || *look == PW_V_WIRE
                                           || *look == UN_BRIDGE
                                          )
                           ) { //Auto-bridging
                            *look = UN_BRIDGE;
                        } else {
                            *look = UN_E_DIODE;
                        }
                        toMove = true;
                        break;
                    case 'c': //South Diode
                        if (autoBridge && (*look == UN_WIRE || *look == PW_WIRE
                                           || *look == UN_E_DIODE || *look == PW_E_DIODE
                                           || *look == UN_W_DIODE || *look == PW_W_DIODE
                                           || *look == UN_H_WIRE  || *look == PW_H_WIRE
                                           || *look == UN_V_WIRE  || *look == PW_V_WIRE
                                           || *look == UN_BRIDGE
                                          )
                           ) { //Auto-bridging
                            *look = UN_BRIDGE;
                        } else {
                            *look = UN_S_DIODE;
                        }
                        toMove = true;
                        break;
                    case 'G': //West diode
                        if (autoBridge && (*look == UN_WIRE || *look == PW_WIRE
                                           || *look == UN_S_DIODE || *look == PW_S_DIODE
                                           || *look == UN_N_DIODE || *look == PW_N_DIODE
                                           || *look == UN_H_WIRE  || *look == PW_H_WIRE
                                           || *look == UN_V_WIRE  || *look == PW_V_WIRE
                                           || *look == UN_BRIDGE
                                          )
                           ) { //Auto-bridging
                            *look = UN_BRIDGE;
                        } else {
                            *look = UN_W_DIODE;
                        }
                        toMove = true;
                        break;
                    case 'd': //Delay
                        *look = UN_DELAY;
                        toMove = true;
                        break;
                    case 'D': //Stretcher
                        *look = U1_STRETCH;
                        toMove = true;
                        break;
                    case 'z': //Undo
                        if (u == 0) { u = UNDOS; }
                        --u;
                        board[undos[u][0]][undos[u][1]] = undos[u][2];
                        ++canr;
                        break;
                    case 'v': //Redo
                        if (u == UNDOS) { u = 0; }
                        if (canr > 0) {
                            --canr;
                            board[undos[u][0]][undos[u][1]] = undos[u][3];
                            ++u;
                        }
                        break;
                    case 'x': //Copy
                        if (copying = !copying) { //Are we not copying?
                            if (dataToPaste) { //Did we have something to paste?
                              //If so, clear it
                                copying = false;
                                copyData->clear();
                                dataToPaste = false;
                            } else {
                              //Set copy
                                copyY = cursorY++;
                                copyX = cursorX++;
                              //'hide' last pasted
                                pasteX = pasteX2 = -1;
                                pasteY = pasteY2 = -1;
                            }
                        } else { //We were copying
                          //Perform the copy
                            copyY2 = cursorY;
                            copyX2 = cursorX;
                            copyYDist = (copyY2 - copyY);
                            copyXDist = (copyX2 - copyX);
                            copyData->clear();
                            for (int y = copyY; y < copyY2; ++y)
                            {
                                for (int x = copyX; x < copyX2; ++x)
                                {
                                    copyData->push_back(board[y][x]);
                                }
                            }
                            dataToPaste = true;
                            cursorY = copyY;
                            cursorX = copyX;
                        }
                        break;
                    case 'b': //Paste -or- Bit
                        if (!dataToPaste) {
                            *look = UN_BIT;
                            toMove = true;
                            break;
                        }
                    case 'B': //Paste unelectrified
                    case 'm': //Paste x-flip
                    case 'w': //Paste y-flip
                    case 'k': //Cut
                    case 'K': //Swap
                    case 'j': //Clear area
                    case 'J': //Paste mask
                        if (dataToPaste) {
                            if (pressedCh == 'k' || pressedCh == 'j') { //Remove copied-from area (move/clear)?
                                for (int y = copyY; y < copyY2; ++y) {
                                    for (int x = copyX; x < copyX2; ++x) {
                                        board[y][x] = EMPTY;
                                    }
                                }
                            }
                            int cYend, cXend;
                            cYend = cursorY + copyYDist;
                            cXend = cursorX + copyXDist;
                            if (pressedCh != 'j') { //Paste copy data onto the board
                                uint32_t i = 0;
                                if (pressedCh == 'K') { //Swap
                                    for (int y = copyY, y2 = cursorY; y < copyY2; ++y, ++y2) {
                                        for (int x = copyX, x2 = cursorX; x < copyX2; ++x, ++x2) {
                                            board[y][x] = board[y2][x2];
                                            board[y2][x2] = copyData->at(i);
                                            ++i;
                                        }
                                    }
                                    i = 0;
                                }
                                if (pressedCh == 'm') { //x-flip paste
                                    for (int y = cursorY; y < cYend; ++y) {
                                        for (int x = cXend - 1; x >= cursorX; --x) {
                                            board[y][x] = copyData->at(i);
                                            if (board[y][x] == UN_E_DIODE) { board[y][x] = UN_W_DIODE; } //E Diode to W Diode
                                            else if (board[y][x] == UN_W_DIODE) { board[y][x] = UN_E_DIODE; } //W Diode to E Diode
                                            ++i;
                                        }
                                    }
                                } else if (pressedCh == 'w') { //y-flip paste
                                    for (int y = cYend - 1; y >= cursorY; --y) {
                                        for (int x = cursorX; x < cXend; ++x) {
                                            board[y][x] = copyData->at(i);
                                            if (board[y][x] == UN_S_DIODE) { board[y][x] = UN_N_DIODE; } //S Diode to N Diode
                                            else if (board[y][x] == UN_N_DIODE) { board[y][x] = UN_S_DIODE; } //N Diode to S Diode
                                            ++i;
                                        }
                                    }
                                } else { //Other paste
                                    if (pressedCh == 'B') { //Unelectrified
                                        for (int y = cursorY; y < cYend; ++y) {
                                            for (int x = cursorX; x < cXend; ++x) {
                                                uint8_t c = copyData->at(i);
                                                switch (c) {
                                                    case P3_STRETCH: case P2_STRETCH: case P1_STRETCH: c = U1_STRETCH; break;
                                                    case PW_WIRE: case PW_BRIDGE: case PW_BIT: case PW_N_DIODE: case PW_DELAY: case PW_E_DIODE: case PW_S_DIODE: case PW_W_DIODE: case PW_H_WIRE: case PW_V_WIRE:
                                                        --c;
                                                        break;
                                                }
                                                board[y][x] = c;
                                                ++i;
                                            }
                                        }
                                    } else { //Unmodified
                                        if (pressedCh == 'J') { //Paste mask (' ' is not pasted)
                                            for (int y = cursorY; y < cYend; ++y) {
                                                for (int x = cursorX; x < cXend; ++x) {
                                                    char c = copyData->at(i);
                                                    if (c) { board[y][x] = c; }
                                                    ++i;
                                                }
                                            }
                                        } else { //Normal, full paste
                                            for (int y = cursorY; y < cYend; ++y) {
                                                for (int x = cursorX; x < cXend; ++x) {
                                                    board[y][x] = copyData->at(i);
                                                    ++i;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            if (pressedCh == 'k' || pressedCh == 'j' || pressedCh == 'K') { //Was remove copied-from area (cut/clear) or have we finished (swap)?
                                copyData->clear();
                                dataToPaste = false;
                            }
                          //Set last pasted area markers
                            pasteY  = cursorY;
                            pasteX  = cursorX;
                            pasteY2 = cYend;
                            pasteX2 = cXend;
                          //Re-calculate the electrification area
                          //Use heuristic
                            if (pasteY < elecY) { elecY = pasteY; }
                            else if (pasteY2 - 1 > elecY2) { elecY2 = pasteY2 - 1; }
                            if (pasteX < elecX) { elecX = pasteX; }
                            else if (pasteX2 - 1 > elecX2) { elecX2 = pasteX2 - 1; }
                        }
                        break;
                    case 'q': //Quit
                    {
                        std::cout << "\033[0;37;40mAre you sure you want to quit (y/N)?" << std::endl;
                        while (!kbhit()) { std::this_thread::sleep_for(std::chrono::milliseconds(50)); }
                        char sure = getchar();
                        if (sure == 'y') {
                            std::cout << "Quit.\033[0m" << std::endl;
                            run = false;
                        }
                    }
                        break;
                    case 'Y': //Save
                    {
                        std::cout << "\033[0;37;40mSave name: ";
                        std::string save = getInput(projName);
                        if (save.length()) {
                            std::cout << "Confirm? (y/N)" << std::endl;
                            char sure = getchar();
                            if (sure == 'y') {
                                projName = save;
                                std::cout << "Saving..." << std::endl;
                                system(("rm " + save + ".gz &> /dev/null").c_str());
                                std::string saveData = "";
                                for (int y = elecY; y <= elecY2; ++y) {
                                    for (int x = elecX; x <= elecX2; ++x) {
                                        switch (board[y][x]) {
                                            case EMPTY:                          saveData += ' '; break;
                                            case UN_WIRE: case PW_WIRE:          saveData += '#'; break;
                                            case UN_AND: case PW_AND:            saveData += 'A'; break;
                                            case UN_NOT: case PW_NOT:            saveData += 'N'; break;
                                            case UN_XOR: case PW_XOR:            saveData += 'X'; break;
                                            case UN_BRIDGE: case PW_BRIDGE:      saveData += '+'; break;
                                            case PW_POWER:                       saveData += '@'; break;
                                            case UN_WALL:                        saveData += ';'; break;
                                            case UN_BIT: case PW_BIT:            saveData += 'B'; break;
                                            case UN_N_DIODE: case PW_N_DIODE:    saveData += '^'; break;
                                            case UN_E_DIODE: case PW_E_DIODE:    saveData += '>'; break;
                                            case UN_S_DIODE: case PW_S_DIODE:    saveData += 'V'; break;
                                            case UN_DELAY: case PW_DELAY:        saveData += '%'; break;
                                            case U1_STRETCH: case P1_STRETCH: case P2_STRETCH: case P3_STRETCH: saveData += '$'; break;
                                            case UN_W_DIODE: case PW_W_DIODE:    saveData += '<'; break;
                                            case UN_H_WIRE: case PW_H_WIRE:      saveData += '-'; break;
                                            case UN_V_WIRE: case PW_V_WIRE:      saveData += '|'; break;
                                        }
                                        if (board[y][x] >= 50 && board[y][x] <= 59) { //Is switch?
                                            saveData += board[y][x] - 2;
                                        } else if (board[y][x] >= 97 && board[y][x] <= 122) { //Is label?
                                            saveData += board[y][x];
                                        }
                                    }
                                    saveData += '\n';
                                }
                                std::ofstream out(save.c_str());
                                out << saveData;
                                out.close();
                                std::cout << "Compressing..." << std::endl;
                                system((SYSPATH + "gzip " + save).c_str());
                            }
                        }
                    }
                        break;
                    case 'y': //Load
                    {
                        std::cout << "\033[0;37;40mLoad name: ";
                        std::string load = getInput();
                        if (load.length()) {
                            std::cout << "Confirm? (y/N)" << std::endl;
                            char sure = getchar();
                            if (sure == 'y') {
                                std::cout << "Decompressing..." << std::endl;
                                system(("cp " + load + ".gz load.gz").c_str());
                                system((SYSPATH + "gzip -d load.gz").c_str());
                                std::string saveData = "";
                                std::ifstream in("load");
                                saveData = std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
                                in.close();
                                system("rm load");
                                std::cout << "Loading..." << std::endl;
                                memset(board, 0, sizeof(board[0][0]) * boardH * boardW); //Set the board Empty
                                projName = load;
                                uint64_t len = saveData.length();
                                if (len > 0) {
                                    uint8_t saveChar;
                                    int y = 0, x = 0, maxX = 0;
                                    for (uint32_t i = 0; i < len; ++i) {
                                        if (saveData[i] == '\n') {
                                            ++y;
                                            maxX = x;
                                            x = 0;
                                        } else {
                                            saveChar = EMPTY;
                                            switch (saveData[i])
                                            {
                                                case ' ': break; //Empty
                                                case '#': saveChar = UN_WIRE;    break;
                                                case 'A': saveChar = UN_AND;     break;
                                                case 'N': saveChar = UN_NOT;     break;
                                                case 'X': saveChar = UN_XOR;     break;
                                                case '+': saveChar = UN_BRIDGE;  break;
                                                case '@': saveChar = PW_POWER;   break;
                                                case 'V': saveChar = UN_S_DIODE; break;
                                                case ';': saveChar = UN_WALL;    break;
                                                case 'B': saveChar = UN_BIT;     break;
                                                case '^': saveChar = UN_N_DIODE; break;
                                                case '%': saveChar = UN_DELAY;   break;
                                                case '$': saveChar = U1_STRETCH; break;
                                                case '>': saveChar = UN_E_DIODE; break;
                                                case '<': saveChar = UN_W_DIODE; break;
                                                case '-': saveChar = UN_H_WIRE;  break;
                                                case '|': saveChar = UN_V_WIRE;  break;
                                            }
                                            if (saveData[i] >= 48 && saveData[i] <= 57) //Is switch?
                                            {
                                                switch_num = saveData[i] - 48;
                                                saveChar = switch_num + 50;
                                                switches[switch_num] = false; //Set its power
                                                placedSwitches[switch_num] = true; //Say it's placed
                                            } else if (saveData[i] >= 97 && saveData[i] <= 122) { //Is label?
                                                saveChar = saveData[i];
                                            }
                                            board[y][x] = saveChar;
                                            ++x;
                                        }
                                    }
                                    elecY = 0; elecX = 0;
                                    elecY2 = y - 1; elecX2 = maxX - 1;
                                  //Turn off all Switches
                                    for (uint32_t i = 0; i < 10; ++i) {
                                        switches[i] = false;
                                    }
                                  //Remove all previous electricity
                                    memset(branch, 0, sizeof(branch));
                                    branches = 0;
                                  //Move cursor
                                    cursorY = screenHh;
                                    cursorX = screenWh;
                                }
                            }
                        }
                    }
                        break;
                    case 'F': //GOTO
                    {
                        std::cout << "Enter x coord: ";
                        std::string xcoord = getInput();
                        std::cout << "Enter y coord: ";
                        std::string ycoord = getInput();
                        cursorY = atoi(ycoord.c_str());
                        cursorX = atoi(xcoord.c_str());
                    }
                        break;
                    case 'f': //Crosshairs
                        crosshairs = !crosshairs;
                        break;
                    case 'p': //Next elec
                        elec();
                        break;
                    case 'P': //Toggle pause
                        paused = !paused;
                        break;
                    case 'i': //Slow-mo
                        slowMo = !slowMo;
                        fastMo = false;
                        break;
                    case 'I': //Fast-mo
                        fastMo = !fastMo;
                        slowMo = false;
                        break;
                    case '\'': //Label mode
                        labelMode = !labelMode;
                        startLabelX = cursorX;
                        break;
                }
                if (pressedCh >= 48 && pressedCh <= 57) { //Power button
                    switch_num = pressedCh - 48;
                    if (placedSwitches[switch_num]) { //Is the switch already placed? Power it
                        bool found = false;
                        for (int y = 0; y < boardH; ++y)
                        {
                            for (int x = 0; x < boardW; ++x)
                            {
                                if (board[y][x] == 50 + switch_num) { found = true; }
                            }
                        }
                        if (found) { //But was it reaaaaaaally placed? It could have been overwrote
                            switches[switch_num] = !switches[switch_num];
                        } else {
                            placedSwitches[switch_num] = false; //Mark it as non-existant
                        }
                    } else { //No? Place it
                        *look = 50 + switch_num;
                        switches[switch_num] = false;
                        placedSwitches[switch_num] = true;
                        toMove = true;
                    }
                }
              //Re-calculate the electrification area
                if (toMove) { //Use heuristic
                    int y = cursorY;
                    int x = cursorX;
                    if (y < elecY) { elecY = y; }
                    else if (y > elecY2) { elecY2 = y; }
                    if (x < elecX) { elecX = x; }
                    else if (x > elecX2) { elecX2 = x; }
                }
                if (elecReCalc) { //Do full calculate
                    elecY = boardH;
                    elecY2 = 0;
                    elecX = boardW;
                    elecX2 = 0;
                    for (int y = 0; y < boardH; ++y) {
                        for (int x = 0; x < boardW; ++x) {
                            if (board[y][x]) {
                                if (y < elecY) { elecY = y; }
                                else if (y > elecY2) { elecY2 = y; }
                                if (x < elecX) { elecX = x; }
                                else if (x > elecX2) { elecX2 = x; }
                            }
                        }
                    }
                }
            }
            if (toMove) { //We placed something, so lets move out of the way
                if (labelMode) {
                    ++cursorX;
                } else {
                    cursorY += lDirY;
                    cursorX += lDirX;
                }
                //This was probably an action requiring an undo, so let's record it
                undos[u][0] = prevY;
                undos[u][1] = prevX;
                undos[u][2] = prevZ;
                undos[u][3] = board[prevY][prevX];
                canr = 0;
                ++u;
                if (u == UNDOS) { u = 0; }
            }
          //Bounds checking
            if (lDirY != 0 || lDirX != 0) { //The user made a move
              //General bounds check
                if (cursorY < 0) { cursorY = 0; }
                if (cursorX < 0) { cursorX = 0; }
                if (cursorY > boardH) { cursorY = boardH; }
                if (cursorX > boardW) { cursorX = boardW; }
              //Copy/paste bounds check
                if (copying && (cursorY < copyY || cursorX < copyX)) //Is the user trying to go out of copying bounds while copying?
                {
                    cursorY = copyY;
                    cursorX = copyX;
                }
              //See if we've moved enough in one direction to make the user move twice as fast
                if      (prevMove == NORTH && lDirY == -1) { ++prevMoveCount; }
                else if (prevMove == EAST && lDirX == 1)  { ++prevMoveCount; }
                else if (prevMove == SOUTH && lDirY == 1)  { ++prevMoveCount; }
                else if (prevMove == WEST && lDirX == -1) { ++prevMoveCount; }
                else { prevMoveCount = 0; }
                if (prevMoveCount > UNTILHYPER) { prevMoveCount = UNTILHYPER; }
            }
        }
      //Sleep
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        prevMoveCount -= (prevMoveCount > 0);
    }
    return 0;
}

//std::cout << ": " + std::to_string() << std::endl;
