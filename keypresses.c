#include <sys/time.h> //For time
#include <termios.h> //For key presses
#include <stdlib.h> //For key presses
#include <string.h> //For key presses


//===================================
//For keypresses
//http://ubuntuforums.org/showthread.php?t=554845
//===================================
static struct termios g_old_kbd_mode;
static char pressedCh;

// did somebody press somthing???
static int kbhit(void){
    struct timeval timeout;
    fd_set read_handles;

    // check stdin (fd 0) for activity
    FD_ZERO(&read_handles);
    FD_SET(0, &read_handles);
    timeout.tv_sec = timeout.tv_usec = 0;
    return select(1, &read_handles, NULL, NULL, &timeout); //Return status
}

// put the things as they were befor leave..!!!
static void old_attr(void){
    tcsetattr(0, TCSANOW, &g_old_kbd_mode);
}

static void loadKeyListen(void) //For listening to keys pressed
{
    struct termios new_kbd_mode;
    // put keyboard (stdin, actually) in raw, unbuffered mode
    tcgetattr(0, &g_old_kbd_mode);
    memcpy(&new_kbd_mode, &g_old_kbd_mode, sizeof(struct termios));
    
    new_kbd_mode.c_lflag &= ~(ICANON | ECHO);
    new_kbd_mode.c_cc[VTIME] = 0;
    new_kbd_mode.c_cc[VMIN] = 1;
    tcsetattr(0, TCSANOW, &new_kbd_mode);
    atexit(old_attr);
}
//===================================
