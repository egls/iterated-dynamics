/*
        Overlayed odds and ends that don't fit anywhere else.
*/
#include "port.h"
#include "prototyp.h"

#include "miscovl.h"

#include "biginit.h"
#include "calcfrac.h"
#include "cmdfiles.h"
#include "convert_center_mag.h"
#include "drivers.h"
#include "find_path.h"
#include "fractalp.h"
#include "framain2.h"
#include "full_screen_choice.h"
#include "get_key_no_help.h"
#include "helpdefs.h"
#include "id_data.h"
#include "load_config.h"
#include "rotate.h"
#include "stop_msg.h"
#include "video_mode.h"

#include <algorithm>
#include <array>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

static int check_modekey(int curkey, int choice);
static bool ent_less(int lhs, int rhs);
static void update_id_cfg();

bool g_is_true_color = false;

// JIIM

#define PAR_KEY(x)  ( x < 10 ? '0' + x : 'a' - 10 + x)

inline bool is_writable(const std::string &path)
{
    const fs::perms read_write = fs::perms::owner_read | fs::perms::owner_write;
    return (fs::status(path).permissions() & read_write) == read_write;
}

static std::array<int, MAX_VIDEO_MODES> entnums;
static bool modes_changed = false;

static void format_vid_table(int choice, char *buf)
{
    char local_buf[81];
    char kname[5];
    int truecolorbits;
    std::memcpy((char *)&g_video_entry, (char *)&g_video_table[entnums[choice]],
           sizeof(g_video_entry));
    vidmode_keyname(g_video_entry.keynum, kname);
    std::sprintf(buf, "%-5s %-25s %5d %5d ",  // 44 chars
            kname, g_video_entry.name, g_video_entry.xdots, g_video_entry.ydots);
    truecolorbits = g_video_entry.dotmode/1000;
    if (truecolorbits == 0)
    {
        std::snprintf(local_buf, NUM_OF(local_buf), "%s%3d",  // 47 chars
                buf, g_video_entry.colors);
    }
    else
    {
        std::snprintf(local_buf, NUM_OF(local_buf), "%s%3s",  // 47 chars
                buf, (truecolorbits == 4)?" 4g":
                (truecolorbits == 3)?"16m":
                (truecolorbits == 2)?"64k":
                (truecolorbits == 1)?"32k":"???");
    }
    std::sprintf(buf, "%s %.12s %.12s",  // 74 chars
            local_buf, g_video_entry.driver->name, g_video_entry.comment);
}

