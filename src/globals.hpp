#ifndef GLOBALS_INCLUDE
#define GLOBALS_INCLUDE

#include <string>       //For use of strings
#include <vector>       //For use of vectors


#define NODIR       0
#define NORTH       1
#define EAST        2
#define SOUTH       3
#define WEST        4

#define EMPTY       0
#define UN_WIRE     1
#define PW_WIRE     2
#define UN_H_WIRE   3
#define PW_H_WIRE   4
#define UN_V_WIRE   5
#define PW_V_WIRE   6
#define UN_AND      7
#define PW_AND      8
#define UN_NOT      9
#define PW_NOT      10
#define UN_XOR      11
#define PW_XOR      12
#define UN_ADAPTER  13
#define PW_ADAPTER  14
#define UN_BRIDGE   15
#define PW_BRIDGE   16
#define UN_LEAKYB   17
#define PW_LEAKYB   18
#define PW_POWER    19
#define UN_WALL     20
#define E_WALL      21
#define UN_BIT      22
#define PW_BIT      23
#define UN_N_DIODE  24
#define PW_N_DIODE  25
#define UN_E_DIODE  26
#define PW_E_DIODE  27
#define UN_S_DIODE  28
#define PW_S_DIODE  29
#define UN_W_DIODE  30
#define PW_W_DIODE  31
#define U1_STRETCH  32
#define P1_STRETCH  33
#define P2_STRETCH  34
#define P3_STRETCH  35
#define UN_DELAY    36
#define PW_DELAY    37
#define UN_DISPLAY  38
#define PW_DISPLAY  39
#define NOTHING     40

const uint16_t board_W = 4096;
const uint16_t board_H = 4096;
const uint16_t UNDOS = 512;

extern const std::string SYSPATH;

extern std::string proj_name;
extern bool is_run;

extern const uint16_t elec_W;
extern const uint16_t elec_H;
extern char board[board_W][board_H];
extern const uint8_t MOVE_FAR;
extern uint16_t cursor_X;
extern uint16_t cursor_Y;
extern bool to_elec_cursor;
extern bool to_copy_cursor;

extern uint8_t switches[10];
extern bool placed_switches[10];
extern uint8_t switch_num;

extern uint16_t elec_X, elec_Y, elec_X2, elec_Y2;

extern uint16_t undos[UNDOS][4];
extern uint16_t can_redo;
extern uint16_t u;

extern bool is_copying, is_data_to_paste;
extern uint16_t copy_X, copy_Y, copy_X2, copy_Y2, paste_X_dist, paste_Y_dist;
extern uint16_t pasted_X, pasted_Y, pasted_X2, pasted_Y2;
extern std::vector<char> *copy_data;
extern std::vector<char> *restore_origin_data;
extern std::vector<char> *restore_destin_data;
extern bool is_no_copy_source;

extern bool is_auto_bridge;
extern bool is_dir_wire;
extern bool is_slow_mo;
extern bool is_fast_mo;
extern bool is_paused;
extern bool is_label_mode;

extern uint16_t screen_W;
extern uint16_t screen_H;
extern uint16_t screen_W_half;
extern uint16_t screen_H_half;


extern void cloneVector (std::vector<char>*, std::vector<char>*);

#endif
