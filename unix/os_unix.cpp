#include "port.h"
#include "prototyp.h"

#include "cmplx.h"
#include "drivers.h"
#include "id.h"
#include "mpmath.h"
#include "prompts2.h"

#include <dirent.h>
#include <sys/stat.h>
#include <sys/statvfs.h>

#include <cassert>
#include <cstring>
#include <string>

// Global variables (yuck!)
int g_hi_attenuation = 0;
long g_l_init_x = 0;
long g_l_init_y = 0;
long g_save_base = 0;              // base clock ticks
long g_save_ticks = 0;             // save after this many ticks

/* g_video_table
 *
 *  |--Adapter/Mode-Name------|-------Comments-----------|
 *  |------INT 10H------|Dot-|--Resolution---|
 *  |key|--AX---BX---CX---DX|Mode|--X-|--Y-|Color|
 */
VIDEOINFO g_video_table[MAX_VIDEO_MODES]{};

// Global variables that should be phased out (old video mode stuff)
int g_video_vram = 0;

unsigned long get_disk_space()
{
    // TODO
    return 0x7FFFFFFF;
}

void init_failure(char const *message)
{
    std::fputs(message, stderr);
}

extern void (*dotwrite)(int, int, int); // write-a-dot routine
extern int (*dotread)(int, int);    // read-a-dot routine
extern void (*linewrite)(int y, int x, int lastx, BYTE *pixels);     // write-a-line routine
extern void (*lineread)(int y, int x, int lastx, BYTE *pixels);      // read-a-line routine

#if defined(USE_DRIVER_FUNCTIONS)
void set_normal_dot()
{
    dotwrite = driver_write_pixel;
    dotread = driver_read_pixel;
}
#else
static void driver_dot_write(int x, int y, int color)
{
    driver_write_pixel(x, y, color);
}

static int driver_dot_read(int x, int y)
{
    return driver_read_pixel(x, y);
}

void set_normal_dot()
{
    dotwrite = driver_dot_write;
    dotread = driver_dot_read;
}
#endif

void normaline(int y, int x, int lastx, BYTE *pixels)
{
    int width = lastx - x + 1;
    assert(dotwrite);
    for (int i = 0; i < width; i++)
    {
        (*dotwrite)(x + i, y, pixels[i]);
    }
}

void normalineread(int y, int x, int lastx, BYTE *pixels)
{
    int width = lastx - x + 1;
    assert(dotread);
    for (int i = 0; i < width; i++)
    {
        pixels[i] = (*dotread)(x + i, y);
    }
}

void set_normal_line()
{
    lineread = normalineread;
    linewrite = normaline;
}
