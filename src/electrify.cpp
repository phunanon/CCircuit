#include "electrify.hpp"

struct Branch
{
    uint16_t x, y;
    uint8_t d;
};

Branch branch[8192];
uint16_t branches = 0;

void wipeBoard ()
{
  //Empty the current board
    memset(board, 0, sizeof(board[0][0]) * board_H * board_W);
  //Remove and turn off all Switches
    for (uint8_t i = 0; i < 10; ++i) {
        placed_switches[i] = false;
        switches[i] = false;
    }
  //Remove all previous electricity
    memset(branch, 0, sizeof(branch));
    branches = 0;
}

void elecReCalculate ()
{
    elec_X = board_W;
    elec_X2 = 0;
    elec_Y = board_H;
    elec_Y2 = 0;
    for (uint16_t x = 1; x < elec_W; ++x) {
        for (uint16_t y = 1; y < elec_H; ++y) {
            if (board[x][y]) {
                if (x < elec_X) { elec_X = x; }
                else if (x > elec_X2) { elec_X2 = x; }
                if (y < elec_Y) { elec_Y = y; }
                else if (y > elec_Y2) { elec_Y2 = y; }
            }
        }
    }
}



uint8_t nextToAdapter (uint16_t _X, uint16_t _Y)
{
    char look;
    look = board[_X][_Y - 1];
    if (look == UN_ADAPTER || look == PW_ADAPTER) { return NORTH; }
    look = board[_X + 1][_Y];
    if (look == UN_ADAPTER || look == PW_ADAPTER) { return EAST; }
    look = board[_X - 1][_Y];
    if (look == UN_ADAPTER || look == PW_ADAPTER) { return WEST; }
    return NODIR;
}


bool powerAtDir (uint16_t _X, uint16_t _Y, uint8_t _dir, bool _is_dead = false)
{
    if (_X < 0 || _Y < 0 || _X >= board_W || _Y >= board_H) { return false; }
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
    _X += xd;
    _Y += yd;
    char look = board[_X][_Y];
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
    if (board[_X][_Y+1] == PW_LEAKYB && _dir == NORTH) { return !_is_dead; }
    look = board[_X][_Y];
    bool is_next_to_adapter = nextToAdapter(_X, _Y);
    bool is_power_present = false, is_powered = false;
    if (look >= 50 && look <= 59) {
        is_power_present = true;
        is_powered = switches[look - 50] && !is_next_to_adapter;
    }
  //Return for if checking for dead
    if (_is_dead) {
        --diode;
        --wire;
        if (is_power_present) { return !is_powered; }  //There's a Switch
        if (!is_next_to_adapter) {
            if (look == PW_POWER) { return false; } //There's a Power
            if (look == PW_RANDOM) { return false; } //There's a Powered Random
            if (look == UN_RANDOM) { return true; } //There's a Random
        }
        if (look == UN_WIRE                             //
         || look == UN_BRIDGE || look == UN_LEAKYB      //
         || look == wire                                //
         || look == diode                               //
         || (_dir == NORTH && (look == UN_ADAPTER || (!is_next_to_adapter && (look == UN_BIT || look == UN_DELAY || look == UN_AND || look == UN_XOR || look == PW_NOT || look == U1_STRETCH)))) // There's a dead Wire/Bridge/LeakyB/D Wire/Diode/Bit/Delay/AND/XOR/NOT/Stretcher/Adapter
           ) { return true; }
        return false; //Anything else could never power us
    }
  //Return for if checking for alive
    return look == PW_WIRE                          //
        || look == wire                              //
        || (!is_next_to_adapter && (look == PW_POWER || look == PW_RANDOM)) //
        || look == diode                             // Powered by Wire/D Wire/Power/Random/Diode
        || (_dir == NORTH && (look == PW_ADAPTER || (!is_next_to_adapter && (look == PW_DELAY || look == PW_BIT || look == PW_AND || look == PW_XOR || look == UN_NOT || look == P1_STRETCH || look == P2_STRETCH || look == P3_STRETCH)))) // Powered from the North by Bit/Delay/AND/XOR/NOT/Stretcher/Adapter
        || (is_power_present && is_powered);      // Power
}