int select_video_mode(int curmode)
{
    int attributes[MAX_VIDEO_MODES];
    int ret;

    for (int i = 0; i < g_video_table_len; ++i)  // init tables
    {
        entnums[i] = i;
        attributes[i] = 1;
    }
    std::sort(entnums.begin(), entnums.end(), ent_less);

    // pick default mode
    if (curmode < 0)
    {
        g_video_entry.videomodeax = 19;  // vga
        g_video_entry.colors = 256;
    }
    else
    {
        std::memcpy((char *) &g_video_entry, (char *) &g_video_table[curmode], sizeof(g_video_entry));
    }
    int i;
    for (i = 0; i < g_video_table_len; ++i)  // find default mode
    {
        if (g_video_entry.videomodeax == g_video_table[entnums[i]].videomodeax
            && g_video_entry.colors == g_video_table[entnums[i]].colors
            && (curmode < 0
                || std::memcmp((char *) &g_video_entry, (char *) &g_video_table[entnums[i]], sizeof(g_video_entry)) == 0))
        {
            break;
        }
    }
    if (i >= g_video_table_len) // no match, default to first entry
    {
        i = 0;
    }

    bool const old_tab_mode = g_tab_mode;
    help_labels const old_help_mode = g_help_mode;
    modes_changed = false;
    g_tab_mode = false;
    g_help_mode = help_labels::HELPVIDSEL;
    i = fullscreen_choice(CHOICE_HELP,
                          "Select Video Mode",
                          "key...name.......................xdot..ydot.colr.driver......comment......",
                          nullptr, g_video_table_len, nullptr, attributes,
                          1, 16, 74, i, format_vid_table, nullptr, nullptr, check_modekey);
    g_tab_mode = old_tab_mode;
    g_help_mode = old_help_mode;
    if (i == -1)
    {
        // update id.cfg for new key assignments
        if (modes_changed
            && g_bad_config == config_status::OK
            && stopmsg(STOPMSG_CANCEL | STOPMSG_NO_BUZZER | STOPMSG_INFO_ONLY,
                "Save new function key assignments or cancel changes?") == 0)
        {
            update_id_cfg();
        }
        return -1;
    }
    // picked by function key or ENTER key
    i = (i < 0) ? (-1 - i) : entnums[i];
    // the selected entry now in g_video_entry
    std::memcpy((char *) &g_video_entry, (char *) &g_video_table[i], sizeof(g_video_entry));

    // copy id.cfg table to resident table, note selected entry
    int k = 0;
    for (i = 0; i < g_video_table_len; ++i)
    {
        if (g_video_table[i].keynum > 0)
        {
            if (std::memcmp((char *)&g_video_entry, (char *)&g_video_table[i], sizeof(g_video_entry)) == 0)
            {
                k = g_video_table[i].keynum;
            }
        }
    }
    ret = k;
    if (k == 0)  // selected entry not a copied (assigned to key) one
    {
        std::memcpy((char *)&g_video_table[MAX_VIDEO_MODES-1],
               (char *)&g_video_entry, sizeof(*g_video_table));
        ret = 1400; // special value for check_vidmode_key
    }

    // update id.cfg for new key assignments
    if (modes_changed && g_bad_config == config_status::OK)
    {
        update_id_cfg();
    }

    return ret;
}

static int check_modekey(int curkey, int choice)
{
    int i = check_vidmode_key(1, curkey);
    if (i >= 0)
    {
        return -1-i;
    }
    i = entnums[choice];
    int ret = 0;
    if ((curkey == '-' || curkey == '+')
        && (g_video_table[i].keynum == 0 || g_video_table[i].keynum >= 1084))
    {
        if (g_bad_config != config_status::OK)
        {
            stopmsg(STOPMSG_NONE, "Missing or bad id.cfg file. Can't reassign keys.");
        }
        else
        {
            if (curkey == '-')
            {
                // deassign key?
                if (g_video_table[i].keynum >= 1084)
                {
                    g_video_table[i].keynum = 0;
                    modes_changed = true;
                }
            }
            else
            {
                // assign key?
                int j = getakeynohelp();
                if (j >= 1084 && j <= 1113)
                {
                    for (int k = 0; k < g_video_table_len; ++k)
                    {
                        if (g_video_table[k].keynum == j)
                        {
                            g_video_table[k].keynum = 0;
                            ret = -1; // force redisplay
                        }
                    }
                    g_video_table[i].keynum = j;
                    modes_changed = true;
                }
            }
        }
    }
    return ret;
}

static bool ent_less(const int lhs, const int rhs)
{
    int i = g_video_table[lhs].keynum;
    if (i == 0)
    {
        i = 9999;
    }
    int j = g_video_table[rhs].keynum;
    if (j == 0)
    {
        j = 9999;
    }
    return i < j || i == j && lhs < rhs;
}

