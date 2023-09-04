// NF Lib doesn't seem to provide functions for forcing Hflip/Vflip on specific
// background tiles, nor getting the current value, only flipping the existing value...
#ifndef __NFEXTRA_H__
#define __NFEXTRA_H__

#include <nf_lib.h>

void NF_ForceTileHflip(int screen, u32 layer, u32 tile_x, u32 tile_y, int set);
void NF_ForceTileVflip(int screen, u32 layer, u32 tile_x, u32 tile_y, int set);

#endif