uint8_t nextToLives (uint16_t _X, uint16_t _Y, bool _false_on_dead = false)
{
    uint8_t ignore_dir = nextToAdapter(_X, _Y);
    if (!ignore_dir) { ignore_dir = SOUTH; }
    uint8_t lives = 0;
  //Check North
    if (ignore_dir != NORTH) {
        if (_false_on_dead && powerAtDir(_X, _Y, NORTH, true)) { return 0; }
        else if (powerAtDir(_X, _Y, NORTH)) { ++lives; }
    }
  //Check East
    if (ignore_dir != EAST) {
        if (_false_on_dead && powerAtDir(_X, _Y, EAST, true)) { return 0; }
        else if (powerAtDir(_X, _Y, EAST)) { ++lives; }
    }
  //Check South
    if (ignore_dir != SOUTH) {
        if (_false_on_dead && powerAtDir(_X, _Y, SOUTH, true)) { return 0; }
        else if (powerAtDir(_X, _Y, SOUTH)) { ++lives; }
    }
  //Check West
    if (ignore_dir != WEST) {
        if (_false_on_dead && powerAtDir(_X, _Y, WEST, true)) { return 0; }
        if (powerAtDir(_X, _Y, WEST)) { ++lives; }
    }
  //Return
    return lives;
}


uint16_t addBranch (uint16_t _X, uint16_t _Y, uint8_t prev_dir)
{
    branch[branches].x = _X;
    branch[branches].y = _Y;
    branch[branches].d = prev_dir; //0: none, NESW
    return branches++;
}

void powerAdapter (uint16_t _X, uint16_t _Y, uint8_t _default_DIR = SOUTH)
{
    bool is_found = false;
    char look;
    look = board[_X][_Y - 1];
    if (look == UN_ADAPTER || look == PW_ADAPTER) { addBranch(_X, _Y - 1, NORTH); is_found = true; }
    look = board[_X + 1][_Y];
    if (look == UN_ADAPTER || look == PW_ADAPTER) { addBranch(_X + 1, _Y, EAST);  is_found = true; }
    look = board[_X][_Y + 1];
    if (look == UN_ADAPTER || look == PW_ADAPTER) { addBranch(_X, _Y + 1, SOUTH); is_found = true; }
    look = board[_X - 1][_Y];
    if (look == UN_ADAPTER || look == PW_ADAPTER) { addBranch(_X - 1, _Y, WEST);  is_found = true; }
    if (!is_found) {
        switch (_default_DIR) {
            case NODIR: addBranch(_X, _Y, NODIR);     break;
            case NORTH: addBranch(_X, _Y - 1, NORTH); break;
            case EAST:  addBranch(_X + 1, _Y, EAST);  break;
            case SOUTH: addBranch(_X, _Y + 1, SOUTH); break;
            case WEST:  addBranch(_X - 1, _Y, WEST);  break;
        }
    }
}