static void update_id_cfg()
{
#ifndef XFRACT
    char buf[121], kname[5];
    std::FILE *cfgfile, *outfile;
    int i, j, linenum, nextlinenum, nextmode;
    VIDEOINFO vident;

    const std::string cfgname = find_path("id.cfg");

    if (!is_writable(cfgname))
    {
        std::snprintf(buf, NUM_OF(buf), "Can't write %s", cfgname.c_str());
        stopmsg(STOPMSG_NONE, buf);
        return;
    }
    const std::string outname{(fs::path{cfgname}.parent_path() / "id.tmp").string()};
    outfile = std::fopen(outname.c_str(), "w");
    if (outfile == nullptr)
    {
        std::snprintf(buf, NUM_OF(buf), "Can't create %s", outname.c_str());
        stopmsg(STOPMSG_NONE, buf);
        return;
    }
    cfgfile = std::fopen(cfgname.c_str(), "r");

    nextmode = 0;
    linenum = nextmode;
    nextlinenum = g_cfg_line_nums[0];
    while (std::fgets(buf, NUM_OF(buf), cfgfile))
    {
        char colorsbuf[10];
        ++linenum;
        if (linenum == nextlinenum)
        {
            // replace this line
            std::memcpy((char *)&vident, (char *)&g_video_table[nextmode],
                   sizeof(g_video_entry));
            vidmode_keyname(vident.keynum, kname);
            std::strcpy(buf, vident.name);
            i = (int) std::strlen(buf);
            while (i && buf[i-1] == ' ') // strip trailing spaces to compress
            {
                --i;
            }
            j = i + 5;
            while (j < 32)
            {
                // tab to column 33
                buf[i++] = '\t';
                j += 8;
            }
            buf[i] = 0;
            int truecolorbits = vident.dotmode/1000;
            if (truecolorbits == 0)
            {
                std::snprintf(colorsbuf, NUM_OF(colorsbuf), "%3d", vident.colors);
            }
            else
            {
                std::snprintf(colorsbuf, NUM_OF(colorsbuf), "%3s",
                        (truecolorbits == 4)?" 4g":
                        (truecolorbits == 3)?"16m":
                        (truecolorbits == 2)?"64k":
                        (truecolorbits == 1)?"32k":"???");
            }
            std::fprintf(outfile, "%-4s,%s,%4x,%4x,%4x,%4x,%4d,%5d,%5d,%s,%s\n",
                    kname,
                    buf,
                    vident.videomodeax,
                    vident.videomodebx,
                    vident.videomodecx,
                    vident.videomodedx,
                    vident.dotmode%1000, // remove true-color flag, keep g_text_safe
                    vident.xdots,
                    vident.ydots,
                    colorsbuf,
                    vident.comment);
            if (++nextmode >= g_video_table_len)
            {
                nextlinenum = 32767;
            }
            else
            {
                nextlinenum = g_cfg_line_nums[nextmode];
            }
        }
        else
        {
            std::fputs(buf, outfile);
        }
    }

    std::fclose(cfgfile);
    std::fclose(outfile);
    fs::remove(cfgname);         // success assumed on these lines
    fs::rename(outname, cfgname); // since we checked earlier with access
#endif
}

/* make_mig() takes a collection of individual GIF images (all
   presumably the same resolution and all presumably generated
   by Fractint and its "divide and conquer" algorithm) and builds
   a single multiple-image GIF out of them.  This routine is
   invoked by the "batch=stitchmode/x/y" option, and is called
   with the 'x' and 'y' parameters
*/

