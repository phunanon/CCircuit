#include "display.hpp"

bool to_crosshairs = false;
bool to_hide_UI = false;


void clearScreen () { std::cout << "\033c"; }

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



const std::string display_colour[] = {
    "30;47", "30;47", "30;42", "30;47", "30;42", "30;47", "30;42", "37;43", "1;37;43", "37;41",                 //0-9
    "1;37;41", "37;45", "1;37;45", "30;47", "30;42", "30;47", "30;42", "4;30;47", "4;30;42", "1;37;44",         //10-19
    "1;30;40", "1;30;40", "1;30;47", "1;37;44", "30;47", "30;42", "30;47", "30;42", "30;47", "30;42",           //20-29
    "30;47", "30;42", "1;30;47", "1;30;47", "1;30;42", "1;30;42", "1;30;47", "1;30;42", "30;40", "34;44",       //30-39
    "37;44", "1;37;44", "", "", "", "", "", "", "", "",                                                         //40-49
    "1;31;47", "1;31;47", "1;31;47", "1;31;47", "1;31;47", "1;31;47", "1;31;47", "1;31;47", "1;31;47", "1;31;47"//50-59
};



void display ()
{
    resizeScreen();
  //Calculate part of board on the screen
    int16_t board_crop_X;
    int16_t board_crop_Y;
    uint16_t board_crop_X2;
    uint16_t board_crop_Y2;
    board_crop_X = cursor_X - screen_W_half;
    if (board_crop_X < 0) { board_crop_X = 0; }
    board_crop_X2 = board_crop_X + screen_W;
    if (board_crop_X2 > board_W) { board_crop_X = board_W - screen_W; board_crop_X2 = board_W; }
    board_crop_Y = cursor_Y - screen_H_half;
    if (board_crop_Y < 0) { board_crop_Y = 0; }
    board_crop_Y2 = board_crop_Y + screen_H;
    if (board_crop_Y2 > board_H) { board_crop_Y = board_H - screen_H; board_crop_Y2 = board_H; }
  //Prepare
    std::string buffer;
    std::string buff;
    buffer = "";
  //Top bar
    if (!to_hide_UI) {
        uint16_t bar_off_len = 0;
        buffer += "\033[0;37;40m"; //bg of bar
        bar_off_len += 10;
        buffer += std::to_string(cursor_X) +", "+ std::to_string(cursor_Y) +"  ";
        if (is_copying)       { buffer += std::to_string(cursor_X - copy_X) +"x"+ std::to_string(cursor_Y - copy_Y) +"  "; }
        if (is_data_to_paste) { buffer += std::to_string(copy_X2 - copy_X) +"x"+ std::to_string(copy_Y2 - copy_Y) +"  "; }
        for (uint8_t i = 0; i < 10; ++i) {
            buffer += "\033[37;40m" + (switches[i] ? "\033[30;42m" + std::to_string(i) : (placed_switches[i] ? "\033[30;43m" + std::to_string(i) : "\033[37;40m" + std::to_string(i)));
            bar_off_len += 16;
        }
        buffer += "\033[37;40m";
        bar_off_len += 8;
        buffer += (!is_label_mode && !is_copying ? "  " + std::string(is_dir_wire ? "-|" : "#") : ""); 
        if (is_copying) {
            buffer += "  SELECT "+ std::string(1, keys[c_select]) +":complete";
        } else {
            if (is_data_to_paste) {
                buffer += "  PASTING ";
                if (is_no_copy_source) {
                    buffer += std::string(1, keys[c_select]) +":dismiss "+ std::string(1, keys[c_paste]) +":paste "+ std::string(1, keys[c_paste_unelec]) +":unelec "+ std::string(1, keys[c_paste_x_flip]) +":x-flip "+ std::string(1, keys[c_paste_y_flip]) +":y-flip "+ std::string(1, keys[c_paste_mask]) +":mask";
                } else {
                    buffer += std::string(1, keys[c_select]) +":dismiss "+ std::string(1, keys[c_paste]) +":paste "+ std::string(1, keys[c_paste_unelec]) +":unelec "+ std::string(1, keys[c_paste_move]) +":cut "+ std::string(1, keys[c_paste_swap]) +":swap "+ std::string(1, keys[c_clear_area]) +":clear "+ std::string(1, keys[c_paste_x_flip]) +":x-flip "+ std::string(1, keys[c_paste_y_flip]) +":y-flip "+ std::string(1, keys[c_paste_mask]) +":mask "+ std::string(1, keys[c_save]) +":save";
                }
            }
        }
        buffer += (is_auto_bridge ? "  auto bridge" : "");
        buffer += (is_slow_mo ? "  slow-mo" : (is_fast_mo ? "  fast-mo" : ""));
        buffer += (is_paused ? "  paused" : "");
        buffer += (is_label_mode ? "  LABEL" : "");
        std::string space = "";
        uint16_t s_len = screen_W - (buffer.length() - bar_off_len) - proj_name.length();
        for (uint16_t i = 0; i < s_len; ++i) { space += " "; }
        buffer += space + "\033[1;4m" + proj_name + "\033[0m";
        buffer += '\n';
    } else {
        ++board_crop_Y2;
    }
  //Board
    uint16_t sx, sy;
    std::string prev_colour = "";
    for (uint16_t y = board_crop_Y, sy = 0; y < board_crop_Y2; ++y, ++sy) {
        if (sy) { buffer += '\n'; }
        for (uint16_t x = board_crop_X, sx = 0; x < board_crop_X2; ++x, ++sx) {
            buff = " ";
            char *look = &board[x][y];
            std::string colour = "\033[0;";
            std::string add_colour = "";

            auto adapterColour = [x, y](char adapter) {
                char look;
                uint8_t adapters = 0;
                std::string colour;
                auto isAdaptable = [&adapters, &colour] (char look) {
                    if (look == UN_AND || look == UN_NOT || look == UN_XOR || look == PW_AND || look == PW_NOT || look == PW_XOR || look == UN_DELAY || look == PW_DELAY || look == UN_RANDOM || look == PW_RANDOM || look == PW_POWER || (look >= 50 && look <= 59) || (look >= U1_STRETCH && look <= P3_STRETCH)) { ++adapters; colour = display_colour[look]; }
                };
                look = board[x][y - 1];
                isAdaptable(look);
                look = board[x + 1][y];
                isAdaptable(look);
                look = board[x][y + 1];
                isAdaptable(look);
                look = board[x - 1][y];
                isAdaptable(look);
                return (adapters == 1 ? colour : display_colour[adapter]);
            };

            switch (*look) {
                case EMPTY: //Empty
                    buff = (x >= elec_X && x < elec_X2 + 1 && y >= elec_Y && y < elec_Y2 + 1 ? " " : ".");
                    break;
                case UN_WIRE: //Unpowered Wire
                case PW_WIRE: //Powered Wire
                    buff = "#";
                    break;
                case UN_H_WIRE: //Horizontal Wire
                case PW_H_WIRE: //Powered Horizontal Wire
                    buff = "-";
                    break;
                case UN_V_WIRE: //Vertical Wire
                case PW_V_WIRE: //Powered Vertical Wire
                    buff = "|";
                    break;
                case UN_AND: //AND
                case PW_AND: //Powered AND
                    buff = "A";
                    break;
                case UN_NOT: //NOT
                case PW_NOT: //Powered NOT
                    buff = "N";
                    break;
                case UN_XOR: //XOR
                case PW_XOR: //Powered XOR
                    buff = "X";
                    break;
                case UN_BRIDGE: //Bridge
                case PW_BRIDGE: //Powered Bridge
                case UN_LEAKYB: //Leaky Bridge
                case PW_LEAKYB: //Powered Leaky Bridge
                    buff = "+";
                    break;
                case PW_POWER: //Power
                    buff = "@";
                    break;
                case UN_WALL: //Wall
                    buff = ";";
                    break;
                case B_WALL: //Bridge Wall
                    buff = ":";
                    break;
                case UN_BIT: //Bit
                case PW_BIT: //Powered Bit
                    buff = "B";
                    break;
                case UN_N_DIODE: //North Diode
                case PW_N_DIODE: //Powered North Diode
                    buff = "^";
                    break;
                case UN_E_DIODE: //East Diode
                case PW_E_DIODE: //Powered East Diode
                    buff = ">";
                    break;
                case UN_S_DIODE: //South Diode
                case PW_S_DIODE: //Powered South Diode
                    buff = "V";
                    break;
                case UN_W_DIODE: //West Diode
                case PW_W_DIODE: //Powered West Diode
                    buff = "<";
                    break;
                case U1_STRETCH: //
                case P1_STRETCH: //
                case P2_STRETCH: //
                case P3_STRETCH: //Powered Stretcher
                    buff = "$";
                    break;
                case UN_DELAY: //Delay
                case PW_DELAY: //Powered Delay
                    buff = "%";
                    break;
                case UN_ADAPTER: //Adapter
                case PW_ADAPTER: //Powered Adapter
                    add_colour = adapterColour(*look);
                    buff = "O";
                    break;
                case UN_DISPLAY: //Display
                    buff = "+";
                    break;
                case PW_DISPLAY: //Powered Display
                    buff = "+";
                    break;
                case UN_RANDOM: //Random
                case PW_RANDOM: //Powered Random
                    buff = "R";
                    break;
            }

            if (add_colour == "" && *look < NOTHING) { add_colour = display_colour[*look]; }
            colour += add_colour +"m";
            if (prev_colour != colour) { buff = colour + buff; }
            prev_colour = colour;
            
            if (*look >= 50 && *look <= 59) { //Switch
                switch_num = *look - 50;
                buff = (switches[switch_num] ? "\033[1;33;44m" + std::to_string(switch_num) : "\033[1;31;47m" + std::to_string(switch_num)) ; //Power switch
                prev_colour = "";
            }
            
            if (*look >= 97 && *look <= 122) { //Label
                buff = "\033[4;1;34;47m" + std::string(look, look + 1);
                prev_colour = "";
            }

            if (!to_hide_UI) {
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
                        buff = "\033[0;30;46m" + buff.substr(buff.length() - 1, buff.length());
                    }
                  //Pasted markers
                    else if ((x == pasted_X && y == pasted_Y) || (x == pasted_X2 - 1 && y == pasted_Y) || (x == pasted_X && y == pasted_Y2 - 1) || (x == pasted_X2 - 1 && y == pasted_Y2 - 1)) {
                        buff = "\033[0;30;43m" + buff.substr(buff.length() - 1, buff.length());
                    }
                  //Paste area
                    else if (x >= cursor_X && x < cursor_X + paste_X_dist && y >= cursor_Y && y < cursor_Y + paste_Y_dist) {
                        buff = "\033[0;30;4"+ std::string(to_copy_cursor ? "0" : "6") +"m" + buff.substr(buff.length() - 1, buff.length());
                    }
                    prev_colour = "";
                }
              //Crosshairs and cursor
                if ((to_crosshairs && (x == cursor_X || y == cursor_Y)) || (x == cursor_X && y == cursor_Y) || (x == cursor_X && sy == 0) || (x == cursor_X && sy == screen_H - 1) || (sx == 0 && y == cursor_Y) || (sx == screen_W - 1 && y == cursor_Y)) {
                    buff = buff.substr(buff.length() - 1, buff.length());
                    buff = "\033[0;37;4" + std::string(is_data_to_paste ? (to_copy_cursor ? "0" : "6") : (to_elec_cursor ? "4" : "0")) + "m" + buff;
                    prev_colour = "";
                }
            }

            buffer += buff;
        }
    }
    clearScreen();
    std::cout << buffer;
    fflush(stdout);
    to_copy_cursor = false;
}
