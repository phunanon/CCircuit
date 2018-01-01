#include <iostream>     //For output to the terminal
#include <stdio.h>      //For output to the terminal: getchar; system ()
#include <string>       //For use of strings
#include <thread>       //For threads, and sleeping
#include <chrono>       //For thread sleeping
#include <ctime>        //For time
#include <fstream>      //For reading/writing
#include <vector>       //For use of vectors
#include "keypresses.c" //For detecting keypresses: kbhit (), pressed_ch

#include "globals.hpp"  //For all global variables throughout the program
#include "display.hpp"  //For displaying board and IDE to the terminal
#include "electrify.hpp"//For handling circuit simulation


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
    const uint8_t max_buffer = 255;
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



std::string getInput (std::string _pretext = "", bool to_autocomplete = true)
{
    std::string to_return = _pretext;
    std::cout << to_return;
    fflush(stdout);
    bool is_inputted = false;
    while (!is_inputted) {
        while (kbhit()) {
            pressed_ch = getchar(); //Get the key
            if (pressed_ch == '\n') { //Done
                is_inputted = true;
            } else if (pressed_ch == 27) { //Escape
                to_return = "";
                is_inputted = true;
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
  //Trim
    while (to_return.length() > 1 && to_return.substr(to_return.length() - 2, to_return.length() - 1) == " ") { to_return.substr(0, to_return.length() - 2); }
    return to_return;
}



void saveBoard (std::string save_name, bool _is_component = false)
{
    if (!_is_component) { proj_name = save_name; }
  //Recalculate electrification area
    elecReCalculate();
    std::cout << "Saving..." << std::endl;
    system(("rm " + save_name + ".gz &> /dev/null").c_str());
    std::string save_data = "";
    
    uint16_t x1, y1, x2, y2;
    if (_is_component) {
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
    
    for (uint16_t y = y1; y <= y2; ++y) {
        for (uint16_t x = x1; x <= x2; ++x) {
            switch (board[x][y]) {
                case EMPTY:                         save_data += ' '; break;
                case UN_WIRE: case PW_WIRE:         save_data += '#'; break;
                case UN_H_WIRE: case PW_H_WIRE:     save_data += '-'; break;
                case UN_V_WIRE: case PW_V_WIRE:     save_data += '|'; break;
                case UN_AND: case PW_AND:           save_data += 'A'; break;
                case UN_NOT: case PW_NOT:           save_data += 'N'; break;
                case UN_XOR: case PW_XOR:           save_data += 'X'; break;
                case UN_BRIDGE: case PW_BRIDGE:     save_data += '+'; break;
                case UN_LEAKYB: case PW_LEAKYB:     save_data += '~'; break;
                case PW_POWER:                      save_data += '@'; break;
                case UN_WALL:                       save_data += ';'; break;
                case E_WALL:                        save_data += ':'; break;
                case UN_BIT:                        save_data += 'B'; break;
                case PW_BIT:                        save_data += '&'; break;
                case UN_N_DIODE: case PW_N_DIODE:   save_data += '^'; break;
                case UN_E_DIODE: case PW_E_DIODE:   save_data += '>'; break;
                case UN_S_DIODE: case PW_S_DIODE:   save_data += 'V'; break;
                case UN_W_DIODE: case PW_W_DIODE:   save_data += '<'; break;
                case UN_DELAY: case PW_DELAY:       save_data += '%'; break;
                case U1_STRETCH: case P1_STRETCH: case P2_STRETCH: case P3_STRETCH: save_data += '$'; break;
                case UN_DISPLAY: case PW_DISPLAY:   save_data += 'D'; break;
                case UN_ADAPTER: case PW_ADAPTER:   save_data += 'O'; break;
                case UN_RANDOM: case PW_RANDOM:     save_data += 'R'; break;
            }
            if (board[x][y] >= 50 && board[x][y] <= 59) { //Is switch?
                save_data += board[x][y] - 2;
            } else if (board[x][y] >= 97 && board[x][y] <= 122) { //Is label?
                save_data += board[x][y];
            }
        }
        save_data += '\n';
    }
    std::ofstream out(save_name.c_str());
    out << save_data;
    out.close();
    std::cout << "Compressing..." << std::endl;
    system((SYSPATH + "gzip " + save_name).c_str());
}


void loadBoard (std::string load_name, bool _is_component = false)
{
  //Decompress project from storage
    std::cout << "Decompressing..." << std::endl;
    system(("cp " + load_name + ".gz load.gz").c_str());
    system((SYSPATH + "gzip -d load.gz").c_str());
    std::string load_data = "";
    std::ifstream in("load");
    load_data = std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();
    system("rm load");
    std::cout << "Loading..." << std::endl;
  //Load project/component from decompressed data
    uint64_t len = load_data.length();
    uint16_t top_left_X = 0, top_left_Y = 0;
  //Either: find rough project dimensions to place the project in the middle of the board OR to define paste size for component
    uint16_t rough_W, rough_H;
    for (uint16_t i = 0; i < len; ++i) {
        if (load_data[i] == '\n') {
            rough_W = i;
            rough_H = len / i;
            if (_is_component) {
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
    if (_is_component) {
        copy_data->clear();
        is_data_to_paste = true;
        is_no_copy_source = true;
        copy_X = copy_Y = copy_X2 = copy_Y2 = pasted_X = pasted_Y = pasted_X2 = pasted_Y2 = -1;
    } else {
        wipeBoard();
    }
  //Iterate through data
    if (len > 0) {
        uint8_t load_char;
        uint16_t x = top_left_X, y = top_left_Y;
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
                    case '-': load_char = UN_H_WIRE;  break;
                    case '|': load_char = UN_V_WIRE;  break;
                    case 'A': load_char = UN_AND;     break;
                    case 'N': load_char = UN_NOT;     break;
                    case 'X': load_char = UN_XOR;     break;
                    case '+': load_char = UN_BRIDGE;  break;
                    case '~': load_char = UN_LEAKYB;  break;
                    case '@': load_char = PW_POWER;   break;
                    case ';': load_char = UN_WALL;    break;
                    case ':': load_char = E_WALL;     break;
                    case 'B': load_char = UN_BIT;     break;
                    case '&': load_char = PW_BIT;     break;
                    case '^': load_char = UN_N_DIODE; break;
                    case '>': load_char = UN_E_DIODE; break;
                    case 'V': load_char = UN_S_DIODE; break;
                    case '<': load_char = UN_W_DIODE; break;
                    case '%': load_char = UN_DELAY;   break;
                    case '$': load_char = U1_STRETCH; break;
                    case 'D': load_char = UN_DISPLAY; break;
                    case 'O': load_char = UN_ADAPTER; break;
                    case 'R': load_char = UN_RANDOM;  break;
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
                if (_is_component) {
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
        if (!_is_component && proj_name != load_name) {
            cursor_X = elec_X + (elec_X2 - elec_X > MOVE_FAR ? MOVE_FAR*2 : 0);
            cursor_Y = elec_Y + (elec_Y2 - elec_X > MOVE_FAR ? MOVE_FAR : 0);
          //Set project name
            proj_name = load_name;
        }
    }
}


void outputWelcome ()
{
    auto wh = [](std::string text) { return "\n\033[0;37;40m"+ text +"\033[0m\t"; }; //Colour text to white on black
    auto rd = [](std::string text) { return "\033[1;31;40m"+ text +"\033[0m\t"; }; //Colour text to light red on black
    auto bl = [](std::string text) { return "\033[1;34;40m"+ text +"\033[0m\t"; }; //Colour text to light blue on black
    const std::string l_blob[2][24] = {
      //Dvorak
        { "[space]", "[enter]", ".oeu", ">OEU", "hH", "A", "L", "f", "F", "crCR", "gGl_", "tns-", "dDSb", "PpiI", "yYvW", "zZ", "ax", "X", "BkKj", "Mmw" },
      //Qwerty
        { "[space]", "[enter]", "ijkl", "IJKL", "fw", "+", "F", "u", "=", "ghGH", "tTer", "anAE", "dDsb", "Pp-_", "oSOW", "zy", "cv", "Z", "VxXM", "m,<" }
    };
    std::string blob[24] = l_blob[layout];
    uint8_t l = layout;
    std::cout << rd("\nCCircuit - a Linux terminal logic circuit simulator & IDE\nPatrick Bowen @phunanon 2017\n")
              << bl("\nINSTRUCTIONS\n============")
              << wh(blob[0]) +"remove anything"
              << wh(blob[1]) +"electrify cursor"
              << wh(blob[2]) +"up, left, down, right"
              << wh(blob[3]) +"far up, far left, far down, far right"
              << wh(blob[4]) +"place wire, toggle general use wire/directional wire"
              << wh(blob[5]) +"toggle auto-bridge for wires and diodes"
              << wh(blob[6]) +"far lay under cursor"
              << wh(blob[7]) +"crosshairs/hide UI"
              << wh(blob[8]) +"go-to coordinates"
              << wh("0-9") +"place/toggle switch"
              << wh(blob[9]) +"North/South/West/East diodes"
              << wh(blob[10]) +"bridge, leaky bridge, power, random"
              << wh(blob[11]) +"AND, NOT, XOR, adapter"
              << wh(blob[12]) +"delay, display, stretcher, bit"
              << wh(blob[13]) +"pause, next, slow-motion, fast-motion"
              << wh(";:") +"wall, conductive wall"
              << wh(blob[14]) +"load, save, import component, wipe board"
              << wh("'") +"toggle [lowercase] label mode"
              << wh(blob[15]) +"undo, redo"
              << wh("qQ") +"quit/no onexitsave quit"
              << wh("?") +"show this welcome screen"
              << std::endl
              << wh(blob[16]) +"initiate/complete/discard selection, paste"
              << wh(blob[18]) +"... paste unelectrified, move, swap, clear area"
              << wh(blob[19]) +"... paste mask, paste x flip, paste y flip"
              << wh(blob[17]) +"restore last cut/paste operation"
              << "\n\n- After initiating a selection, you can do a Save to export that component."
              << bl("\n\nINFORMATION\n===========")
              << "\n- Electronic tick is 1/10s (normal), 1s (slow), 1/80s (fast)"
              << "\n- Execute as ./CCircuit dvorak to change keyboard layout"
              << std::endl;
}



enum Control { c_nothing, c_remove, c_electrify, c_quit, c_quit_hard, c_backspace, c_select, c_north, c_east, c_south, c_west, c_far_north, c_far_east, c_far_south, c_far_west, c_wire, c_toggle_autobridge, c_toggle_wire, c_AND, c_NOT, c_XOR, c_bridge, c_leaky_bridge, c_power, c_random, c_far_lay, c_wall, c_e_wall, c_north_diode, c_east_diode, c_south_diode, c_west_diode, c_delay, c_display, c_stretcher, c_bit, c_adapter, c_undo, c_redo, c_paste, c_paste_unelec, c_paste_x_flip, c_paste_y_flip, c_paste_move, c_paste_swap, c_clear_area, c_paste_mask, c_undo_paste, c_load_component, c_load_project, c_save, c_wipe, c_goto, c_toggle_UI, c_slow_mo, c_fast_mo, c_toggle_label, c_next_elec, c_toggle_pause, c_help };
Control controls[256] = { c_nothing };

void determineKbLayout (int argc, char* argv[])
{
    if (argc > 1) {
        std::string entered = std::string(argv[1]);
        if         (entered == "dvorak") {
            layout = kb_dvorak;
        } else if (entered == "qwerty") {
            layout = kb_qwerty;
        }
    }
    
    controls[' ']   = c_remove;
    controls['\n']  = c_electrify;
    controls['q'] = c_quit;
    controls['Q'] = c_quit_hard;
    controls[127] = c_backspace;
    controls['\'']  = c_toggle_label;
    controls['p']   = c_next_elec;
    controls['P']   = c_toggle_pause;
    controls['?']   = c_help;
    
    switch (layout) {
        case kb_dvorak:
            controls['.']   = c_north;
            controls['u']   = c_east;
            controls['e']   = c_south;
            controls['o']   = c_west;
            controls['>']   = c_far_north;
            controls['U']   = c_far_east;
            controls['E']   = c_far_south;
            controls['O']   = c_far_west;
            controls['h']   = c_wire;
            controls['H']   = c_toggle_wire;
            controls['A']   = c_toggle_autobridge;
            controls['t']   = c_AND;
            controls['n']   = c_NOT;
            controls['s']   = c_XOR;
            controls['g']   = c_bridge;
            controls['G']   = c_leaky_bridge;
            controls['l']   = c_power;
            controls['_']   = c_random;
            controls['L']   = c_far_lay;
            controls[';']   = c_wall;
            controls[':']   = c_e_wall;
            controls['c']   = c_north_diode;
            controls['r']   = c_east_diode;
            controls['C']   = c_south_diode;
            controls['R']   = c_west_diode;
            controls['d']   = c_delay;
            controls['D']   = c_display;
            controls['S']   = c_stretcher;
            controls['b']   = c_bit;
            controls['-']   = c_adapter;
            controls['z']   = c_undo;
            controls['Z']   = c_redo;
            controls['a']   = c_select;
            controls['x']   = c_paste;
            controls['B']   = c_paste_unelec;
            controls['k']   = c_paste_move;
            controls['K']   = c_paste_swap;
            controls['m']   = c_paste_x_flip;
            controls['w']   = c_paste_y_flip;
            controls['j']   = c_clear_area;
            controls['M']   = c_paste_mask;
            controls['X']   = c_undo_paste;
            controls['v']   = c_load_component;
            controls['y']   = c_load_project;
            controls['Y']   = c_save;
            controls['W']   = c_wipe;
            controls['F']   = c_goto;
            controls['f']   = c_toggle_UI;
            controls['i']   = c_slow_mo;
            controls['I']   = c_fast_mo;
            break;
        case kb_qwerty:
            controls['i']   = c_north;
            controls['l']   = c_east;
            controls['k']   = c_south;
            controls['j']   = c_west;
            controls['I']   = c_far_north;
            controls['L']   = c_far_east;
            controls['K']   = c_far_south;
            controls['J']   = c_far_west;
            controls['f']   = c_wire;
            controls['w']   = c_toggle_wire;
            controls['+']   = c_toggle_autobridge;
            controls['a']   = c_AND;
            controls['n']   = c_NOT;
            controls['A']   = c_XOR;
            controls['t']   = c_bridge;
            controls['T']   = c_leaky_bridge;
            controls['e']   = c_power;
            controls['r']   = c_random;
            controls['F']   = c_far_lay;
            controls[';']   = c_wall;
            controls[':']   = c_e_wall;
            controls['g']   = c_north_diode;
            controls['H']   = c_east_diode;
            controls['h']   = c_south_diode;
            controls['G']   = c_west_diode;
            controls['d']   = c_delay;
            controls['D']   = c_display;
            controls['s']   = c_stretcher;
            controls['b']   = c_bit;
            controls['E']   = c_adapter;
            controls['z']   = c_undo;
            controls['y']   = c_redo;
            controls['c']   = c_select;
            controls['v']   = c_paste;
            controls['V']   = c_paste_unelec;
            controls['x']   = c_paste_move;
            controls['X']   = c_paste_swap;
            controls[',']   = c_paste_x_flip;
            controls['<']   = c_paste_y_flip;
            controls['M']   = c_clear_area;
            controls['m']   = c_paste_mask;
            controls['Z']   = c_undo_paste;
            controls['O']   = c_load_component;
            controls['o']   = c_load_project;
            controls['S']   = c_save;
            controls['W']   = c_wipe;
            controls['=']   = c_goto;
            controls['u']   = c_toggle_UI;
            controls['-']   = c_slow_mo;
            controls['_']   = c_fast_mo;
            break;
    }
}


uint16_t start_label_X; //Label's X
char prev_dir_X, prev_dir_Y, prev_Z, elec_counter;
uint16_t prev_X, prev_Y;
int32_t main (int argc, char* argv[])
{
  //Load shite to listen to pressed keys
    loadKeyListen();
    auto kbPause = []() { while (!kbhit()) { std::this_thread::sleep_for(std::chrono::milliseconds(50)); } };
  //Detect keyboard layout
    determineKbLayout(argc, argv);
  //Welcome screen
    outputWelcome();
    std::cout << "\n\nInitialising board..." << std::endl;
    memset(board, 0, sizeof(board[0][0]) * board_H * board_W); //Set the board Empty
    prev_dir_Y = 1;
    std::cout << "\033[37;40mLoad onexitsave? (Y/n): "; fflush(stdout);
    if (getchar() != 'n') {
        std::cout << "Load onexitsave..." << std::endl;
        loadBoard("onexitsave");
    }


    auto clock = std::chrono::system_clock::now();

    while (is_run) {
        display();
        if (!is_paused) {
            if (elec_counter >= (is_slow_mo ? 10 : 1)) { elec_counter = 0; elec(); }
            if (is_fast_mo) { for (uint8_t i = 0; i < 8; ++i) { elec(); } }
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
                prev_dir_Y = prev_dir_X = 0;
                prev_dir_X = 1;
            } else {

                uint8_t move_dist = 1;
                Control pressed_control = controls[pressed_ch];
                bool to_auto_bridge = is_auto_bridge && (*look == UN_BRIDGE
                                            || *look == UN_WIRE || *look == PW_WIRE
                                            || *look == UN_N_DIODE || *look == PW_N_DIODE
                                            || *look == UN_E_DIODE || *look == PW_E_DIODE
                                            || *look == UN_S_DIODE || *look == PW_S_DIODE
                                            || *look == UN_W_DIODE || *look == PW_W_DIODE
                                            || *look == UN_H_WIRE  || *look == PW_H_WIRE
                                            || *look == UN_V_WIRE  || *look == PW_V_WIRE
                                            || *look == UN_LEAKYB || *look == PW_LEAKYB);

                switch (pressed_control) {

                    case c_far_north: //Far North
                        move_dist = (is_data_to_paste ? paste_Y_dist : MOVE_FAR);
                    case c_north: //North
                        if (cursor_Y - move_dist >= 0) { cursor_Y -= move_dist; }
                        prev_dir_Y = prev_dir_X = 0;
                        prev_dir_Y = -1;
                        break;

                    case c_far_east: //Far East
                        move_dist = (is_data_to_paste ? paste_X_dist : MOVE_FAR*2);
                    case c_east: //East
                        if (cursor_X + move_dist < elec_W) { cursor_X += move_dist; }
                        prev_dir_Y = prev_dir_X = 0;
                        prev_dir_X = 1;
                        break;

                    case c_far_south: //Far South
                        move_dist = (is_data_to_paste ? paste_Y_dist : MOVE_FAR);
                    case c_south: //South
                        if (cursor_Y + move_dist < elec_H) { cursor_Y += move_dist; }
                        prev_dir_Y = prev_dir_X = 0;
                        prev_dir_Y = 1;
                        break;

                    case c_far_west: //Far West
                        move_dist = (is_data_to_paste ? paste_X_dist : MOVE_FAR*2);
                    case c_west: //West
                        if (cursor_X - move_dist >= 0) { cursor_X -= move_dist; }
                        prev_dir_Y = prev_dir_X = 0;
                        prev_dir_X = -1;
                        break;

                    case c_electrify: //New-line OR electrify cursor
                        if (is_label_mode) {
                            cursor_X = start_label_X;
                            if (cursor_Y + 1 < elec_H) { ++cursor_Y; }
                        } else {
                            to_elec_cursor = true;
                        }
                        break;

                    case c_backspace: //Left-remove
                        if (cursor_X > 1) { --cursor_X; }
                        prev_dir_Y = 0;
                        prev_dir_X = -1;
                        look = &board[cursor_X][cursor_Y];
                    case c_remove: //Remove
                        if (*look == 50 + switch_num) { //If on top of a switch, remove it correctly
                            placed_switches[switch_num] = false;
                            to_move = true;
                        }
                        *look = EMPTY;
                        to_move = (pressed_ch == 127 ? false : true);
                        break;

                    case c_wire: //Wire
                        if (to_auto_bridge) { //Auto-bridging
                            *look = (*look == UN_LEAKYB || *look == PW_LEAKYB ? *look : UN_BRIDGE);
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

                    case c_toggle_autobridge: //Auto-bridge
                        is_auto_bridge = !is_auto_bridge;
                        break;

                    case c_toggle_wire: //Toggle wire
                        is_dir_wire = !is_dir_wire;
                        break;

                    case c_AND: //AND
                        *look = UN_AND;
                        to_move = true;
                        break;

                    case c_NOT: //NOT
                        *look = UN_NOT;
                        to_move = true;
                        break;

                    case c_XOR: //XOR
                        *look = UN_XOR;
                        to_move = true;
                        break;

                    case c_bridge: //Bridge
                        *look = UN_BRIDGE;
                        to_move = true;
                        break;

                    case c_leaky_bridge: //Leaky Bridge
                        *look = UN_LEAKYB;
                        to_move = true;
                        break;

                    case c_power: //Power
                        *look = PW_POWER;
                        to_move = true;
                        break;

                    case c_random: //Random
                        *look = UN_RANDOM;
                        to_move = true;
                        break;

                    case c_far_lay: //Far lay
                        for (uint8_t i = 0; i < MOVE_FAR && cursor_X > 1 && cursor_Y > 1 && cursor_X < elec_W && cursor_Y < elec_H; ++i) {
                            cursor_X += prev_dir_X;
                            cursor_Y += prev_dir_Y;
                            if (board[cursor_X][cursor_Y] == EMPTY) { board[cursor_X][cursor_Y] = *look; } else { break; }
                        }
                        to_recalc = true;
                        break;

                    case c_wall: //Wall
                        *look = UN_WALL;
                        to_move = true;
                        break;

                    case c_e_wall: //Conductive Wall
                        *look = E_WALL;
                        to_move = true;
                        break;

                    case c_north_diode: //North Diode
                        if (to_auto_bridge) { //Auto-bridging
                            *look = (*look == UN_LEAKYB || *look == PW_LEAKYB ? *look : UN_BRIDGE);
                        } else {
                            *look = UN_N_DIODE;
                        }
                        to_move = true;
                        break;

                    case c_east_diode: //East diode
                        if (to_auto_bridge) { //Auto-bridging
                            *look = (*look == UN_LEAKYB || *look == PW_LEAKYB ? *look : UN_BRIDGE);
                        } else {
                            *look = UN_E_DIODE;
                        }
                        to_move = true;
                        break;

                    case c_south_diode: //South Diode
                        if (to_auto_bridge) { //Auto-bridging
                            *look = (*look == UN_LEAKYB || *look == PW_LEAKYB ? *look : UN_BRIDGE);
                        } else {
                            *look = UN_S_DIODE;
                        }
                        to_move = true;
                        break;

                    case c_west_diode: //West diode
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

                    case c_delay: //Delay
                        *look = UN_DELAY;
                        to_move = true;
                        break;

                    case c_stretcher: //Stretcher
                        *look = U1_STRETCH;
                        to_move = true;
                        break;

                    case c_display: //Display
                        *look = UN_DISPLAY;
                        to_move = true;
                        break;

                    case c_bit: //Bit
                        *look = UN_BIT;
                        to_move = true;
                        break;

                    case c_adapter: //Component Adapter
                        *look = UN_ADAPTER;
                        to_move = true;
                        break;

                    case c_undo: //Undo
                        if (u == 0) { u = UNDOS; }
                        --u;
                        board[undos[u][0]][undos[u][1]] = undos[u][2];
                        ++can_redo;
                        break;

                    case c_redo: //Redo
                        if (u == UNDOS) { u = 0; }
                        if (can_redo > 0) {
                            --can_redo;
                            board[undos[u][0]][undos[u][1]] = undos[u][3];
                            ++u;
                        }
                        break;

                    case c_select: //Copy
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
                                pasted_X = pasted_X2 = -1;
                                pasted_Y = pasted_Y2 = -1;
                            }
                        } else { //We were copying
                          //Perform the copy
                            copy_X2 = cursor_X;
                            copy_Y2 = cursor_Y;
                            paste_X_dist = (copy_X2 - copy_X);
                            paste_Y_dist = (copy_Y2 - copy_Y);
                            is_no_copy_source = false;
                            copy_data->clear();
                            for (uint16_t y = copy_Y; y < copy_Y2; ++y) {
                                for (uint16_t x = copy_X; x < copy_X2; ++x) {
                                    copy_data->push_back(board[x][y]);
                                }
                            }
                            is_data_to_paste = true;
                            cursor_X = copy_X;
                            cursor_Y = copy_Y;
                        }
                        break;


                    case c_paste: //Paste
                    case c_paste_unelec: //Paste unelectrified
                    case c_paste_x_flip: //Paste x-flip
                    case c_paste_y_flip: //Paste y-flip
                    case c_paste_move: //Cut
                    case c_paste_swap: //Swap
                    case c_clear_area: //Clear area
                    case c_paste_mask: //Paste mask
                        if (is_data_to_paste) {
                            to_copy_cursor = true;
                            uint16_t paste_X_end, paste_Y_end;
                            paste_X_end = cursor_X + paste_X_dist;
                            paste_Y_end = cursor_Y + paste_Y_dist;
                          //Prepare for later restore
                            bool can_restore_origin = !is_no_copy_source && (pressed_control == c_paste_move || pressed_control == c_paste_swap || pressed_control == c_clear_area);
                            bool can_restore_destin = (pressed_control == c_paste || pressed_control == c_paste_unelec || pressed_control == c_paste_x_flip || pressed_control == c_paste_move || pressed_control == c_paste_swap || pressed_control == c_paste_mask);
                            if (can_restore_origin) {
                                cloneVector(restore_origin_data, copy_data);
                            } else {
                                restore_origin_data->clear();
                            }
                            if (can_restore_destin) {
                                for (uint16_t y = cursor_Y; y < paste_Y_end; ++y) {
                                    for (uint16_t x = cursor_X; x < paste_X_end; ++x) {
                                        restore_destin_data->push_back(board[x][y]);
                                    }
                                }
                            } else {
                                restore_destin_data->clear();
                            }
                          //Paste operations
                            if (!is_no_copy_source) {
                                if (pressed_control == c_paste_move || pressed_control == c_clear_area) { //Remove copied-from area (move/clear)?
                                    for (uint16_t x = copy_X; x < copy_X2; ++x) {
                                        for (uint16_t y = copy_Y; y < copy_Y2; ++y) {
                                            board[x][y] = EMPTY;
                                        }
                                    }
                                }
                            }
                            if (pressed_control != c_clear_area) { //Paste copy data onto the board
                                uint16_t i = 0;
                                if (pressed_control == c_paste_swap && !is_no_copy_source) { //Swap
                                    for (uint16_t y = copy_Y, y2 = cursor_Y; y < copy_Y2; ++y, ++y2) {
                                        for (uint16_t x = copy_X, x2 = cursor_X; x < copy_X2; ++x, ++x2) {
                                            board[x][y] = board[x2][y2];
                                            board[x2][y2] = copy_data->at(i);
                                            ++i;
                                        }
                                    }
                                    i = 0;
                                }
                                if (pressed_control == c_paste_x_flip) { //x-flip paste
                                    for (uint16_t y = cursor_Y; y < paste_Y_end; ++y) {
                                        for (uint16_t x = paste_X_end - 1; x >= cursor_X; --x) {
                                            board[x][y] = copy_data->at(i);
                                            if (board[x][y] == UN_E_DIODE || board[x][y] == PW_E_DIODE) { board[x][y] = UN_W_DIODE; } //E Diode to W Diode
                                            else if (board[x][y] == UN_W_DIODE || board[x][y] == PW_W_DIODE) { board[x][y] = UN_E_DIODE; } //W Diode to E Diode
                                            ++i;
                                        }
                                    }
                                } else if (pressed_control == c_paste_y_flip) { //y-flip paste
                                    for (uint16_t y = paste_Y_end - 1; y >= cursor_Y; --y) {
                                        for (uint16_t x = cursor_X; x < paste_X_end; ++x) {
                                            board[x][y] = copy_data->at(i);
                                            if (board[x][y] == UN_S_DIODE || board[x][y] == PW_S_DIODE) { board[x][y] = UN_N_DIODE; } //S Diode to N Diode
                                            else if (board[x][y] == UN_N_DIODE || board[x][y] == PW_N_DIODE) { board[x][y] = UN_S_DIODE; } //N Diode to S Diode
                                            ++i;
                                        }
                                    }
                                } else { //Other paste
                                    if (pressed_control == c_paste_unelec) { //Unelectrified
                                        for (uint16_t y = cursor_Y; y < paste_Y_end; ++y) {
                                            for (uint16_t x = cursor_X; x < paste_X_end; ++x) {
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
                                        if (pressed_control == c_paste_mask) { //Paste mask (' ' is not pasted)
                                            for (uint16_t y = cursor_Y; y < paste_Y_end; ++y) {
                                                for (uint16_t x = cursor_X; x < paste_X_end; ++x) {
                                                    char c = copy_data->at(i);
                                                    if (c) { board[x][y] = c; }
                                                    ++i;
                                                }
                                            }
                                        } else if (pressed_control == c_paste || pressed_control == c_paste_move) { //Normal, full paste
                                            for (uint16_t y = cursor_Y; y < paste_Y_end; ++y) {
                                                for (uint16_t x = cursor_X; x < paste_X_end; ++x) {
                                                    if (i >= copy_data->size()) { break; }
                                                    board[x][y] = copy_data->at(i);
                                                    ++i;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            if (!is_no_copy_source && (pressed_control == c_paste_move || pressed_control == c_clear_area || pressed_control == c_paste_swap)) { //Was remove copied-from area (cut/clear) or have we finished (swap)?
                                copy_data->clear();
                                is_data_to_paste = false;
                            }
                          //Set last pasted area markers
                            pasted_X  = cursor_X;
                            pasted_Y  = cursor_Y;
                            pasted_X2 = paste_X_end;
                            pasted_Y2 = paste_Y_end;
                          //Re-calculate the electrification area
                          //Use heuristic
                            if (pasted_X < elec_X)       { elec_X = pasted_X; }
                            if (pasted_X2 - 1 > elec_X2) { elec_X2 = pasted_X2 - 1; }
                            if (pasted_Y < elec_Y)       { elec_Y = pasted_Y; }
                            if (pasted_Y2 - 1 > elec_Y2) { elec_Y2 = pasted_Y2 - 1; }
                        }
                        break;

                    case c_undo_paste: //Restore last cut/paste
                        if (restore_origin_data->size()) {
                            uint32_t i = 0;
                            for (uint16_t y = copy_Y; y < copy_Y2; ++y) {
                                for (uint16_t x = copy_X; x < copy_X2; ++x) {
                                    board[x][y] = restore_origin_data->at(i);
                                    ++i;
                                }
                            }
                            restore_origin_data->clear();
                        }
                        if (restore_destin_data->size()) {
                            uint32_t i = 0;
                            for (uint16_t y = pasted_Y; y < pasted_Y2; ++y) {
                                for (uint16_t x = pasted_X; x < pasted_X2; ++x) {
                                    board[x][y] = restore_destin_data->at(i);
                                    ++i;
                                }
                            }
                            restore_destin_data->clear();
                        }
                        break;

                    case c_quit: //Quit
                    case c_quit_hard: //No onexitsave Quit
                        std::cout << std::string("\033[0;37;40mAre you sure you want to quit") + (pressed_ch == 'Q' ? " without onexitsave" : "") + " (y/N)?" << std::endl;
                        kbPause();
                        if (getchar() == 'y') {
                            if (pressed_ch != 'Q') { 
                                std::cout << "On-exit saving...\033[0m" << std::endl;
                                saveBoard("onexitsave");
                            }
                            std::cout << "Quit.\033[0m" << std::endl;
                            is_run = false;
                        }
                        break;

                    case c_save: //Save project/component
                    {
                        bool is_save_component = is_data_to_paste && !is_no_copy_source;
                        std::cout << "\033[0;37;40mSaving  \033[1;37;40m";
                        if (is_save_component) {
                            std::cout << "COMPONENT" << std::endl;
                        } else {
                            std::cout << "PROJECT" << std::endl;
                        }
                        std::cout << "\033[0;37;40mSave name: ";
                        std::string save_name = getInput(proj_name + (is_save_component ? "-" : ""));
                        if (save_name.length()) {
                            std::cout << std::endl << "Confirm? (y/N)" << std::endl;
                            if (getchar() == 'y') {
                              //Save project to storage
                                saveBoard(save_name, is_save_component);
                            }
                        }
                    }
                        break;

                    case c_load_component: //Load component
                    case c_load_project: //Load project
                    {
                        bool is_load_component = (pressed_control == c_load_component);
                        std::cout << "\033[0;37;40mLoading \033[1;37;40m";
                        if (is_load_component) {
                            std::cout << "COMPONENT" << std::endl;
                        } else {
                            std::cout << "PROJECT" << std::endl;
                        }
                        std::cout << "\033[0;37;40mLoad name: ";
                        std::string load_name = getInput();
                        if (load_name.length()) {
                            std::cout << std::endl << "Confirm? (y/N)" << std::endl;
                            if (getchar() == 'y') {
                                loadBoard(load_name, is_load_component);
                            }
                        }
                    }
                        break;

                    case c_wipe:
                        std::cout << "\033[0;37;40mAre you sure you want to wipe the board? (y/N)?" << std::endl;
                        kbPause();
                        if (getchar() == 'y') {
                            wipeBoard();
                            elecReCalculate();
                        }
                        break;

                    case c_goto: //Go-to
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

                    case c_toggle_UI: //Crosshairs & UI
                        if (to_crosshairs && to_hide_UI) { to_crosshairs = to_hide_UI = false; break; }
                        if (to_crosshairs) { to_hide_UI = true; break; }
                        to_crosshairs = true;
                        break;

                    case c_next_elec: //Next elec
                        elec();
                        break;

                    case c_toggle_pause: //Toggle pause
                        is_paused = !is_paused;
                        break;

                    case c_slow_mo: //Slow-mo
                        is_slow_mo = !is_slow_mo;
                        is_fast_mo = false;
                        break;

                    case c_fast_mo: //Fast-mo
                        is_fast_mo = !is_fast_mo;
                        is_slow_mo = false;
                        break;

                    case c_toggle_label: //Label mode
                        is_label_mode = !is_label_mode;
                        start_label_X = cursor_X;
                        break;

                    case c_help: //Show welcome screen
                        outputWelcome();
                        std::cout << "\nPress any key to return." << std::endl;
                        kbPause();
                        getchar();
                        break;
                }
                if (pressed_ch >= 48 && pressed_ch <= 57) { //Power button
                    switch_num = pressed_ch - 48;
                    if (placed_switches[switch_num]) { //Is the switch already placed? Power it
                        bool found = false;
                        for (uint16_t x = 1; x < elec_W; ++x) {
                            for (uint16_t y = 1; y < elec_H; ++y) {
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
                    if (cursor_X < elec_X) { elec_X = cursor_X; }
                    else if (cursor_X > elec_X2) { elec_X2 = cursor_X; }
                    if (cursor_Y < elec_Y) { elec_Y = cursor_Y; }
                    else if (cursor_Y > elec_Y2) { elec_Y2 = cursor_Y; }
                }
            }
            if (to_move || is_label_mode) { //We placed something, so lets move out of the way
                cursor_X += prev_dir_X;
                cursor_Y += prev_dir_Y;
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
                if (cursor_X < 1) { cursor_X = 1; }
                if (cursor_Y < 1) { cursor_Y = 1; }
                if (cursor_X >= board_W) { cursor_X = board_W - 1; }
                if (cursor_Y >= board_H) { cursor_Y = board_H - 1; }
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
        if (clock + std::chrono::seconds(1) < std::chrono::system_clock::now()) { clock = std::chrono::system_clock::now(); }
        std::this_thread::sleep_until(clock);
    }
    return 0;
}
