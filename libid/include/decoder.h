// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "port.h"

extern int                 (*g_out_line)(BYTE *, int);

short decoder(short);
void set_byte_buff(BYTE *ptr);
