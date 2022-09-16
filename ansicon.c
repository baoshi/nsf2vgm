#ifdef _WIN32
# include <windows.h>
#else
# include <termios.h>
# include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>

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
    printf(ANSI_ATTRIBUTE_RESET);    
    // Reset console mode
    if (!SetConsoleMode(_hStdOut, _dwModeOutSave) || !SetConsoleMode(_hStdIn, _dwModeInSave)) 
    {
        return -1;
    }
    return 0;
}

#else


int ansicon_setup(void) 
{
    tcgetattr(STDIN_FILENO, &orig_term);
    new_term = orig_term;
    new_term.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_term);
}


int ansicon_restore(void)
{
    // Reset colors
    printf(ANSI_ATTRIBUTE_RESET);
    // Reset console mode
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_term);
}

#endif

