#pragma once


#ifdef __cplusplus
extern "C" {
#endif

static const char * ANSI_CLS                = "\033[2J\033[3J";
static const char * ANSI_ATTRIBUTE_RESET    = "\033[0m";
static const char * ANSI_CURSOR_HIDE        = "\033[?25l";
static const char * ANSI_CURSOR_SHOW        = "\033[?25h";
static const char * ANSI_CURSOR_HOME        = "\033[H";
static const char * ANSI_BLACK              = "\033[22;30m";
static const char * ANSI_RED                = "\033[22;31m";
static const char * ANSI_GREEN              = "\033[22;32m";
static const char * ANSI_BROWN              = "\033[22;33m";
static const char * ANSI_BLUE               = "\033[22;34m";
static const char * ANSI_MAGENTA            = "\033[22;35m";
static const char * ANSI_CYAN               = "\033[22;36m";
static const char * ANSI_GREY               = "\033[22;37m";
static const char * ANSI_DARKGREY           = "\033[01;30m";
static const char * ANSI_LIGHTRED           = "\033[01;31m";
static const char * ANSI_LIGHTGREEN         = "\033[01;32m";
static const char * ANSI_YELLOW             = "\033[01;33m";
static const char * ANSI_LIGHTBLUE          = "\033[01;34m";
static const char * ANSI_LIGHTMAGENTA       = "\033[01;35m";
static const char * ANSI_LIGHTCYAN          = "\033[01;36m";
static const char * ANSI_WHITE              = "\033[01;37m";
static const char * ANSI_BACKGROUND_BLACK   = "\033[40m";
static const char * ANSI_BACKGROUND_RED     = "\033[41m";
static const char * ANSI_BACKGROUND_GREEN   = "\033[42m";
static const char * ANSI_BACKGROUND_YELLOW  = "\033[43m";
static const char * ANSI_BACKGROUND_BLUE    = "\033[44m";
static const char * ANSI_BACKGROUND_MAGENTA = "\033[45m";
static const char * ANSI_BACKGROUND_CYAN    = "\033[46m";
static const char * ANSI_BACKGROUND_WHITE   = "\033[47m";


int ansicon_setup(void);
int ansicon_restore(void);
void ansicon_show_cursor(void);
void ansicon_hide_cursor(void);
void ansicon_puts(const char *color, const char *str);
void ansicon_printf(const char *color, const char *fmt, ...);
int ansicon_set_string(const char *color, const char *str);
void ansicon_move_cursor_right(int pos);
int ansicon_getch_non_blocking(void);

#ifdef __cplusplus
}
#endif