uint16_t next_branch;
void elec () //Electrify the board appropriately
{
  //Reset the electrified
    for (uint16_t x = elec_X; x <= elec_X2; ++x) {
        for (uint16_t y = elec_Y; y <= elec_Y2; ++y) {
            char *look = &board[x][y];
            if      (*look == PW_WIRE)    { *look = UN_WIRE; }    //Powered Wire to Wire
            else if (*look == PW_H_WIRE)  { *look = UN_H_WIRE;  } //Powered H Wire to H Wire
            else if (*look == PW_V_WIRE)  { *look = UN_V_WIRE;  } //Powered V Wire to V Wire
            else if (*look == PW_N_DIODE) { *look = UN_N_DIODE; } //Powered N Diode to N Diode
            else if (*look == PW_E_DIODE) { *look = UN_E_DIODE; } //Powered E Diode to E Diode
            else if (*look == PW_S_DIODE) { *look = UN_S_DIODE; } //Powered S Diode to S Diode
            else if (*look == PW_W_DIODE) { *look = UN_W_DIODE; } //Powered W Diode to W Diode
            else if (*look == PW_BRIDGE)  { *look = UN_BRIDGE;  } //Powered Bridge to Bridge
            else if (*look == PW_LEAKYB)  { *look = UN_LEAKYB;  } //Powered Leaky Bridge to Leaky Bridge
            else if (*look == PW_ADAPTER) { *look = UN_ADAPTER; } //Powered Adapter to Adapter
        }
    }
  //Electrify cursor?
    if (to_elec_cursor) {
        to_elec_cursor = false;
        addBranch(cursor_X, cursor_Y, 0);
    }
  //Wires & inline components
    bool moved = true;
    while (moved) {
    
        moved = false;
        
        for (uint16_t b = 0; b < branches; ++b) {
            
            char *look = &board[branch[b].x][branch[b].y];
            if (*look == EMPTY || *look == UN_AND || *look == PW_AND|| *look == UN_NOT || *look == PW_NOT || *look == UN_XOR || *look == PW_XOR || *look == UN_DELAY || *look == PW_DELAY || *look == UN_WALL || *look == PW_BIT || *look == UN_DISPLAY || *look == PW_DISPLAY) { continue; }
            uint8_t *dir = &branch[b].d;
            bool is_bridge = (*look == UN_BRIDGE || *look == PW_BRIDGE || *look == UN_LEAKYB || *look == PW_LEAKYB);
            if (*look == UN_BIT && (*dir == EAST || *dir == WEST)) { continue; } //Stop at Unpowered Bit
            else if (*look == UN_ADAPTER) { *look = PW_ADAPTER; } //Electrify Adapter
            else if (*look == UN_WIRE)    { *look = PW_WIRE; }    //Electrify Wire
            else if (*look == UN_H_WIRE)  { *look = PW_H_WIRE; }  //Electrify H Wire
            else if (*look == UN_V_WIRE)  { *look = PW_V_WIRE; }  //Electrify V Wire
            else if (*look == E_WALL || is_bridge) { //If we've ended up in a Bridge, or Conductive Wall, continue in the direction we were going
              //Electrify & leak
                if (*look == UN_BRIDGE) { *look = PW_BRIDGE; }
                if (*look == UN_LEAKYB) { *look = PW_LEAKYB; }
                if (*look == PW_LEAKYB && *dir != SOUTH && *dir != NORTH) { addBranch(branch[b].x, branch[b].y + 1, SOUTH); }
              //Handle branch direction
                switch (*dir) {
                    case NODIR: continue; //We never had a direction (Bridge may have been placed on top of Power)
                    case NORTH: --branch[b].y; break;
                    case EAST:  ++branch[b].x; break;
                    case SOUTH: ++branch[b].y; break;
                    case WEST:  --branch[b].x; break;
                }
                --b;
                continue;
            }
            else if (*look == UN_N_DIODE || *look == PW_N_DIODE) { //Upon an N Diode, go North
                if (*dir == SOUTH) { continue; } //Trying to go past the diode
                *look = PW_N_DIODE; //Electrify
                --branch[b].y;
              //Check if there is a North diode yonder we can skip to
                char *here = &board[branch[b].x][branch[b].y];
                char *yonder = &board[branch[b].x][branch[b].y - 1];
                if ((*here == UN_E_DIODE || *here == PW_E_DIODE || *here == UN_W_DIODE || *here == PW_W_DIODE) && (*yonder == UN_N_DIODE || *yonder == PW_N_DIODE)) { addBranch(branch[b].x, branch[b].y - 1, NORTH); }
                *dir = NORTH; //Reset prev_dir
                --b;
                continue;
            }
            else if (*look == UN_E_DIODE ||*look == PW_E_DIODE) { //Upon an E Diode, go East
                if (*dir == WEST) { continue; } //Trying to go past the diode
                *look = PW_E_DIODE; //Electrify
                ++branch[b].x;
                *dir = EAST; //Reset prev_dir
                --b;
                continue;
            }
            else if (*look == UN_S_DIODE || *look == PW_S_DIODE) { //Upon an S Diode, go South
                if (*dir == NORTH) { continue; } //Trying to go past the diode
                *look = PW_S_DIODE; //Electrify
                ++branch[b].y;
              //Check if there is a South diode yonder we can skip to
                char *here = &board[branch[b].x][branch[b].y];
                char *yonder = &board[branch[b].x][branch[b].y + 1];
                if ((*here == UN_E_DIODE || *here == PW_E_DIODE || *here == UN_W_DIODE || *here == PW_W_DIODE) && (*yonder == UN_S_DIODE || *yonder == PW_S_DIODE)) { addBranch(branch[b].x, branch[b].y + 1, SOUTH); }
                *dir = SOUTH; //Reset prev_dir
                --b;
                continue;
            }
            else if (*look == UN_W_DIODE || *look == PW_W_DIODE) { //Upon a W Diode, go West
                if (*dir == EAST) { continue; } //Trying to go past the diode
                *look = PW_W_DIODE; //Electrify
                --branch[b].x;
                *dir = WEST; //Reset prev_dir
                --b;
                continue;
            }
            else if (*look == UN_BIT) { //If we've ended up in a Bit, activate it
                *look = PW_BIT;
                continue;
            }
            else if (*look >= U1_STRETCH && *look <= P3_STRETCH) { //Upon a Stretcher, instantly continue
                if (*look == U1_STRETCH ) {
                    powerAdapter(branch[b].x, branch[b].y);
                    --b;
                }
                *look = P3_STRETCH; //Electrify
                continue;
            }

          //Wires (electric branches)
            bool can_north = false, can_east = false, can_south = false, can_west = false, can_double_north = false, can_double_east = false, can_double_south = false, can_double_west = false;
            uint8_t routes = 0;
            auto isUnDiode  = [](char look) { return look == UN_N_DIODE || look == UN_E_DIODE || look == UN_S_DIODE || look == UN_W_DIODE; };
            auto isPwDWire  = [](char look) { return look == PW_V_WIRE || look == PW_H_WIRE; };
            auto isXBridge = [](char look) { return look == UN_BRIDGE || look == PW_BRIDGE; };
            auto isXLeakyB = [](char look) { return look == UN_LEAKYB || look == PW_LEAKYB; };
            char our_pos = board[branch[b].x][branch[b].y];
            bool can_V = (our_pos != UN_H_WIRE && our_pos != PW_H_WIRE);
            bool can_H = (our_pos != UN_V_WIRE && our_pos != PW_V_WIRE);
          //North
            if (can_V && *dir != SOUTH)                 {
                char *look = &board[branch[b].x][branch[b].y - 1];
                if (*look == UN_WIRE || *look == UN_N_DIODE)    { ++routes; can_north = true; }
                else if (*look == UN_V_WIRE)                    { ++routes; can_north = true; }
                else if (isXBridge(*look))                      { ++routes; can_double_north = true; *look = PW_BRIDGE; }
                else if (isXLeakyB(*look))                      { ++routes; can_north = true; *look = PW_LEAKYB; }
            }
          //East
            if (can_H && *dir != WEST)                  {
                char *look = &board[branch[b].x + 1][branch[b].y];
                if (*look == UN_WIRE || *look == UN_E_DIODE)    { ++routes; can_east = true; }
                else if (*look == UN_H_WIRE)                    { ++routes; can_east = true; }
                else if (isXBridge(*look))                      { ++routes; can_double_east = true; *look = PW_BRIDGE; }
                else if (isXLeakyB(*look))                      { ++routes; can_east = true; *look = PW_LEAKYB; }
            }
          //South
            if (can_V && *dir != NORTH && branch[b].y)  {
                char *look = &board[branch[b].x][branch[b].y + 1];
                if (*look == UN_WIRE || *look == UN_S_DIODE || *look == UN_BIT || *look == U1_STRETCH || *look == UN_DELAY) { ++routes; can_south = true; }
                else if (*look == UN_V_WIRE)                    { ++routes; can_south = true; }
                else if (isXBridge(*look))                      { ++routes; can_double_south = true; *look = PW_BRIDGE; }
                else if (isXLeakyB(*look))                      { ++routes; can_south = true; *look = PW_LEAKYB; }
            }
          //West
            if (can_H && *dir != EAST)                  {
                char *look = &board[branch[b].x - 1][branch[b].y];
                if (*look == UN_WIRE || *look == UN_W_DIODE)    { ++routes; can_west = true; }
                else if (*look == UN_H_WIRE)                    { ++routes; can_west = true; }
                else if (isXBridge(*look))                      { ++routes; can_double_west = true; *look = PW_BRIDGE; }
                else if (isXLeakyB(*look))                      { ++routes; can_west = true; *look = PW_LEAKYB; }
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
                    else if (can_double_north) { branch[next_branch].y -= 2;    branch[next_branch].d = NORTH;    can_double_north = false; }
                    else if (can_double_east)  { branch[next_branch].x += 2;    branch[next_branch].d = EAST;     can_double_east = false; }
                    else if (can_double_south) { branch[next_branch].y += 2;    branch[next_branch].d = SOUTH;    can_double_south = false; }
                    else if (can_double_west)  { branch[next_branch].x -= 2;    branch[next_branch].d = WEST;     can_double_west = false; }
                    --routes;
                } while (routes > 0);
            }
        }
    }
    memset(branch, 0, sizeof(branch)); //Remove all existing branches
    branches = 0;
  //Components
    for (uint16_t x = elec_X; x <= elec_X2; ++x) {
        for (uint16_t y = elec_Y2; y >= elec_Y; --y) {  //Evaluate NORTH, so recently changed components aren't re-evaluated
            switch (board[x][y]) {
                case EMPTY: //Empty
                    continue;
                case UN_AND: //AND
                case PW_AND: //Powered AND
                {
                    board[x][y] = UN_AND;
                    bool is_line = false;
                    uint16_t line_east, line_west;
                  //Seek across potential line of AND's
                    uint16_t tx = x + 1;
                    while (board[tx][y] == UN_AND || board[tx][y] == PW_AND) {
                        ++tx;
                        is_line = true;
                    }
                    line_east = tx - 1;
                    tx = x - 1;
                    while (board[tx][y] == UN_AND || board[tx][y] == PW_AND) {
                        --tx;
                        is_line = true;
                    }
                    line_west = tx + 1;
                    if (is_line) {
                        uint8_t alive = 0, dead = 0;
                        alive += powerAtDir(line_east, y, EAST);
                        alive += powerAtDir(line_west, y, WEST);
                        dead  += powerAtDir(line_east, y, EAST, true);
                        dead  += powerAtDir(line_west, y, WEST, true);
                        char above = board[x][y - 1];
                        bool is_adapter_above = (above == UN_ADAPTER || above == PW_ADAPTER);
                        bool is_powered = (is_adapter_above ? powerAtDir(x, y, SOUTH) : powerAtDir(x, y, NORTH));
                        if (alive && !dead && is_powered) {
                            if (is_adapter_above) { addBranch(x, y - 1, NORTH); } else { addBranch(x, y + 1, SOUTH); }
                            board[x][y] = PW_AND;
                        }
                    } else {
                        if (nextToLives(x, y, true) > 1) {
                            powerAdapter(x, y);
                            board[x][y] = PW_AND;
                        }
                    }
                    break;
                }
                case UN_NOT: //NOT
                case PW_NOT: //Powered NOT
                    if (!nextToLives(x, y)) {
                        board[x][y] = UN_NOT;
                        powerAdapter(x, y);
                    } else {
                        board[x][y] = PW_NOT;
                    }
                    break;
                case UN_XOR: //XOR
                case PW_XOR: //Powered XOR
                    if (nextToLives(x, y) == 1) {
                        board[x][y] = PW_XOR;
                        powerAdapter(x, y);
                    } else {
                        board[x][y] = UN_XOR;
                    }
                    break;
                case UN_DELAY: //Delay
                case PW_DELAY: //Powered Delay
                    if (nextToLives(x, y)) {
                        board[x][y] = PW_DELAY;
                        powerAdapter(x, y);
                    } else {
                        board[x][y] = UN_DELAY;
                    }
                    break;
                case UN_RANDOM: //Random
                case PW_RANDOM: //Powered Random
                    if (rand() % 2) { board[x][y] = UN_RANDOM; break; }
                    board[x][y] = PW_RANDOM; //Electrify
                case PW_POWER: //Power
                    powerAdapter(x, y, NODIR);
                    break;
                case PW_BIT: //Powered Bit
                {
                  //Check if being reset
                  //Seek horiz RIGHT across a potential line of Bits
                    uint16_t tx = x;
                    while (board[tx][y] == UN_BIT || board[tx][y] == PW_BIT) {
                        ++tx;
                    }
                    if (powerAtDir(tx - 1, y, EAST)) {
                        board[x][y] = UN_BIT;
                    } else {
                      //Seek horiz LEFT across a potential line of Bits
                        uint16_t tx = x;
                        while (board[tx][y] == UN_BIT || board[tx][y] == PW_BIT) {
                            --tx;
                        }
                        if (powerAtDir(tx + 1, y, WEST)) {
                            board[x][y] = UN_BIT;
                        } else {
                            if (board[x][y] == PW_BIT) { //If already set
                                powerAdapter(x, y);
                            }
                        }
                    }
                    break;
                }
                case P1_STRETCH: //
                case P2_STRETCH: //
                case P3_STRETCH: //Powered Stretcher
                  //Check if we should be unpowered
                    if (!powerAtDir(x, y, NORTH) || powerAtDir(x, y, NORTH, true)) {
                        --board[x][y];
                    }
                    if (board[x][y] >= P1_STRETCH) {
                        powerAdapter(x, y);
                    }
                    break;
                case UN_DISPLAY: //Display
                case PW_DISPLAY: //Powered Display
                {
                    bool did_scroll = false;
                  //Check if we should scroll (from WEST flank)
                    uint16_t tx = x;
                    while (board[tx][y] == UN_DISPLAY || board[tx][y] == PW_DISPLAY) {
                        --tx;
                    }
                    if (powerAtDir(tx + 1, y, WEST)) {
                        did_scroll = true;
                        if (board[x - 1][y] == UN_DISPLAY) {
                            board[x - 1][y] = board[x][y];
                        } else {
                            board[x][y] = UN_DISPLAY;
                        }
                    }
                  //Check if we should be powered (from NORTH and EAST flank)
                    //Seek vert NORTH across a potential line of Display
                    uint16_t ty = y;
                    while (board[x][ty] == UN_DISPLAY || board[x][ty] == PW_DISPLAY) {
                        --ty;
                    }
                    //Seek horz EAST across a potential line of Display
                    tx = x;
                    while (board[tx][y] == UN_DISPLAY || board[tx][y] == PW_DISPLAY) {
                        ++tx;
                    }
                    if (powerAtDir(x, ty + 1, NORTH) && powerAtDir(tx - 1, y, EAST)) {
                        board[x][y] = PW_DISPLAY;
                    } else if (did_scroll) {
                        board[x][y] = UN_DISPLAY;
                    }
                }
                    break;
                default:
                  //Switches
                    if (board[x][y] >= 50 && board[x][y] <= 59) {
                        if (switches[board[x][y] - 50]) {
                            powerAdapter(x, y, NODIR);
                        }
                    }
                    break;
            }
        }
    }
}
