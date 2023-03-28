/*
    Copyright 2023 Hydr8gon

    This file is part of NXEngine64.

    NXEngine64 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published
    by the Free Software Foundation, either version 3 of the License,
    or (at your option) any later version.

    NXEngine64 is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with NXEngine64. If not, see <https://www.gnu.org/licenses/>.
*/

#include "../../src/graphics/Renderer.h"

#include <libdragon.h>

extern display_context_t disp;

namespace NXE
{
namespace Graphics
{

Renderer::Renderer()
{
}

Renderer::~Renderer()
{
}

Renderer *Renderer::getInstance()
{
    return Singleton<Renderer>::get();
}

bool Renderer::init(int resolution)
{
    // Initialize the libdragon display system
    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 3, GAMMA_NONE, ANTIALIAS_RESAMPLE);
    rdp_init();
    rdp_set_texture_flush(FLUSH_STRATEGY_NONE);
    rdp_set_default_clipping();
    rdp_enable_texture_copy();
    return sprites.init();
}

void Renderer::close()
{
}

bool Renderer::initVideo()
{
    return false;
}

void Renderer::setFullscreen(bool enable)
{
}

bool Renderer::setResolution(int factor, bool restoreOnFailure)
{
    return false;
}

const gres_t *Renderer::getResolutions(bool full_list)
{
    return nullptr;
}

int Renderer::getResolutionCount()
{
    return 0;
}

bool Renderer::flushAll()
{
    return false;
}

void Renderer::showLoadingScreen()
{
}

void Renderer::drawSurface(Surface *src, int dstx, int dsty, int srcx, int srcy, int wd, int ht)
{
    // Get the surface's sprite
    sprite_t *sprite = (sprite_t*)src->texture();
    sprite->hslices = sprite->width / wd;
    sprite->vslices = sprite->height / ht;

    // Draw the sprite with the RDP
    rdp_sync(SYNC_PIPE);
    rdp_load_texture_stride(0, 0, MIRROR_DISABLED, sprite, (srcy / ht) * sprite->hslices + (srcx / wd));
    rdp_draw_sprite(0, dstx, dsty, MIRROR_DISABLED);
}

void Renderer::drawSurfaceMirrored(Surface *src, int dstx, int dsty, int srcx, int srcy, int wd, int ht)
{
    drawSurface(src, dstx, dsty, srcx, srcy, wd, ht);
}

void Renderer::blitPatternAcross(Surface *sfc, int x_dst, int y_dst, int y_src, int height)
{
}

void Renderer::clearScreen(uint8_t r, uint8_t g, uint8_t b)
{
}

void Renderer::fillRect(int x1, int y1, int x2, int y2, uint8_t r, uint8_t g, uint8_t b)
{
}

void Renderer::setClip(int x, int y, int w, int h)
{
}

void Renderer::clearClip()
{
}

void Renderer::drawSpotLight(int x, int y, Object* o, int r, int g, int b, int upscale)
{
}

void Renderer::tintScreen()
{
}

} // Graphics
} // NXE
