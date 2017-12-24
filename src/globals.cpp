#include "globals.hpp"

const std::string SYSPATH = ""; //"/system/xbin"; //For BusyBox on Android

std::string proj_name = "untitled";
bool is_run = true;

const uint16_t elec_W = board_W - 2;
const uint16_t elec_H = board_H - 2;
char board[board_W][board_H];
const uint8_t MOVE_FAR = 8;
uint16_t cursor_X = MOVE_FAR*2;
uint16_t cursor_Y = MOVE_FAR;
bool to_elec_cursor = false;
bool to_copy_cursor = false;

uint8_t switches[10];
bool placed_switches[10];
uint8_t switch_num;

uint16_t elec_X = board_W, elec_Y = board_H, elec_X2 = 0, elec_Y2 = 0; //Electric bounds (swapped to induce tight first electrification)

uint16_t undos[UNDOS][4]; //[u][x, y, was, now]
uint16_t can_redo = 0;
uint16_t u = 0;

bool is_copying, is_data_to_paste;
uint16_t copy_X, copy_Y, copy_X2, copy_Y2, paste_X_dist, paste_Y_dist;
uint16_t pasted_X, pasted_Y, pasted_X2, pasted_Y2;
std::vector<char> *copy_data = new std::vector<char>();
std::vector<char> *restore_origin_data = new std::vector<char>();
std::vector<char> *restore_destin_data = new std::vector<char>();
bool is_no_copy_source; //Data was from storage, not project

bool is_auto_bridge = false;
bool is_dir_wire = false;
bool is_slow_mo = false;
bool is_fast_mo = false;
bool is_paused = false;
bool is_label_mode = false; //Are we typing a label?

uint16_t screen_W = 60; //The width of the screen
uint16_t screen_H = 20; //The height of the screen
uint16_t screen_W_half = screen_W / 2; //Half of the width of the screen
uint16_t screen_H_half = screen_H / 2; //Half of the height of the screen



void cloneVector (std::vector<char>* _dest, std::vector<char>* _orig)
{
    _dest->clear();
    _dest->reserve(_orig->size());
    for(uint32_t i = 0, ilen = _orig->size(); i < ilen; ++i) { _dest->push_back(_orig->at(i)); }
}




