//TODO
//Move variables into local scope
//Break into multiple .cpp & .hpp
//Don't allow movement out of bounds when pasting
//Make bikeyboardal
//Ensure paste text fits on screen at all sizes
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
#include "keypresses.c" //For detecting keypresses: kbhit (), pressed_ch

const std::string SYSPATH = ""; //"/system/xbin"; //For BusyBox on Android

std::string proj_name = "untitled";
bool run = true;

const uint32_t board_W = 4096;
const uint32_t board_H = 4096;
char board[board_W][board_H];
const uint8_t MOVE_FAR = 8;
int32_t cursor_X = MOVE_FAR*2;
int32_t cursor_Y = MOVE_FAR;
bool to_elec_cursor = false;
bool to_copy_cursor = false;

uint8_t switches[10];
bool placed_switches[10];
uint8_t switch_num;

int32_t elec_X = board_W, elec_Y = board_H, elec_X2 = 0, elec_Y2 = 0; //Electric bounds (swapped to induce tight first electrification)

const uint32_t UNDOS = 512;
uint32_t undos[UNDOS][4]; //[u][x, y, was, now]
uint32_t can_redo = 0;
uint32_t u = 0;

bool is_copying, is_data_to_paste;
int32_t copy_X, copy_Y, copy_X2, copy_Y2, paste_X_dist, paste_Y_dist;
int32_t paste_X, paste_Y, paste_X2, paste_Y2;
std::vector<char> *copy_data = new std::vector<char>();
bool is_no_copy_source; //Data was from storage, not project

bool is_auto_bridge = false;
bool is_dir_wire = false;
bool is_slow_mo = false;
bool is_fast_mo = false;
bool paused = false;
bool is_label_mode = false; //Are we typing a label?

uint32_t screen_W = 60; //The width of the screen
uint32_t screen_H = 20; //The height of the screen
uint32_t screen_W_half = screen_W / 2; //Half of the width of the screen
uint32_t screen_H_half = screen_H / 2; //Half of the height of the screen


void clearScreen () { std::cout << "\033[2J\033[1;1H"; }

