// NF Lib doesn't seem to provide functions for forcing Hflip/Vflip on specific
// background tiles, nor getting the current value, only flipping the existing value...

// Code modified from NF Lib
#include "nf_extra.h"

void NF_ForceTileHflip(int screen, u32 layer, u32 tile_x, u32 tile_y, int set)
{
    u32 address = NF_GetTileMapAddress(screen, layer, tile_x, tile_y);

    // Extract the top 8 bits of the map entry
    u32 hibyte = *(NF_BUFFER_BGMAP[NF_TILEDBG_LAYERS[screen][layer].bgslot] + (address + 1));

    // Set the bit that controls the horizontal flip of the tile
    if (set)
        hibyte |= 0x04;
    else
        hibyte &= ~0x04;

    *(NF_BUFFER_BGMAP[NF_TILEDBG_LAYERS[screen][layer].bgslot] + (address + 1)) = hibyte;
}

void NF_ForceTileVflip(int screen, u32 layer, u32 tile_x, u32 tile_y, int set)
{
    u32 address = NF_GetTileMapAddress(screen, layer, tile_x, tile_y);

    // Extract the top 8 bits of the map entry
    u32 hibyte = *(NF_BUFFER_BGMAP[NF_TILEDBG_LAYERS[screen][layer].bgslot] + (address + 1));

    // Set the bit that controls the vertical flip of the tile
    if (set)
        hibyte |= 0x08;
    else
        hibyte &= ~0x08;

    *(NF_BUFFER_BGMAP[NF_TILEDBG_LAYERS[screen][layer].bgslot] + (address + 1)) = hibyte;
}
