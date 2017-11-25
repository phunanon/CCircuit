#include <sys/time.h> // For time
#include <termios.h>  //
#include <stdlib.h>   //
#include <string.h>   // For key presses


//===================================
//For keypresses
//http://ubuntuforums.org/showthread.php?t=554845
//===================================
static struct termios g_old_kbd_mode;
static char pressed_ch;

//Check if a key has been pressed
static int kbhit (void)
{
    struct timeval timeout;
    fd_set read_handles;

    //Check stdin (fd 0) for activity
    FD_ZERO(&read_handles);
    FD_SET(0, &read_handles);
    timeout.tv_sec = timeout.tv_usec = 0;
  //Return status
    return select(1, &read_handles, NULL, NULL, &timeout);
}

//Restore terminal state
static void old_attr (void)
{
    tcsetattr(0, TCSANOW, &g_old_kbd_mode);
}

//Prepare to listen to key presses
static void loadKeyListen (void)
{
    struct termios new_kbd_mode;
  //Set stdin to raw, unbuffered mode
    tcgetattr(0, &g_old_kbd_mode);
    memcpy(&new_kbd_mode, &g_old_kbd_mode, sizeof(struct termios));
    
    new_kbd_mode.c_lflag &= ~(ICANON | ECHO);
    new_kbd_mode.c_cc[VTIME] = 0;
    new_kbd_mode.c_cc[VMIN] = 1;
    tcsetattr(0, TCSANOW, &new_kbd_mode);
    atexit(old_attr);
}
//===================================