int32_t board_crop_X;
int32_t board_crop_Y;
uint32_t board_crop_X2;
uint32_t board_crop_Y2;
void calcBoardCrops () //For calculating the part of the board on the screen
{
    board_crop_X = cursor_X - screen_W_half;
    if (board_crop_X < 0) { board_crop_X = 0; }
    board_crop_X2 = board_crop_X + screen_W;
    if (board_crop_X2 > board_W) { board_crop_X = board_W - screen_W; board_crop_X2 = board_W; }
    board_crop_Y = cursor_Y - screen_H_half;
    if (board_crop_Y < 0) { board_crop_Y = 0; }
    board_crop_Y2 = board_crop_Y + screen_H;
    if (board_crop_Y2 > board_H) { board_crop_Y = board_H - screen_H; board_crop_Y2 = board_H; }
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
    return dim;
}
void resizeScreen ()
{
    auto dim = getTerminalDimensions();
    screen_W = (dim.first < board_W ? dim.first : board_W);
    screen_H = (dim.second < board_H ? dim.second: board_H) - 1;
    screen_W_half = screen_W / 2;
    screen_H_half = screen_H / 2;
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
#define E_WALL      10
#define UN_S_DIODE  11
#define PW_S_DIODE  12
#define UN_BIT      13
#define PW_BIT      14
#define UN_N_DIODE  15
#define PW_N_DIODE  16
#define UN_E_DIODE  17
#define PW_E_DIODE  18
#define UN_W_DIODE  19
#define PW_W_DIODE  20
#define U1_STRETCH  21
#define P1_STRETCH  22
#define P2_STRETCH  23
#define P3_STRETCH  24
#define UN_DELAY    25
#define PW_DELAY    26
#define UN_H_WIRE   27
#define PW_H_WIRE   28
#define UN_V_WIRE   29
#define PW_V_WIRE   30
#define PW_AND      31
#define PW_NOT      32
#define PW_XOR      33
#define UN_LEAKYB   34
#define PW_LEAKYB   35
#define NOTHING     36

bool to_crosshairs = false;
void display ()
{
    resizeScreen();
    calcBoardCrops();
    std::string buffer;
    std::string buff;
    buffer = "";
    clearScreen();
  //Top bar
    uint32_t bar_off_len = 0;
    buffer += "\033[0;37;40m"; //bg of bar
    bar_off_len += 10;
    buffer += std::to_string(cursor_X) + ", " + std::to_string(cursor_Y);
    buffer += " ";
    for (uint32_t i = 0; i < 10; ++i) {
        buffer += "\033[37;40m" + (switches[i] ? "\033[30;42m" + std::to_string(i) : (placed_switches[i] ? "\033[30;43m" + std::to_string(i) : "\033[37;40m" + std::to_string(i)));
        bar_off_len += 16;
    }
    buffer += "\033[37;40m";
    bar_off_len += 8;
    buffer += (!is_label_mode && !is_copying ? "  " + std::string(is_dir_wire ? "direct" : "genperp") + " wire" : ""); 
    buffer += (is_copying ? "  selecting (x complete)" : (is_data_to_paste ?
        (is_no_copy_source ?
            "  ready (x dismiss, b paste, B unelec, m x-flip, w y-flip, J mask)" : "  ready (x dismiss, b paste, B unelec, k cut, K swap, j clear, m x-flip, w y-flip, J mask, Y save)")
        : ""));
    buffer += (is_auto_bridge ? "  auto bridging" : "");
    buffer += (is_slow_mo ? "  slow-mo" : (is_fast_mo ? "  fast-mo" : ""));
    buffer += (paused ? "  paused" : "");
    buffer += (is_label_mode ? "  label mode" : "");
    std::string space = "";
    int32_t s_len = screen_W - (buffer.length() - bar_off_len) - proj_name.length();
    for (int32_t i = 0; i < s_len; ++i) { space += " "; }
    buffer += space + "\033[1;4m" + proj_name + "\033[0m";
  //Board
    int32_t sx, sy;
    std::string prev_colour = "";
    for (int32_t y = board_crop_Y, sy = 0; y < board_crop_Y2; ++y, ++sy) {
        buffer += '\n';
        for (int32_t x = board_crop_X, sx = 0; x < board_crop_X2; ++x, ++sx) {
            buff = " ";
            char *look = &board[x][y];
            std::string colour = "\033[";
            switch (*look) {
                case EMPTY: //Empty
                    colour += "0;30;47";
                    buff = (x >= elec_X && x < elec_X2 + 1 && y >= elec_Y && y < elec_Y2 + 1 ? " " : ".");
                    break;
                case UN_WIRE: //Unpowered Wire
                    colour += "0;30;47";
                    buff = "#";
                    break;
                case PW_WIRE: //Powered Wire
                    colour += "0;30;42";
                    buff = "#";
                    break;
                case UN_AND: //AND
                    colour += "0;37;43";
                    buff = "A";
                    break;
                case PW_AND: //Powered AND
                    colour += "1;37;43";
                    buff = "A";
                    break;
                case UN_NOT: //NOT
                    colour += "0;37;41";
                    buff = "N";
                    break;
                case PW_NOT: //Powered NOT
                    colour += "1;37;41";
                    buff = "N";
                    break;
                case UN_XOR: //XOR
                    colour += "0;37;45";
                    buff = "X";
                    break;
                case PW_XOR: //Powered XOR
                    colour += "1;37;45";
                    buff = "X";
                    break;
                case UN_BRIDGE: //Bridge
                    colour += "0;30;47";
                    buff = "+";
                    break;
                case PW_BRIDGE: //Powered Bridge
                    colour += "0;30;42";
                    buff = "+";
                    break;
                case UN_LEAKYB: //Leaky Bridge
                    colour += "0;37;46";
                    buff = "+";
                    break;
                case PW_LEAKYB: //Powered Leaky Bridge
                    colour += "1;37;46";
                    buff = "+";
                    break;
                case PW_POWER: //Power
                    colour += "1;37;44";
                    buff = "@";
                    break;
                case UN_WALL: //Wall
                    colour += "1;30;40";
                    buff = ";";
                    break;
                case E_WALL: //Conductive Wall
                    colour += "1;30;40";
                    buff = ":";
                    break;
                case UN_BIT: //Bit
                    colour += "1;30;47";
                    buff = "B";
                    break;
                case PW_BIT: //Powered Bit
                    colour += "1;37;44";
                    buff = "B";
                    break;
                case UN_N_DIODE: //North Diode
                    colour += "0;30;47";
                    buff = "^";
                    break;
                case PW_N_DIODE: //Powered North Diode
                    colour += "0;30;42";
                    buff = "^";
                    break;
                case UN_E_DIODE: //East Diode
                    colour += "0;30;47";
                    buff = ">";
                    break;
                case PW_E_DIODE: //Powered East Diode
                    colour += "0;30;42";
                    buff = ">";
                    break;
                case UN_S_DIODE: //South Diode
                    colour += "0;30;47";
                    buff = "V";
                    break;
                case PW_S_DIODE: //Powered South Diode
                    colour += "0;30;42";
                    buff = "V";
                    break;
                case UN_W_DIODE: //West Diode
                    colour += "0;30;47";
                    buff = "<";
                    break;
                case PW_W_DIODE: //Powered West Diode
                    colour += "0;30;42";
                    buff = "<";
                    break;
                case UN_DELAY: //Delay
                    colour += "1;30;47";
                    buff = "%";
                    break;
                case PW_DELAY: //Powered Delay
                    colour += "1;30;42";
                    buff = "%";
                    break;
                case U1_STRETCH:
                case P1_STRETCH:
                    colour += "1;30;47";
                    buff = "$"; //Stretcher
                    break;
                case P2_STRETCH:
                case P3_STRETCH:
                    colour += "1;30;42";
                    buff = "$"; //Powered Stretcher
                    break;
                case UN_H_WIRE:
                    colour += "0;30;47";
                    buff = "-"; //Horizontal Wire
                    break;
                case PW_H_WIRE:
                    colour += "0;30;42";
                    buff = "-"; //Powered Horizontal Wire
                    break;
                case UN_V_WIRE:
                    colour += "0;30;47";
                    buff = "|"; //Vertical Wire
                    break;
                case PW_V_WIRE:
                    colour += "0;30;42";
                    buff = "|"; //Powered Vertical Wire
                    break;
            }

            colour += "m";
            if (prev_colour != colour) { buff = colour + buff; }
            prev_colour = colour;
            
            if (*look >= 50 && *look <= 59) { //Power
                switch_num = *look - 50;
                buff = (switches[switch_num] ? "\033[1;33;44m" + std::to_string(switch_num) : "\033[1;31;47m" + std::to_string(switch_num)) ; //Power switch
                prev_colour = "";
            }
            
            if (*look >= 97 && *look <= 122) { //Label
                buff = "\033[4;1;34;47m" + std::string(look, look + 1);
                prev_colour = "";
            }

            if (is_copying) { //Copy box
                if (y == copy_Y - 1 && x >= copy_X - 1 && x <= cursor_X)    { buff = "\033[1;30;46mx"; } //North
                if (x == cursor_X && y >= copy_Y - 1 && y <= cursor_Y)      { buff = "\033[1;30;46mx"; } //East
                if (y == cursor_Y && x >= copy_X - 1 && x <= cursor_X)      { buff = "\033[1;30;46mx"; } //South
                if (x == copy_X - 1 && y >= copy_Y - 1 && y <= cursor_Y)    { buff = "\033[1;30;46mx"; } //West
                prev_colour = "";
            }
            if (is_data_to_paste) {
              //Copy markers
                if ((x == copy_X && y == copy_Y) || (x == copy_X2 - 1 && y == copy_Y) || (x == copy_X && y == copy_Y2 - 1) || (x == copy_X2 - 1 && y == copy_Y2 - 1)) {
                    buff = "\033[30;46m" + buff.substr(buff.length() - 1, buff.length());
                }
              //Pasted markers
                else if ((x == paste_X && y == paste_Y) || (x == paste_X2 - 1 && y == paste_Y) || (x == paste_X && y == paste_Y2 - 1) || (x == paste_X2 - 1 && y == paste_Y2 - 1)) {
                    buff = "\033[30;43m" + buff.substr(buff.length() - 1, buff.length());
                }
              //Paste area
                else if (x >= cursor_X && x < cursor_X + paste_X_dist && y >= cursor_Y && y < cursor_Y + paste_Y_dist) {
                    buff = "\033[30;4"+ std::string(to_copy_cursor ? "0" : "6") +"m" + buff.substr(buff.length() - 1, buff.length());
                }
                prev_colour = "";
            }
          //Crosshairs and cursor
            if ((to_crosshairs && (x == cursor_X || y == cursor_Y)) || (x == cursor_X && y == cursor_Y) || (x == cursor_X && sy == 0) || (x == cursor_X && sy == screen_H - 1) || (sx == 0 && y == cursor_Y) || (sx == screen_W - 1 && y == cursor_Y)) {
                buff = buff.substr(buff.length() - 1, buff.length());
                buff = "\033[0;37;4" + std::string(is_data_to_paste ? (to_copy_cursor ? "0" : "6") : (to_elec_cursor ? "4" : "0")) + "m" + buff;
                prev_colour = "";
            }

            buffer += buff;
        }
    }
    std::cout << buffer;
    fflush(stdout);
    to_copy_cursor = false;
}



void elecReCalculate ()
{
    elec_X = board_W;
    elec_X2 = 0;
    elec_Y = board_H;
    elec_Y2 = 0;
    for (int32_t x = 0; x < board_W; ++x) {
        for (int32_t y = 0; y < board_H; ++y) {
            if (board[x][y]) {
                if (x < elec_X) { elec_X = x; }
                else if (x > elec_X2) { elec_X2 = x; }
                if (y < elec_Y) { elec_Y = y; }
                else if (y > elec_Y2) { elec_Y2 = y; }
            }
        }
    }
}



bool powerAtDir (int32_t _X, int32_t _Y, uint8_t _dir, bool _is_dead = false)
{
    if (_X < 0 || _Y < 0 || _X >= board_W || _Y >= board_H) { return false; }
    char look = board[_X][_Y];
    uint8_t diode;
    uint8_t wire;
    char xd = 0;
    char yd = 0;
    switch (_dir) {
        case NORTH: diode = PW_S_DIODE; wire = PW_V_WIRE; yd = -1; break; //North: Powered S Diode, V Wire
        case EAST:  diode = PW_W_DIODE; wire = PW_H_WIRE; xd = 1;  break; //East:  Powered W Diode, H Wire
        case SOUTH: diode = PW_N_DIODE; wire = PW_V_WIRE; yd = 1;  break; //South: Powered N Diode, V Wire
        case WEST:  diode = PW_E_DIODE; wire = PW_H_WIRE; xd = -1; break; //West:  Powered E Diode, H Wire
    }
  //Check for conditions
    if (look == PW_BRIDGE || look == PW_LEAKYB) { //Powered Bridge/Powered Leaky Bridge
      //Check this Powered Bridge is powered in this direction
        _X += xd;
        _Y += yd;
        while (board[_X][_Y] == PW_BRIDGE || board[_X][_Y] == PW_LEAKYB) { //Seek along the Bridge
            _X += xd;
            _Y += yd;
        }
        //Reached the end of the bridges
    }
    look = board[_X][_Y];
    bool is_power_present = false, is_powered = false;
    if (look >= 50 && look <= 59) {
        is_power_present = true;
        is_powered = switches[look - 50];
    }
  //Return for if checking for dead
    if (_is_dead) {
        --diode;
        --wire;
        if (is_power_present) { return !is_powered; }  //There's a Switch
        if (look == PW_POWER) { return false; }       //There's a Power
        if (look == UN_WIRE                             //
         || look == UN_BRIDGE || look == UN_LEAKYB      //
         || look == wire                                //
         || look == diode                               //
         || look == UN_BIT                              //
         || (_dir == NORTH ? look == UN_DELAY : false) //
         || look == U1_STRETCH                          // There's a dead Wire/Bridge/LeakyB/D Wire/Diode/Bit/Delay/Stretcher
           ) { return true; }
        return false; //Anything else could never power us
    }
  //Return for if checking for alive
    return look == PW_WIRE                      //
        || look == wire                         //
        || look == PW_BIT                       //
        || look == PW_POWER                     //
        || look == diode                        // Powered by Wire/D Wire/Bit/Power/Diode
        || (_dir == NORTH ? look == PW_DELAY    //
                        || look == P1_STRETCH   //
                        || look == P2_STRETCH   //
                        || look == P3_STRETCH   //
           : false)                             // Powered by Delay/Stretcher
        || (is_power_present && is_powered);    // Power
}

//TODO Fix bounds checking
uint8_t nextToLives (int32_t _X, int32_t _Y, uint8_t mode) //mode: 0 AND, 1 OR, 2 NOT, 3 XOR, 4 Stretcher
{
    uint8_t lives = 0;
  //Check North
    if      (mode == 0 && powerAtDir(_X, _Y - 1, NORTH, true)) { return 0; }
    else if (powerAtDir(_X, _Y - 1, NORTH)) {
        ++lives;
    }
  //Check East
    if      (mode == 0 && powerAtDir(_X + 1, _Y, EAST, true)) { return 0; }
    else if (powerAtDir(_X + 1, _Y, EAST)) {
        ++lives;
    }
  //Check West
    if      (mode == 0 && powerAtDir(_X - 1, _Y, WEST, true)) { return 0; }
    else if (powerAtDir(_X - 1, _Y, WEST)) {
        ++lives;
    }
  //Return
    return lives;
}

uint8_t betweenLives (int32_t _X, int32_t _Y)
{
    uint8_t lives = 0;
  //Check East
    if (powerAtDir(_X + 1, _Y, EAST)) {
        ++lives;
    }
  //Check West
    if (powerAtDir(_X - 1, _Y, WEST)) {
        ++lives;
    }
  //Return
    return lives;
}

struct Branch
{
    int32_t x, y;
    uint8_t d;
};

Branch branch[4096];
uint32_t branches = 0;
int32_t addBranch (int32_t _X, int32_t _Y, uint8_t prev_dir)
{
    branch[branches].x = _X;
    branch[branches].y = _Y;
    branch[branches].d = prev_dir; //0: none, NESW
    return branches++;
}

uint32_t next_branch;
void elec () //Electrify the board appropriately
{
  //Reset the electrified
    for (int32_t x = elec_X; x <= elec_X2; ++x) {
        for (int32_t y = elec_Y; y <= elec_Y2; ++y) {
            char *look = &board[x][y];
            if      (*look == PW_WIRE)    { *look = UN_WIRE; }    //Powered Wire to Wire
            else if (*look == PW_H_WIRE)  { *look = UN_H_WIRE;  } //Powered H Wire to H Wire
            else if (*look == PW_V_WIRE)  { *look = UN_V_WIRE;  } //Powered V Wire to V Wire
            else if (*look == PW_BRIDGE)  { *look = UN_BRIDGE;  } //Powered Bridge to Bridge
            else if (*look == PW_LEAKYB)  { *look = UN_LEAKYB;  } //Powered Leaky Bridge to Leaky Bridge
            else if (*look == PW_N_DIODE) { *look = UN_N_DIODE; } //Powered N Diode to N Diode
            else if (*look == PW_E_DIODE) { *look = UN_E_DIODE; } //Powered E Diode to E Diode
            else if (*look == PW_S_DIODE) { *look = UN_S_DIODE; } //Powered S Diode to S Diode
            else if (*look == PW_W_DIODE) { *look = UN_W_DIODE; } //Powered W Diode to W Diode
        }
    }
  //Electrify cursor?
    if (to_elec_cursor) {
        to_elec_cursor = false;
        addBranch(cursor_X, cursor_Y, 0);
    }
  //Wires & inline components
    bool moved = true;
    bool skipped = false;
    while (moved) {
    
        moved = false;
        uint32_t prev_branch = 0;
        
        for (uint32_t b = 0; b < branches; ++b) {
            bool can_north = false, can_east = false, can_south = false, can_west = false, can_double_north = false, can_double_east = false, can_double_south = false, can_double_west = false;
            uint8_t routes = 0;
            
            if (b != prev_branch) { skipped = false; }
            prev_branch = b;
            char *look = &board[branch[b].x][branch[b].y];
            if (*look == EMPTY || *look == UN_AND || *look == PW_AND|| *look == UN_NOT || *look == PW_NOT || *look == UN_XOR || *look == PW_XOR || *look == UN_WALL || *look == PW_BIT || *look == PW_DELAY) { continue; }
            uint8_t *dir = &branch[b].d;
            bool is_bridge = (*look == UN_BRIDGE || *look == PW_BRIDGE || *look == UN_LEAKYB || *look == PW_LEAKYB);
            if (*look == UN_BIT && (*dir == EAST || *dir == WEST)) { continue; } //Stop at Unpowered Bit
            else if (*look == UN_WIRE)   { *look = PW_WIRE; }    //Electrify Wire
            else if (*look == UN_H_WIRE) { *look = PW_H_WIRE; }  //Electrify H Wire
            else if (*look == UN_V_WIRE) { *look = PW_V_WIRE; }  //Electrify V Wire
            else if (*look == E_WALL || is_bridge) { //If we've ended up in a Bridge, or Conductive Wall, continue in the direction we were going
              //Electrify
                if (*look == UN_BRIDGE) { *look = PW_BRIDGE; }
                if (*look == UN_LEAKYB) { *look = PW_LEAKYB; }
              //Handle branch direction
                switch (*dir) {
                    case NODIR: continue; //We never had a direction (Bridge may have been placed on top of Power)
                    case NORTH: --branch[b].y; break;
                    case EAST:  ++branch[b].x; break;
                    case SOUTH: ++branch[b].y; break;
                    case WEST:  --branch[b].x; break;
                }
                skipped = true;
                --b;
                continue;
            }
            else if (*look == UN_N_DIODE || *look == PW_N_DIODE) { //Upon an N Diode, go North
                if (*dir == SOUTH) { continue; } //Trying to go past the diode
                *look = PW_N_DIODE; //Electrify
                --branch[b].y;
                *dir = NORTH; //Reset prev_dir
                --b;
                skipped = true;
                continue;
            }
            else if (*look == UN_E_DIODE ||*look == PW_E_DIODE) { //Upon an E Diode, go East
                if (*dir == WEST) { continue; } //Trying to go past the diode
                *look = PW_E_DIODE; //Electrify
                ++branch[b].x;
                *dir = EAST; //Reset prev_dir
                --b;
                skipped = true;
                continue;
            }
            else if (*look == UN_S_DIODE || *look == PW_S_DIODE) { //Upon an S Diode, go South
                if (*dir == NORTH) { continue; } //Trying to go past the diode
                *look = PW_S_DIODE; //Electrify
                ++branch[b].y;
                *dir = SOUTH; //Reset prev_dir
                --b;
                skipped = true;
                continue;
            }
            else if (*look == UN_W_DIODE || *look == PW_W_DIODE) { //Upon a W Diode, go West
                if (*dir == EAST) { continue; } //Trying to go past the diode
                *look = PW_W_DIODE; //Electrify
                --branch[b].x;
                *dir = WEST; //Reset prev_dir
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
                *dir = SOUTH; //Reset prev_dir
                --b;
                skipped = true;
                continue;
            }
          //Wires (electric branches)
            char our_pos = board[branch[b].x][branch[b].y];
            bool can_V = (our_pos != UN_H_WIRE && our_pos != PW_H_WIRE);
            bool can_H = (our_pos != UN_V_WIRE && our_pos != PW_V_WIRE);
            bool is_leaky = false;
            if (can_V && *dir != SOUTH && branch[b].y > 0)      { //North
                char *look = &board[branch[b].x][branch[b].y - 1];
                if (*look == UN_WIRE || *look == UN_N_DIODE)        { ++routes; can_north = true; }
                else if (*look == UN_V_WIRE)                       { ++routes; can_north = true; }
                else if (*look == UN_BRIDGE || *look == PW_BRIDGE) { ++routes; can_double_north = true; *look = PW_BRIDGE; }
                else if (*look == UN_LEAKYB || *look == PW_LEAKYB) { ++routes; can_double_north = true; *look = PW_LEAKYB; }
            }
            if (can_H && *dir != WEST && branch[b].x < board_W) { //East
                char *look = &board[branch[b].x + 1][branch[b].y];
                if (*look == UN_WIRE || *look == UN_E_DIODE)        { ++routes; can_east = true; }
                else if (*look == UN_H_WIRE)                       { ++routes; can_east = true; }
                else if (*look == UN_BRIDGE || *look == PW_BRIDGE) { ++routes; can_double_east = true; *look = PW_BRIDGE; }
                else if (*look == UN_LEAKYB || *look == PW_LEAKYB) { ++routes; can_double_east = true; *look = PW_LEAKYB; is_leaky = true; }
            }
            if (can_V && *dir != NORTH && branch[b].y < board_H) { //South
                char *look = &board[branch[b].x][branch[b].y + 1];
                if (*look == UN_WIRE || *look == UN_S_DIODE || *look == UN_BIT || *look == U1_STRETCH) { ++routes; can_south = true; }
                else if (*look == UN_V_WIRE)                       { ++routes; can_south = true; }
                else if (*look == UN_BRIDGE || *look == PW_BRIDGE) { ++routes; can_double_south = true; *look = PW_BRIDGE; }
                else if (*look == UN_LEAKYB || *look == PW_LEAKYB) { ++routes; can_double_south = true; *look = PW_LEAKYB; }
            }
            if (can_H && *dir != EAST && branch[b].x > 0)      { //West
                char *look = &board[branch[b].x - 1][branch[b].y];
                if (*look == UN_WIRE || *look == UN_W_DIODE)        { ++routes; can_west = true; }
                else if (*look == UN_H_WIRE)                       { ++routes; can_west = true; }
                else if (*look == UN_BRIDGE || *look == PW_BRIDGE) { ++routes; can_double_west = true; *look = PW_BRIDGE; }
                else if (*look == UN_LEAKYB || *look == PW_LEAKYB) { ++routes; can_double_west = true; *look = PW_LEAKYB; is_leaky = true; }
            }

            if (routes == 0) {
                continue;
            } else {
                moved = true;
                do {
                    if (routes == 1) { next_branch = b; } else { next_branch = addBranch(branch[b].x, branch[b].y, NORTH); }
                  //Move new branch onto next wire
                         if (can_north)       { --branch[next_branch].y;       branch[next_branch].d = NORTH;    can_north = false;       }
                    else if (can_east)        { ++branch[next_branch].x;       branch[next_branch].d = EAST;     can_east = false;        }
                    else if (can_south)       { ++branch[next_branch].y;       branch[next_branch].d = SOUTH;    can_south = false;       }
                    else if (can_west)        { --branch[next_branch].x;       branch[next_branch].d = WEST;     can_west = false;        }
                  //Step over bridge
                    else if (can_double_north) { branch[next_branch].y -= 2;    branch[next_branch].d = NORTH;    can_double_north = false; skipped = true; }
                    else if (can_double_east)  { branch[next_branch].x += 2;    branch[next_branch].d = EAST;     can_double_east = false;  skipped = true; }
                    else if (can_double_south) { branch[next_branch].y += 2;    branch[next_branch].d = SOUTH;    can_double_south = false; skipped = true; }
                    else if (can_double_west)  { branch[next_branch].x -= 2;    branch[next_branch].d = WEST;     can_double_west = false;  skipped = true; }
                    --routes;
                } while (routes > 0);
            }
        }
    }
    memset(branch, 0, sizeof(branch)); //Remove all existing branches
    branches = 0;
  //Components
    for (int32_t x = elec_X; x <= elec_X2; ++x) {
        for (int32_t y = elec_Y2; y >= elec_Y; --y) { //Evaluate upwards, so recently changed components aren't re-evaluated
            bool to_unpower_delay = true; //Unpower potential Delay below us?
            switch (board[x][y]) {
                case UN_WIRE: case UN_V_WIRE: case UN_BRIDGE: case UN_LEAKYB: //Wire/V Wire/Delay/Bridge/LeakyB
                    //Just here to unpower a Delay
                    break;
                case UN_AND: //AND
                case PW_AND: //Powered AND
                {
                  //Check if we're in a line of AND's, and if they are currently being activated
                    bool is_left_present = false, is_left_alive = false, is_right_present = false, is_right_alive = false;
                  //Seek horiz RIGHT across a potential line of AND's
                    uint32_t tx = x + 1;
                    while (tx < board_W && board[tx][y] == UN_AND || board[tx][y] == PW_AND) {
                        ++tx;
                    }
                    is_right_alive   = powerAtDir(tx, y, EAST); //Is the line powered?
                    is_right_present = powerAtDir(tx, y, EAST, true) || is_right_alive; //Is there something which could invalidate the AND (e.g. an off wire)?
                  //Seek horiz LEFT across a potential line of AND's
                    tx = x - 1;
                    while (tx > 0 && board[tx][y] == UN_AND || board[tx][y] == PW_AND) {
                        --tx;
                    }
                    is_left_alive   = powerAtDir(tx, y, WEST); //Is the line powered?
                    is_left_present = powerAtDir(tx, y, WEST, true) || is_left_alive; //Is there something which could invalidate the AND (e.g. an off wire)?
                  //Check AND conditions
                    board[x][y] = UN_AND;
                    if ((is_left_present || is_right_present) && (is_left_present ? is_left_alive : true) && (is_right_present ? is_right_alive : true)) { //Are our flanks completely powered?
                        if (powerAtDir(x, y - 1, NORTH) || (!powerAtDir(x, y - 1, NORTH, true) && is_left_alive && is_right_alive)) { //Are we powered from above; or is there nothing dead above us, and we have both flanks available?
                            board[x][y] = PW_AND;
                            addBranch(x, y + 1, SOUTH);
                            to_unpower_delay = false;
                        }
                    }
                    break;
                }
                case UN_NOT: //NOT
                case PW_NOT: //Powered NOT
                    if (!nextToLives(x, y, 2)) {
                        board[x][y] = UN_NOT;
                        addBranch(x, y + 1, SOUTH);
                        to_unpower_delay = false;
                    } else {
                        board[x][y] = PW_NOT;
                    }
                    break;
                case UN_XOR: //XOR
                case PW_XOR: //Powered XOR
                    if (nextToLives(x, y, 3) == 1) {
                        board[x][y] = PW_XOR;
                        addBranch(x, y + 1, SOUTH);
                        to_unpower_delay = false;
                    } else {
                        board[x][y] = UN_XOR;
                    }
                    break;
                case PW_LEAKYB: //Powered Leaky Bridge
                    addBranch(x, y + 1, SOUTH);
                    break;
                case PW_POWER: //Power
                    addBranch(x, y, NODIR);
                    to_unpower_delay = false;
                    break;
                case PW_N_DIODE: //Powered North Diode
                    if (y - 2 < board_H && (board[x][y - 1] == UN_E_DIODE || board[x][y - 1] == PW_E_DIODE || board[x][y - 1] == UN_W_DIODE || board[x][y - 1] == PW_W_DIODE) && (board[x][y - 2] == UN_N_DIODE || board[x][y - 2] == PW_N_DIODE)) { //If there's another North Diode yonder a horizontal Diode, skip to it
                        addBranch(x, y - 2, NORTH);
                    }
                    break;
                case PW_S_DIODE: //Powered South Diode
                    if (y + 2 < board_H && (board[x][y + 1] == UN_E_DIODE || board[x][y + 1] == PW_E_DIODE || board[x][y + 1] == UN_W_DIODE || board[x][y + 1] == PW_W_DIODE) && (board[x][y + 2] == UN_S_DIODE || board[x][y + 2] == PW_S_DIODE)) { //If there's another South Diode yonder a horizontal Diode, skip to it
                        addBranch(x, y + 2, SOUTH);
                    }
                    to_unpower_delay = false;
                    break;
                case PW_BIT: //Powered Bit
                {
                  //Check if being reset
                  //Seek horiz RIGHT across a potential line of Bits
                    uint32_t tx = x;
                    while (tx < board_W && board[tx][y] == UN_BIT || board[tx][y] == PW_BIT) {
                        ++tx;
                    }
                    if (powerAtDir(tx, y, EAST)) {
                        board[x][y] = UN_BIT;
                    } else {
                      //Seek horiz LEFT across a potential line of Bits
                        uint32_t tx = x;
                        while (tx > 0 && board[tx][y] == UN_BIT || board[tx][y] == PW_BIT) {
                            --tx;
                        }
                        if (powerAtDir(tx, y, WEST)) {
                            board[x][y] = UN_BIT;
                        } else {
                            if (board[x][y] == PW_BIT) { //If already set
                                addBranch(x, y + 1, SOUTH);
                                to_unpower_delay = false;
                            }
                        }
                    }
                    break;
                }
                case UN_DELAY: //Delay
                  //Check if we should be powered
                    if (powerAtDir(x, y - 1, NORTH)) {
                        board[x][y] = PW_DELAY;
                        if (board[x][y + 1] != UN_DELAY && y < board_H) {
                            addBranch(x, y + 1, SOUTH);
                        }
                        to_unpower_delay = false;
                    }
                    break;
                case PW_DELAY: //Powered Delay
                  //Check if we should be unpowered
                    if (powerAtDir(x, y - 1, NORTH, true)) {
                        board[x][y] = UN_DELAY;
                    } else {
                        addBranch(x, y + 1, SOUTH);
                    }
                    to_unpower_delay = false;
                    break;
                case P1_STRETCH: //
                case P2_STRETCH: //
                case P3_STRETCH: // Powered Stretcher
                  //Check if we should be unpowered
                    if (!powerAtDir(x, y - 1, NORTH)) {
                        --board[x][y];
                    }
                    if (board[x][y] > P1_STRETCH) {
                        addBranch(x, y + 1, SOUTH);
                        to_unpower_delay = false;
                    }
                    break;
                default:
                  //There's nothing here which could unpower a Delay
                    to_unpower_delay = false;
                  //Switches
                    if (board[x][y] >= 50 && board[x][y] <= 59) {
                        if (switches[board[x][y] - 50]) {
                            addBranch(x, y, NODIR);
                            to_unpower_delay = false;
                        }
                    }
                    break;
            }
            if (to_unpower_delay) {
                if (board[x][y + 1] == PW_DELAY && y + 1 < board_H) { //If there's a powered Delay beneath us, make it unpowered
                    board[x][y + 1] = UN_DELAY;
                }
            }
        }
    }
}



/**
 * Execute a command and get the result.
 * https://stackoverflow.com/a/3578548/7000138
 * @param   cmd - The system command to run.
 * @return  The string command line output of the command.
 */
std::string getStdoutFromCmd(std::string cmd)
{

    std::string data;
    FILE * stream;
    const int max_buffer = 256;
    char buffer[max_buffer];
    cmd.append(" 2>&1"); // Do we want STDERR?

    stream = popen(cmd.c_str(), "r");
    if (stream) {
        while (!feof(stream))
            if (fgets(buffer, max_buffer, stream) != NULL) data.append(buffer);
        pclose(stream);
    }
    return data;
}

std::string autoCompleteForFile (std::string input)
{
    std::string find_result = getStdoutFromCmd(std::string("find . -type f -iname \""+ input +"*.gz\" -exec basename {} .gz ';'").c_str());

  //Seperate lines returned (stackoverflow.com/a/8448359/7000138)
    std::vector<std::string> lines;
    char* c_lines = (char*)find_result.c_str();
    char seps[] = "\n";
    char *token = strtok(c_lines, seps);
    while (token != NULL) {
        lines.push_back(token);
        token = strtok(NULL, seps);
    }

  //Find initial agreement in results compared to input
    uint16_t agreement = lines.size();
    bool is_agreed[agreement];
    for (uint16_t i = 0; i < agreement; ++i) { is_agreed[i] = true; }
    for (uint16_t c = 0, clen = input.size(); c < clen; ++c) {
        for (uint16_t l = 0, llen = lines.size(); l < llen; ++l) {
            if (is_agreed[l] && (lines[l].length() <= c || lines[l].at(c) != input.at(c))) {
                is_agreed[l] = false;
                --agreement;
            }
        }
    }
    if (!agreement) { return input; } //There are no files starting with that text
  //Find consensus in the results for look-ahead
    std::string consensus = input;
    std::string proposal = consensus;
    bool is_agreeing = true;
    while (is_agreeing) {
        consensus = proposal;
        for (uint16_t a = 0, alen = lines.size(); a < alen; ++a) {
            if (is_agreed[a]) {
                if (proposal == consensus) { //Create the first proposal?
                    if (lines[a].length() <= consensus.length()) {
                        is_agreeing = false;
                        break;
                    }
                    proposal += lines[a].at(consensus.length());
                } else {
                    if (lines[a].length() < proposal.length() || proposal != lines[a].substr(0, proposal.length())) {
                        is_agreeing = false;
                        break;
                    }
                }
            }
        }
    }
    return consensus;
}



bool inputted;
std::string getInput (std::string _pretext = "", bool to_autocomplete = true)
{
    std::string to_return = _pretext;
    std::cout << to_return;
    fflush(stdout);
    while (!inputted) {
        while (kbhit()) {
            pressed_ch = getchar(); //Get the key
            if (pressed_ch == '\n') { //Done
                inputted = true;
            } else if (pressed_ch == 27) { //Escape
                to_return = "";
                inputted = true;
            } else if (pressed_ch == 9 && to_autocomplete) { //Autocomplete
                std::string auto_completed = autoCompleteForFile(to_return);
                std::string auto_completed_end = auto_completed.substr(to_return.length(), auto_completed.length() - to_return.length());
                std::cout << auto_completed_end;
                to_return = auto_completed;
            } else {
              //Clear previous autocomplete
                for (uint8_t i = 0; i < 32; ++i) { std::cout << " "; }
                for (uint8_t i = 0; i < 32; ++i) { std::cout << "\b"; }
              //Handle backspace, else normal input
                if (pressed_ch == 127) {
                    if (to_return.length()) {
                        std::cout << "\b \b";
                        to_return = to_return.substr(0, to_return.length() - 1);
                    }
                } else {
                    to_return += pressed_ch;
                    std::cout << pressed_ch; //Echo it to the user
                }
                if (to_autocomplete) {
                  //Echo autocomplete results
                    std::string auto_completed = autoCompleteForFile(to_return);
                    std::string auto_completed_end = auto_completed.substr(to_return.length(), auto_completed.length() - to_return.length());
                    std::cout << "\033[1;30;40m" << auto_completed_end;
                    for (uint16_t i = 0, ilen = auto_completed_end.length(); i < ilen; ++i) { std::cout << "\b"; }
                    std::cout << "\033[0;37;40m";
                }
            }
            fflush(stdout);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(64));
    }
    inputted = false;
  //Trim
    while (to_return.length() > 1 && to_return.substr(to_return.length() - 2, to_return.length() - 1) == " ") { to_return.substr(0, to_return.length() - 2); }
    return to_return;
}


uint32_t start_label_X; //Label's X
char prev_dir_X, prev_dir_Y, prev_Z, elec_counter;
uint32_t prev_X, prev_Y;
int32_t main ()
{
  //Load shite to listen to pressed keys
    loadKeyListen();
    std::cout << "Patrick Bowen @phunanon 2017\n"
              << "\nINSTRUCTIONS"
              << "\n[space]\t\tremove anything"
              << "\n[enter]\t\telectrify cursor"
              << "\n.oeu\t\tup, left, down, right"
              << "\n>OEU\t\tfar up, far left, far down, far right"
              << "\nhH\t\tplace wire, toggle auto-bridge for wires and diodes"
              << "\na\t\ttoggle general use wire/directional wire"
              << "\nL\t\tfar lay under cursor"
              << "\nfF\t\tcrosshairs, go-to coord"
              << "\n0-9\t\tplace/toggle switch"
              << "\ngcGC\t\tNorth/South/West/East diodes"
              << "\nrRl\t\tbridge, leaky bridge, power"
              << "\ndDtns\t\tdelay, stretcher, AND, NOT, XOR"
              << "\nb\t\tplace bit"
              << "\nPpiI\t\tpause, next, slow-motion, fast-motion"
              << "\n;\t\twall"
              << "\nyY\t\tload, save"
              << "\nv\t\timport component"
              << "\n'\t\ttoggle [lowercase] label mode"
              << "\nzZ\t\tundo, redo"
              << "\nxbBkKjJmw\tinitiate/complete/discard selection, paste, paste unelectrified, or move, or swap, or clear area, or paste mask, or paste x flip, or paste y flip"
              << "\nq\t\tquit"
              << "\n\nAfter initiating a selection, you can do a Save to export that component.\n\n\nElectronic tick is 1/10s (normal), 1s (slow), 1/80s (fast)"
              << std::endl;
    while (!kbhit()) { std::this_thread::sleep_for(std::chrono::milliseconds(50)); }
    std::cout << "Loading board..." << std::endl;
    memset(board, 0, sizeof(board[0][0]) * board_H * board_W); //Set the board Empty
    std::cout << "Board loaded." << std::endl;
    prev_dir_Y = 1;
    std::cout << "Loading complete." << std::endl;


    auto clock = std::chrono::system_clock::now();

    while (run) {
        display();
        if (!paused) {
            if (elec_counter >= (is_slow_mo ? 10 : 1)) { elec_counter = 0; elec(); }
            if (is_fast_mo) { for (uint32_t i = 0; i < 8; ++i) { elec(); } }
            ++elec_counter;
        }
        while (kbhit()) {
            bool to_move = false; //Should we auto-move after pressing?
            bool to_recalc = false; //Should we recalculate electrification area using a heuristic?
            char *look = &board[cursor_X][cursor_Y];
            prev_X = cursor_X;
            prev_Y = cursor_Y;
            prev_Z = *look;
            pressed_ch = getchar(); //Get the key
            if (is_label_mode && pressed_ch >= 97 && pressed_ch <= 122) { //Was this a label (and lowercase)?
                *look = pressed_ch;
                if (cursor_X + 1 < board_W) { ++cursor_X; }
                prev_dir_Y = prev_dir_X = 0;
                prev_dir_X = 1;
            } else {
                uint8_t move_dist = 1;
                switch (pressed_ch) {

                    case '>': //Far up
                        move_dist = (is_data_to_paste ? paste_Y_dist : MOVE_FAR);
                    case '.': //Up
                        if (cursor_Y - move_dist >= 0) { cursor_Y -= move_dist; }
                        prev_dir_Y = prev_dir_X = 0;
                        prev_dir_Y = -1;
                        break;

                    case 'U': //Far right
                        move_dist = (is_data_to_paste ? paste_X_dist : MOVE_FAR*2);
                    case 'u': //Right
                        if (cursor_X + move_dist < board_W) { cursor_X += move_dist; }
                        prev_dir_Y = prev_dir_X = 0;
                        prev_dir_X = 1;
                        break;

                    case 'E': //Far down
                        move_dist = (is_data_to_paste ? paste_Y_dist : MOVE_FAR);
                    case 'e': //Down
                        if (cursor_Y + move_dist < board_H) { cursor_Y += move_dist; }
                        prev_dir_Y = prev_dir_X = 0;
                        prev_dir_Y = 1;
                        break;

                    case 'O': //Far left
                        move_dist = (is_data_to_paste ? paste_X_dist : MOVE_FAR*2);
                    case 'o': //Left
                        if (cursor_X - move_dist >= 0) { cursor_X -= move_dist; }
                        prev_dir_Y = prev_dir_X = 0;
                        prev_dir_X = -1;
                        break;

                    case '\n': //New-line OR electrify cursor
                        if (is_label_mode) {
                            cursor_X = start_label_X;
                            if (cursor_Y + 1 < board_H) { ++cursor_Y; }
                        } else {
                            to_elec_cursor = true;
                        }
                        break;
                    case 127: //Left-remove
                        if (cursor_X > 0) { --cursor_X; }
                        prev_dir_Y = 0;
                        prev_dir_X = -1;
                        look = &board[cursor_X][cursor_Y];
                    case ' ': //Remove
                        if (*look == 50 + switch_num) { //If on top of a switch, remove it correctly
                            placed_switches[switch_num] = false;
                            to_move = true;
                        }
                        *look = EMPTY;
                        to_move = (pressed_ch == 127 ? false : true);
                        break;

                    case 'h': //Wire
                        if (is_auto_bridge && (*look == UN_WIRE || *look == PW_WIRE
                                           || *look == UN_N_DIODE || *look == PW_N_DIODE
                                           || *look == UN_E_DIODE || *look == PW_E_DIODE
                                           || *look == UN_S_DIODE || *look == PW_S_DIODE
                                           || *look == UN_W_DIODE || *look == PW_W_DIODE
                                           || *look == UN_H_WIRE  || *look == PW_H_WIRE
                                           || *look == UN_V_WIRE  || *look == PW_V_WIRE
                                           || *look == UN_BRIDGE  || *look == UN_LEAKYB || *look == PW_LEAKYB
                                          )
                           ) { //Auto-bridging
                            *look = UN_BRIDGE;
                        } else {
                            if (is_dir_wire) {
                                if (prev_dir_X) {
                                    *look = UN_H_WIRE;
                                }
                                else if (prev_dir_Y) {
                                    *look = UN_V_WIRE;
                                }
                                else { *look = UN_WIRE; }
                            } else {
                                *look = UN_WIRE;
                            }
                        }
                        to_move = true;
                        break;

                    case 'H': //Auto-bridge
                        is_auto_bridge = !is_auto_bridge;
                        break;

                    case 'a': //Toggle wire
                        is_dir_wire = !is_dir_wire;
                        break;

                    case 't': //AND
                        *look = UN_AND;
                        to_move = true;
                        break;

                    case 'n': //NOT
                        *look = UN_NOT;
                        to_move = true;
                        break;

                    case 's': //XOR
                        *look = UN_XOR;
                        to_move = true;
                        break;

                    case 'r': //Bridge
                        *look = UN_BRIDGE;
                        to_move = true;
                        break;

                    case 'R': //Leaky Bridge
                        *look = UN_LEAKYB;
                        to_move = true;
                        break;

                    case 'l': //Power
                        *look = PW_POWER;
                        to_move = true;
                        break;

                    case 'L': //Far lay
                        for (uint8_t i = 0; i < MOVE_FAR && ((cursor_X += prev_dir_X) | (cursor_Y += prev_dir_Y)); ++i) {
                            board[cursor_X][cursor_Y] = *look;
                        }
                        to_recalc = true;
                        break;

                    case ';': //Wall
                        *look = UN_WALL;
                        to_move = true;
                        break;

                    case ':': //Conductive Wall
                        *look = E_WALL;
                        to_move = true;
                        break;

                    case 'g': //North Diode
                        if (is_auto_bridge && (*look == UN_WIRE || *look == PW_WIRE
                                           || *look == UN_E_DIODE || *look == PW_E_DIODE
                                           || *look == UN_W_DIODE || *look == PW_W_DIODE
                                           || *look == UN_H_WIRE  || *look == PW_H_WIRE
                                           || *look == UN_V_WIRE  || *look == PW_V_WIRE
                                           || *look == UN_BRIDGE  || *look == UN_LEAKYB || *look == PW_LEAKYB
                                          )
                           ) { //Auto-bridging
                            *look = UN_BRIDGE;
                        } else {
                            *look = UN_N_DIODE;
                        }
                        to_move = true;
                        break;

                    case 'C': //East diode
                        if (is_auto_bridge && (*look == UN_WIRE || *look == PW_WIRE
                                           || *look == UN_S_DIODE || *look == PW_S_DIODE
                                           || *look == UN_N_DIODE || *look == PW_N_DIODE
                                           || *look == UN_H_WIRE  || *look == PW_H_WIRE
                                           || *look == UN_V_WIRE  || *look == PW_V_WIRE
                                           || *look == UN_BRIDGE  || *look == UN_LEAKYB || *look == PW_LEAKYB
                                          )
                           ) { //Auto-bridging
                            *look = UN_BRIDGE;
                        } else {
                            *look = UN_E_DIODE;
                        }
                        to_move = true;
                        break;

                    case 'c': //South Diode
                        if (is_auto_bridge && (*look == UN_WIRE || *look == PW_WIRE
                                           || *look == UN_E_DIODE || *look == PW_E_DIODE
                                           || *look == UN_W_DIODE || *look == PW_W_DIODE
                                           || *look == UN_H_WIRE  || *look == PW_H_WIRE
                                           || *look == UN_V_WIRE  || *look == PW_V_WIRE
                                           || *look == UN_BRIDGE  || *look == UN_LEAKYB || *look == PW_LEAKYB
                                          )
                           ) { //Auto-bridging
                            *look = UN_BRIDGE;
                        } else {
                            *look = UN_S_DIODE;
                        }
                        to_move = true;
                        break;

                    case 'G': //West diode
                        if (is_auto_bridge && (*look == UN_WIRE || *look == PW_WIRE
                                           || *look == UN_S_DIODE || *look == PW_S_DIODE
                                           || *look == UN_N_DIODE || *look == PW_N_DIODE
                                           || *look == UN_H_WIRE  || *look == PW_H_WIRE
                                           || *look == UN_V_WIRE  || *look == PW_V_WIRE
                                           || *look == UN_BRIDGE  || *look == UN_LEAKYB || *look == PW_LEAKYB
                                          )
                           ) { //Auto-bridging
                            *look = UN_BRIDGE;
                        } else {
                            *look = UN_W_DIODE;
                        }
                        to_move = true;
                        break;

                    case 'd': //Delay
                        *look = UN_DELAY;
                        to_move = true;
                        break;

                    case 'D': //Stretcher
                        *look = U1_STRETCH;
                        to_move = true;
                        break;

                    case 'z': //Undo
                        if (u == 0) { u = UNDOS; }
                        --u;
                        board[undos[u][0]][undos[u][1]] = undos[u][2];
                        ++can_redo;
                        break;

                    case 'Z': //Redo
                        if (u == UNDOS) { u = 0; }
                        if (can_redo > 0) {
                            --can_redo;
                            board[undos[u][0]][undos[u][1]] = undos[u][3];
                            ++u;
                        }
                        break;

                    case 'x': //Copy
                        if (is_copying = !is_copying) { //Are we not copying?
                            if (is_data_to_paste) { //Did we have something to paste?
                              //If so, clear it
                                is_copying = false;
                                copy_data->clear();
                                is_data_to_paste = false;
                            } else {
                              //Set copy
                                copy_X = cursor_X++;
                                copy_Y = cursor_Y++;
                              //'hide' last pasted
                                paste_X = paste_X2 = -1;
                                paste_Y = paste_Y2 = -1;
                            }
                        } else { //We were copying
                          //Perform the copy
                            copy_X2 = cursor_X;
                            copy_Y2 = cursor_Y;
                            paste_X_dist = (copy_X2 - copy_X);
                            paste_Y_dist = (copy_Y2 - copy_Y);
                            is_no_copy_source = false;
                            copy_data->clear();
                            for (int32_t y = copy_Y; y < copy_Y2; ++y) {
                                for (int32_t x = copy_X; x < copy_X2; ++x) {
                                    copy_data->push_back(board[x][y]);
                                }
                            }
                            is_data_to_paste = true;
                            cursor_X = copy_X;
                            cursor_Y = copy_Y;
                        }
                        break;

                    case 'b': //Paste -or- Bit
                        if (!is_data_to_paste) {
                            *look = UN_BIT;
                            to_move = true;
                            break;
                        }
                    case 'B': //Paste unelectrified
                    case 'm': //Paste x-flip
                    case 'w': //Paste y-flip
                    case 'k': //Cut
                    case 'K': //Swap
                    case 'j': //Clear area
                    case 'J': //Paste mask
                        if (is_data_to_paste) {
                            to_copy_cursor = true;
                            if (!is_no_copy_source) {
                                if (pressed_ch == 'k' || pressed_ch == 'j') { //Remove copied-from area (move/clear)?
                                    for (int32_t x = copy_X; x < copy_X2; ++x) {
                                        for (int32_t y = copy_Y; y < copy_Y2; ++y) {
                                            board[x][y] = EMPTY;
                                        }
                                    }
                                }
                            }
                            int32_t paste_X_end, paste_Y_end;
                            paste_X_end = cursor_X + paste_X_dist;
                            paste_Y_end = cursor_Y + paste_Y_dist;
                            if (pressed_ch != 'j') { //Paste copy data onto the board
                                uint32_t i = 0;
                                if (pressed_ch == 'K' && !is_no_copy_source) { //Swap
                                    for (int32_t y = copy_Y, y2 = cursor_Y; y < copy_Y2; ++y, ++y2) {
                                        for (int32_t x = copy_X, x2 = cursor_X; x < copy_X2; ++x, ++x2) {
                                            board[x][y] = board[x2][y2];
                                            board[x2][y2] = copy_data->at(i);
                                            ++i;
                                        }
                                    }
                                    i = 0;
                                }
                                if (pressed_ch == 'm') { //x-flip paste
                                    for (int32_t y = cursor_Y; y < paste_Y_end; ++y) {
                                        for (int32_t x = paste_X_end - 1; x >= cursor_X; --x) {
                                            board[x][y] = copy_data->at(i);
                                            if (board[x][y] == UN_E_DIODE || board[x][y] == PW_E_DIODE) { board[x][y] = UN_W_DIODE; } //E Diode to W Diode
                                            else if (board[x][y] == UN_W_DIODE || board[x][y] == PW_W_DIODE) { board[x][y] = UN_E_DIODE; } //W Diode to E Diode
                                            ++i;
                                        }
                                    }
                                } else if (pressed_ch == 'w') { //y-flip paste
                                    for (int32_t y = paste_Y_end - 1; y >= cursor_Y; --y) {
                                        for (int32_t x = cursor_X; x < paste_X_end; ++x) {
                                            board[x][y] = copy_data->at(i);
                                            if (board[x][y] == UN_S_DIODE || board[x][y] == PW_S_DIODE) { board[x][y] = UN_N_DIODE; } //S Diode to N Diode
                                            else if (board[x][y] == UN_N_DIODE || board[x][y] == PW_N_DIODE) { board[x][y] = UN_S_DIODE; } //N Diode to S Diode
                                            ++i;
                                        }
                                    }
                                } else { //Other paste
                                    if (pressed_ch == 'B') { //Unelectrified
                                        for (int32_t y = cursor_Y; y < paste_Y_end; ++y) {
                                            for (int32_t x = cursor_X; x < paste_X_end; ++x) {
                                                uint8_t c = copy_data->at(i);
                                                switch (c) {
                                                    case P3_STRETCH: case P2_STRETCH: case P1_STRETCH: c = U1_STRETCH; break;
                                                    case PW_WIRE: case PW_BRIDGE: case PW_LEAKYB: case PW_BIT: case PW_N_DIODE: case PW_DELAY: case PW_E_DIODE: case PW_S_DIODE: case PW_W_DIODE: case PW_H_WIRE: case PW_V_WIRE:
                                                        --c;
                                                        break;
                                                }
                                                board[x][y] = c;
                                                ++i;
                                            }
                                        }
                                    } else { //Unmodified
                                        if (pressed_ch == 'J') { //Paste mask (' ' is not pasted)
                                            for (int32_t y = cursor_Y; y < paste_Y_end; ++y) {
                                                for (int32_t x = cursor_X; x < paste_X_end; ++x) {
                                                    char c = copy_data->at(i);
                                                    if (c) { board[x][y] = c; }
                                                    ++i;
                                                }
                                            }
                                        } else if (pressed_ch == 'b' || pressed_ch == 'k') { //Normal, full paste
                                            for (int32_t y = cursor_Y; y < paste_Y_end; ++y) {
                                                for (int32_t x = cursor_X; x < paste_X_end; ++x) {
                                                    if (i >= copy_data->size()) { break; }
                                                    board[x][y] = copy_data->at(i);
                                                    ++i;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            if ((pressed_ch == 'k' || pressed_ch == 'j' || pressed_ch == 'K') && !is_no_copy_source) { //Was remove copied-from area (cut/clear) or have we finished (swap)?
                                copy_data->clear();
                                is_data_to_paste = false;
                            }
                          //Set last pasted area markers
                            paste_X  = cursor_X;
                            paste_Y  = cursor_Y;
                            paste_X2 = paste_X_end;
                            paste_Y2 = paste_Y_end;
                          //Re-calculate the electrification area
                          //Use heuristic
                            if (paste_X < elec_X) { elec_X = paste_X; }
                            if (paste_X2 - 1 > elec_X2) { elec_X2 = paste_X2 - 1; }
                            if (paste_Y < elec_Y) { elec_Y = paste_Y; }
                            if (paste_Y2 - 1 > elec_Y2) { elec_Y2 = paste_Y2 - 1; }
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

                    case 'Y': //Save project/component
                    {
                        bool is_save_component = is_data_to_paste && !is_no_copy_source;
                        std::cout << "\033[0;37;40mSaving \033[1;37;40m";
                        if (is_save_component) {
                            std::cout << "COMPONENT" << std::endl;
                        } else {
                            std::cout << "PROJECT" << std::endl;
                        }
                        std::cout << "\033[0;37;40mSave name: ";
                        std::string save = getInput(proj_name + (is_save_component ? "-" : ""));
                        if (save.length()) {
                            std::cout << std::endl << "Confirm? (y/N)" << std::endl;
                            char sure = getchar();
                            if (sure == 'y') {
                              //Recalculate electrification area
                                elecReCalculate();
                              //Save project to storage
                                if (!is_save_component) { proj_name = save; }
                                std::cout << "Saving..." << std::endl;
                                system(("rm " + save + ".gz &> /dev/null").c_str());
                                std::string save_data = "";
                                
                                uint32_t x1, y1, x2, y2;
                                if (is_save_component) {
                                    x1 = copy_X;
                                    y1 = copy_Y;
                                    x2 = copy_X2 - 1;
                                    y2 = copy_Y2 - 1;
                                } else {
                                    x1 = elec_X;
                                    y1 = elec_Y;
                                    x2 = elec_X2;
                                    y2 = elec_Y2;
                                }
                                
                                for (uint32_t y = y1; y <= y2; ++y) {
                                    for (uint32_t x = x1; x <= x2; ++x) {
                                        switch (board[x][y]) {
                                            case EMPTY:                          save_data += ' '; break;
                                            case UN_WIRE: case PW_WIRE:          save_data += '#'; break;
                                            case UN_AND: case PW_AND:            save_data += 'A'; break;
                                            case UN_NOT: case PW_NOT:            save_data += 'N'; break;
                                            case UN_XOR: case PW_XOR:            save_data += 'X'; break;
                                            case UN_BRIDGE: case PW_BRIDGE:      save_data += '+'; break;
                                            case UN_LEAKYB: case PW_LEAKYB:      save_data += '~'; break;
                                            case PW_POWER:                       save_data += '@'; break;
                                            case UN_WALL:                        save_data += ';'; break;
                                            case E_WALL:                         save_data += ':'; break;
                                            case UN_BIT: case PW_BIT:            save_data += 'B'; break;
                                            case UN_N_DIODE: case PW_N_DIODE:    save_data += '^'; break;
                                            case UN_E_DIODE: case PW_E_DIODE:    save_data += '>'; break;
                                            case UN_S_DIODE: case PW_S_DIODE:    save_data += 'V'; break;
                                            case UN_DELAY: case PW_DELAY:        save_data += '%'; break;
                                            case U1_STRETCH: case P1_STRETCH: case P2_STRETCH: case P3_STRETCH: save_data += '$'; break;
                                            case UN_W_DIODE: case PW_W_DIODE:    save_data += '<'; break;
                                            case UN_H_WIRE: case PW_H_WIRE:      save_data += '-'; break;
                                            case UN_V_WIRE: case PW_V_WIRE:      save_data += '|'; break;
                                        }
                                        if (board[x][y] >= 50 && board[x][y] <= 59) { //Is switch?
                                            save_data += board[x][y] - 2;
                                        } else if (board[x][y] >= 97 && board[x][y] <= 122) { //Is label?
                                            save_data += board[x][y];
                                        }
                                    }
                                    save_data += '\n';
                                }
                                std::ofstream out(save.c_str());
                                out << save_data;
                                out.close();
                                std::cout << "Compressing..." << std::endl;
                                system((SYSPATH + "gzip " + save).c_str());
                            }
                        }
                    }
                        break;

                    case 'v': //Load component
                    case 'y': //Load project
                    {
                        bool is_load_component = pressed_ch == 'v';
                        std::cout << "\033[0;37;40mLoading \033[1;37;40m";
                        if (is_load_component) {
                            std::cout << "COMPONENT" << std::endl;
                        } else {
                            std::cout << "PROJECT" << std::endl;
                        }
                        std::cout << "\033[0;37;40mLoad name: ";
                        std::string load = getInput();
                        if (load.length()) {
                            std::cout << std::endl << "Confirm? (y/N)" << std::endl;
                            char sure = getchar();
                            if (sure == 'y') {
                              //Decompress project from storage
                                std::cout << "Decompressing..." << std::endl;
                                system(("cp " + load + ".gz load.gz").c_str());
                                system((SYSPATH + "gzip -d load.gz").c_str());
                                std::string load_data = "";
                                std::ifstream in("load");
                                load_data = std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
                                in.close();
                                system("rm load");
                                std::cout << "Loading..." << std::endl;
                              //Load project/component from decompressed data
                                uint64_t len = load_data.length();
                                uint32_t top_left_X = 0, top_left_Y = 0;
                              //Either: find rough project dimensions to place the project in the middle of the board OR to define paste size for component
                                uint32_t rough_W, rough_H;
                                for (uint32_t i = 0; i < len; ++i) {
                                    if (load_data[i] == '\n') {
                                        rough_W = i;
                                        rough_H = len / i;
                                        if (is_load_component) {
                                            paste_X_dist = rough_W;
                                            paste_Y_dist = rough_H - 1;
                                        } else {
                                            top_left_X = board_W/2 - rough_W/2;
                                            top_left_Y = board_H/2 - rough_H/2;
                                        }
                                        break;
                                    }
                                }
                              //Component/project specific preparations
                                if (is_load_component) {
                                    copy_data->clear();
                                    is_data_to_paste = true;
                                    is_no_copy_source = true;
                                    copy_X = copy_Y = copy_X2 = copy_Y2 = paste_X = paste_Y = paste_X2 = paste_Y2 = -1;
                                } else {
                                  //Empty the current board
                                    memset(board, 0, sizeof(board[0][0]) * board_H * board_W);
                                  //Turn off all Switches
                                    for (uint32_t i = 0; i < 10; ++i) {
                                        switches[i] = false;
                                    }
                                  //Remove all previous electricity
                                    memset(branch, 0, sizeof(branch));
                                    branches = 0;
                                }
                              //Iterate through data
                                if (len > 0) {
                                    uint8_t load_char;
                                    int32_t x = top_left_X, y = top_left_Y;
                                    for (uint32_t i = 0; i < len; ++i) {
                                        if (load_data[i] == '\n') {
                                            ++y;
                                            x = top_left_X;
                                        } else {
                                            load_char = EMPTY;
                                            switch (load_data[i])
                                            {
                                                case ' ': break; //Empty
                                                case '#': load_char = UN_WIRE;    break;
                                                case 'A': load_char = UN_AND;     break;
                                                case 'N': load_char = UN_NOT;     break;
                                                case 'X': load_char = UN_XOR;     break;
                                                case '+': load_char = UN_BRIDGE;  break;
                                                case '~': load_char = UN_LEAKYB;  break;
                                                case '@': load_char = PW_POWER;   break;
                                                case 'V': load_char = UN_S_DIODE; break;
                                                case ';': load_char = UN_WALL;    break;
                                                case ':': load_char = E_WALL;     break;
                                                case 'B': load_char = UN_BIT;     break;
                                                case '^': load_char = UN_N_DIODE; break;
                                                case '%': load_char = UN_DELAY;   break;
                                                case '$': load_char = U1_STRETCH; break;
                                                case '>': load_char = UN_E_DIODE; break;
                                                case '<': load_char = UN_W_DIODE; break;
                                                case '-': load_char = UN_H_WIRE;  break;
                                                case '|': load_char = UN_V_WIRE;  break;
                                            }
                                            if (load_data[i] >= 48 && load_data[i] <= 57) //Is switch?
                                            {
                                                switch_num = load_data[i] - 48;
                                                load_char = switch_num + 50;
                                                switches[switch_num] = false; //Set its power
                                                placed_switches[switch_num] = true; //Say it's placed
                                            } else if (load_data[i] >= 97 && load_data[i] <= 122) { //Is label?
                                                load_char = load_data[i];
                                            }
                                            if (is_load_component) {
                                                copy_data->push_back(load_char);
                                            } else {
                                                board[x][y] = load_char;
                                            }
                                            ++x;
                                        }
                                    }
                                  //Recalculate electrification area
                                    elecReCalculate();
                                  //Move cursor
                                    if (!is_load_component && proj_name != load) {
                                        cursor_X = elec_X + MOVE_FAR*2;
                                        cursor_Y = elec_Y + MOVE_FAR;
                                    }
                                  //Set project name
                                    proj_name = load;
                                }
                            }
                        }
                    }
                        break;

                    case 'F': //Go-to
                    {
                        std::cout << "\033[0;37;40mEnter X coord: ";
                        std::string x_coord = getInput(std::to_string(cursor_X), false);
                        if (!x_coord.length()) { break; }
                        std::cout << "\nEnter Y coord: ";
                        std::string y_coord = getInput(std::to_string(cursor_Y), false);
                        if (!y_coord.length()) { break; }
                        cursor_X = atoi(x_coord.c_str());
                        cursor_Y = atoi(y_coord.c_str());
                    }
                        break;

                    case 'f': //Crosshairs
                        to_crosshairs = !to_crosshairs;
                        break;

                    case 'p': //Next elec
                        elec();
                        break;

                    case 'P': //Toggle pause
                        paused = !paused;
                        break;

                    case 'i': //Slow-mo
                        is_slow_mo = !is_slow_mo;
                        is_fast_mo = false;
                        break;

                    case 'I': //Fast-mo
                        is_fast_mo = !is_fast_mo;
                        is_slow_mo = false;
                        break;

                    case '\'': //Label mode
                        is_label_mode = !is_label_mode;
                        start_label_X = cursor_X;
                        break;
                }
                if (pressed_ch >= 48 && pressed_ch <= 57) { //Power button
                    switch_num = pressed_ch - 48;
                    if (placed_switches[switch_num]) { //Is the switch already placed? Power it
                        bool found = false;
                        for (int32_t x = 0; x < board_W; ++x) {
                            for (int32_t y = 0; y < board_H; ++y) {
                                if (board[x][y] == 50 + switch_num) { found = true; }
                            }
                        }
                        if (found) { //But was it reaaaaaaally placed? It could have been overwrote
                            switches[switch_num] = !switches[switch_num];
                        } else {
                            placed_switches[switch_num] = false; //Mark it as non-existant
                        }
                    } else { //No? Place it
                        *look = 50 + switch_num;
                        switches[switch_num] = false;
                        placed_switches[switch_num] = true;
                        to_move = true;
                    }
                }
              //Re-calculate the electrification area using heuristic
                if (to_move || to_recalc) {
                    int32_t x = cursor_X;
                    int32_t y = cursor_Y;
                    if (x < elec_X) { elec_X = x; }
                    else if (x > elec_X2) { elec_X2 = x; }
                    if (y < elec_Y) { elec_Y = y; }
                    else if (y > elec_Y2) { elec_Y2 = y; }
                }
            }
            if (to_move) { //We placed something, so lets move out of the way
                if (is_label_mode) {
                    ++cursor_X;
                } else {
                    cursor_X += prev_dir_X;
                    cursor_Y += prev_dir_Y;
                }
                //This was probably an action requiring an undo, so let's record it
                undos[u][0] = prev_X;
                undos[u][1] = prev_Y;
                undos[u][2] = prev_Z;
                undos[u][3] = board[prev_X][prev_Y];
                can_redo = 0;
                ++u;
                if (u == UNDOS) { u = 0; }
            }
          //Bounds checking
            if (prev_dir_X != 0 || prev_dir_Y != 0) { //The user made a move
              //General bounds check
                if (cursor_X < 0) { cursor_X = 0; }
                if (cursor_Y < 0) { cursor_Y = 0; }
                if (cursor_X > board_W) { cursor_X = board_W; }
                if (cursor_Y > board_H) { cursor_Y = board_H; }
              //Copy/paste bounds check
                if (is_copying && (cursor_X < copy_X || cursor_Y < copy_Y)) //Is the user trying to go out of copying bounds while copying?
                {
                    cursor_X = copy_X;
                    cursor_Y = copy_Y;
                }
            }
        }
      //Sleep
        clock += std::chrono::milliseconds(100);
        std::this_thread::sleep_until(clock);
    }
    return 0;
}

//std::cout << ": " + std::to_string() << std::endl;