void make_mig(unsigned int xmult, unsigned int ymult)
{
    unsigned int xres, yres;
    unsigned int allxres, allyres, xtot, ytot;
    unsigned int xloc, yloc;
    unsigned char ichar;
    unsigned int allitbl, itbl;
    unsigned int i;
    char gifin[15], gifout[15];
    int errorflag, inputerrorflag;
    unsigned char *temp;
    std::FILE *out, *in;

    errorflag = 0;                          // no errors so
    inputerrorflag = 0;
    allitbl = 0;
    allyres = allitbl;
    allxres = allyres;
    in = nullptr;
    out = in;

    std::strcpy(gifout, "fractmig.gif");

    temp = &g_old_dac_box[0][0];                 // a safe place for our temp data

    g_gif87a_flag = true;                     // for now, force this

    // process each input image, one at a time
    for (unsigned ystep = 0U; ystep < ymult; ystep++)
    {
        for (unsigned xstep = 0U; xstep < xmult; xstep++)
        {
            if (xstep == 0 && ystep == 0)          // first time through?
            {
                std::printf(" \n Generating multi-image GIF file %s using", gifout);
                std::printf(" %u X and %u Y components\n\n", xmult, ymult);
                // attempt to create the output file
                out = std::fopen(gifout, "wb");
                if (out == nullptr)
                {
                    std::printf("Cannot create output file %s!\n", gifout);
                    exit(1);
                }
            }

            std::snprintf(gifin, NUM_OF(gifin), "frmig_%c%c.gif", PAR_KEY(xstep), PAR_KEY(ystep));

            in = std::fopen(gifin, "rb");
            if (in == nullptr)
            {
                std::printf("Can't open file %s!\n", gifin);
                exit(1);
            }

            // (read, but only copy this if it's the first time through)
            if (std::fread(temp, 13, 1, in) != 1)   // read the header and LDS
            {
                inputerrorflag = 1;
            }
            std::memcpy(&xres, &temp[6], 2);     // X-resolution
            std::memcpy(&yres, &temp[8], 2);     // Y-resolution

            if (xstep == 0 && ystep == 0)  // first time through?
            {
                allxres = xres;             // save the "master" resolution
                allyres = yres;
                xtot = xres * xmult;        // adjust the image size
                ytot = yres * ymult;
                std::memcpy(&temp[6], &xtot, 2);
                std::memcpy(&temp[8], &ytot, 2);
                if (g_gif87a_flag)
                {
                    temp[3] = '8';
                    temp[4] = '7';
                    temp[5] = 'a';
                }
                temp[12] = 0; // reserved
                if (std::fwrite(temp, 13, 1, out) != 1)     // write out the header
                {
                    errorflag = 1;
                }
            }                           // end of first-time-through

            ichar = (char)(temp[10] & 0x07);        // find the color table size
            itbl = 1 << (++ichar);
            ichar = (char)(temp[10] & 0x80);        // is there a global color table?
            if (xstep == 0 && ystep == 0)   // first time through?
            {
                allitbl = itbl;             // save the color table size
            }
            if (ichar != 0)                // yup
            {
                // (read, but only copy this if it's the first time through)
                if (std::fread(temp, 3*itbl, 1, in) != 1)    // read the global color table
                {
                    inputerrorflag = 2;
                }
                if (xstep == 0 && ystep == 0)       // first time through?
                {
                    if (std::fwrite(temp, 3*itbl, 1, out) != 1)     // write out the GCT
                    {
                        errorflag = 2;
                    }
                }
            }

            if (xres != allxres || yres != allyres || itbl != allitbl)
            {
                // Oops - our pieces don't match
                std::printf("File %s doesn't have the same resolution as its predecessors!\n", gifin);
                exit(1);
            }

            while (true)                       // process each information block
            {
                std::memset(temp, 0, 10);
                if (std::fread(temp, 1, 1, in) != 1)    // read the block identifier
                {
                    inputerrorflag = 3;
                }

                if (temp[0] == 0x2c)           // image descriptor block
                {
                    if (std::fread(&temp[1], 9, 1, in) != 1)    // read the Image Descriptor
                    {
                        inputerrorflag = 4;
                    }
                    std::memcpy(&xloc, &temp[1], 2); // X-location
                    std::memcpy(&yloc, &temp[3], 2); // Y-location
                    xloc += (xstep * xres);     // adjust the locations
                    yloc += (ystep * yres);
                    std::memcpy(&temp[1], &xloc, 2);
                    std::memcpy(&temp[3], &yloc, 2);
                    if (std::fwrite(temp, 10, 1, out) != 1)     // write out the Image Descriptor
                    {
                        errorflag = 4;
                    }

                    ichar = (char)(temp[9] & 0x80);     // is there a local color table?
                    if (ichar != 0)            // yup
                    {
                        if (std::fread(temp, 3*itbl, 1, in) != 1)       // read the local color table
                        {
                            inputerrorflag = 5;
                        }
                        if (std::fwrite(temp, 3*itbl, 1, out) != 1)     // write out the LCT
                        {
                            errorflag = 5;
                        }
                    }

                    if (std::fread(temp, 1, 1, in) != 1)        // LZH table size
                    {
                        inputerrorflag = 6;
                    }
                    if (std::fwrite(temp, 1, 1, out) != 1)
                    {
                        errorflag = 6;
                    }
                    while (true)
                    {
                        if (errorflag != 0 || inputerrorflag != 0)      // oops - did something go wrong?
                        {
                            break;
                        }
                        if (std::fread(temp, 1, 1, in) != 1)    // block size
                        {
                            inputerrorflag = 7;
                        }
                        if (std::fwrite(temp, 1, 1, out) != 1)
                        {
                            errorflag = 7;
                        }
                        i = temp[0];
                        if (i == 0)
                        {
                            break;
                        }
                        if (std::fread(temp, i, 1, in) != 1)    // LZH data block
                        {
                            inputerrorflag = 8;
                        }
                        if (std::fwrite(temp, i, 1, out) != 1)
                        {
                            errorflag = 8;
                        }
                    }
                }

                if (temp[0] == 0x21)           // extension block
                {
                    // (read, but only copy this if it's the last time through)
                    if (std::fread(&temp[2], 1, 1, in) != 1)    // read the block type
                    {
                        inputerrorflag = 9;
                    }
                    if (!g_gif87a_flag && xstep == xmult-1 && ystep == ymult-1)
                    {
                        if (std::fwrite(temp, 2, 1, out) != 1)
                        {
                            errorflag = 9;
                        }
                    }
                    while (true)
                    {
                        if (errorflag != 0 || inputerrorflag != 0)      // oops - did something go wrong?
                        {
                            break;
                        }
                        if (std::fread(temp, 1, 1, in) != 1)    // block size
                        {
                            inputerrorflag = 10;
                        }
                        if (!g_gif87a_flag && xstep == xmult-1 && ystep == ymult-1)
                        {
                            if (std::fwrite(temp, 1, 1, out) != 1)
                            {
                                errorflag = 10;
                            }
                        }
                        i = temp[0];
                        if (i == 0)
                        {
                            break;
                        }
                        if (std::fread(temp, i, 1, in) != 1)    // data block
                        {
                            inputerrorflag = 11;
                        }
                        if (!g_gif87a_flag && xstep == xmult-1 && ystep == ymult-1)
                        {
                            if (std::fwrite(temp, i, 1, out) != 1)
                            {
                                errorflag = 11;
                            }
                        }
                    }
                }

                if (temp[0] == 0x3b)           // end-of-stream indicator
                {
                    break;                      // done with this file
                }

                if (errorflag != 0 || inputerrorflag != 0)      // oops - did something go wrong?
                {
                    break;
                }
            }
            std::fclose(in);                     // done with an input GIF

            if (errorflag != 0 || inputerrorflag != 0)      // oops - did something go wrong?
            {
                break;
            }
        }

        if (errorflag != 0 || inputerrorflag != 0)  // oops - did something go wrong?
        {
            break;
        }
    }

    temp[0] = 0x3b;                 // end-of-stream indicator
    if (std::fwrite(temp, 1, 1, out) != 1)
    {
        errorflag = 12;
    }
    std::fclose(out);                    // done with the output GIF

    if (inputerrorflag != 0)       // uh-oh - something failed
    {
        std::printf("\007 Process failed = early EOF on input file %s\n", gifin);
        /* following line was for debugging
            std::printf("inputerrorflag = %d\n", inputerrorflag);
        */
    }

    if (errorflag != 0)            // uh-oh - something failed
    {
        std::printf("\007 Process failed = out of disk space?\n");
        /* following line was for debugging
            std::printf("errorflag = %d\n", errorflag);
        */
    }

    // now delete each input image, one at a time
    if (errorflag == 0 && inputerrorflag == 0)
    {
        for (unsigned ystep = 0U; ystep < ymult; ystep++)
        {
            for (unsigned xstep = 0U; xstep < xmult; xstep++)
            {
                std::snprintf(gifin, NUM_OF(gifin), "frmig_%c%c.gif", PAR_KEY(xstep), PAR_KEY(ystep));
                std::remove(gifin);
            }
        }
    }

    // tell the world we're done
    if (errorflag == 0 && inputerrorflag == 0)
    {
        std::printf("File %s has been created (and its component files deleted)\n", gifout);
    }
}
