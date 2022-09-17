#ifdef _WIN32
# include <windows.h>
# include <direct.h>
# include <conio.h>
# define getch _getch
# define kbhit _kbhit
#else
# include <termios.h>
# include <unistd.h>
# include <sys/ioctl.h> // for getkey
# include <sys/types.h> // for kbhit()
# include <sys/time.h>  // for kbhit()
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ansicon.h"

#ifdef _WIN32

static HANDLE _hStdOut, _hStdIn;
static DWORD _dwModeOutSave, _dwModeInSave; // for restore

int ansicon_setup(void) 
{
    DWORD dwModeOut = 0, dwModeIn = 0;
    _hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    _hStdIn = GetStdHandle(STD_INPUT_HANDLE);

    if (_hStdOut == INVALID_HANDLE_VALUE || _hStdIn == INVALID_HANDLE_VALUE)
    {
        return -1;
    }
    
    if (!GetConsoleMode(_hStdOut, &dwModeOut) || !GetConsoleMode(_hStdIn, &dwModeIn)) 
    {
        return -1;
    }

    _dwModeOutSave = dwModeOut;
    _dwModeInSave = dwModeIn;
    
    // Enable ANSI escape codes
    dwModeOut |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;

    // Set stdin as no echo and unbuffered
    dwModeIn &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT);

    if (!SetConsoleMode(_hStdOut, dwModeOut) || !SetConsoleMode(_hStdIn, dwModeIn)) 
    {
        return -1;
    }    

    return 0;
}


int ansicon_restore(void)
{
    // Reset colors
    fputs(ANSI_ATTRIBUTE_RESET, stdout);
    // Reset console mode
    if (!SetConsoleMode(_hStdOut, _dwModeOutSave) || !SetConsoleMode(_hStdIn, _dwModeInSave)) 
    {
        return -1;
    }
    return 0;
}

#else

static struct termios orig_term, new_term;

int ansicon_setup(void) 
{
    tcgetattr(STDIN_FILENO, &orig_term);
    new_term = orig_term;
    new_term.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_term);
    return 0;
}


int ansicon_restore(void)
{
    // Reset colors
    fputs(ANSI_ATTRIBUTE_RESET, stdout);
    // Reset console mode
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_term);
    return 0;
}



/**
 * @brief Get a charater without waiting on a Return
 * @details Windows has this functionality in conio.h
 * @return The character
 */
int getch(void)
{
	struct termios oldt, newt;
	int ch;
	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	ch = getchar();
	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	return ch;
}


/**
 * @brief Determines if a button was pressed.
 * @details Windows has this in conio.h
 * @return Number of characters read
 */
int
kbhit(void)
{
	static struct termios oldt, newt;
	int cnt = 0;
	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag    &= ~(ICANON | ECHO);
	newt.c_iflag     = 0; /* input mode */
	newt.c_oflag     = 0; /* output mode */
	newt.c_cc[VMIN]  = 1; /* minimum time to wait */
	newt.c_cc[VTIME] = 1; /* minimum characters to wait for */
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	ioctl(0, FIONREAD, &cnt); /* Read count */
	struct timeval tv;
	tv.tv_sec  = 0;
	tv.tv_usec = 100;
	select(STDIN_FILENO+1, NULL, NULL, NULL, &tv); /* A small time delay */
	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	return cnt; /* Return number of characters */
}

#endif

void ansicon_show_cursor(void)
{
    fputs(ANSI_CURSOR_SHOW, stdout);
}


void ansicon_hide_cursor(void)
{
    fputs(ANSI_CURSOR_HIDE, stdout);
}


void ansicon_puts(const char *color, const char *str)
{
    if (color) fputs(color, stdout);
    if (str) fputs(str, stdout);
    fputs(ANSI_ATTRIBUTE_RESET, stdout);
    fflush(stdout);
}


void ansicon_printf(const char *color, const char *fmt, ...)
{
    va_list args;
	va_start(args, fmt);
	if (color) fputs(color, stdout);
    vprintf(fmt, args);
	va_end(args);
	fputs(ANSI_ATTRIBUTE_RESET, stdout);
    fflush(stdout);
}


// Print string without advance cursor
int ansicon_set_string(const char *color, const char *str)
{
    unsigned int len = (unsigned int)strlen(str);
    if (len == 0) return 0;

    if (color) fputs(color, stdout);
    fputs(str, stdout);
    printf("\033[%uD", len);
    fflush(stdout);
    return len;
}


// move cursor right by pos
void ansicon_move_cursor_right(int pos) 
{
    printf("\033[%dC", pos);
}


int ansicon_getch_non_blocking(void)
{
	if (kbhit()) 
        return getch();
	else
        return 0;
}
