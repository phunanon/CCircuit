#include "fileio.hpp"

void saveBoard (std::string save_name, bool _is_component)
{
    if (!_is_component) { proj_name = save_name; }
  //Recalculate electrification area
    elecReCalculate();
    std::cout << "Saving..." << std::endl;
    system(("rm " + save_name + ".gz &> /dev/null").c_str());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
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
                case B_WALL:                        save_data += ':'; break;
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
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::cout << "Compressing..." << std::endl;
    system((SYSPATH + "gzip " + save_name).c_str());
}



void loadBoard (std::string load_name, bool _is_component)
{
  //Decompress project from storage
    std::cout << "Decompressing..." << std::endl;
    system(("cp " + load_name + ".gz load.gz").c_str());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    system((SYSPATH + "gzip -d load.gz").c_str());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::string load_data = "";
    std::ifstream in("load");
    load_data = std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
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
                paste_Y_dist = rough_H;
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
                    case ':': load_char = B_WALL;     break;
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
        if (!_is_component) {
          //Recalculate electrification area
            elecReCalculate();
            if (proj_name != load_name) {
              //Move cursor
                cursor_X = elec_X + (elec_X2 - elec_X > MOVE_FAR ? MOVE_FAR*2 : 0);
                cursor_Y = elec_Y + (elec_Y2 - elec_X > MOVE_FAR ? MOVE_FAR : 0);
              //Set project name
                proj_name = load_name;
            }
        }
    }
}
