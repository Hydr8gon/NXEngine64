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

#include "../../src/graphics/Surface.h"

namespace NXE
{
namespace Graphics
{

Surface::Surface()
{
}

Surface::~Surface()
{
}

bool Surface::loadImage(const std::string &pbm_name, bool use_colorkey)
{
    return false;
}

Surface *Surface::fromFile(const std::string &pbm_name, bool use_colorkey)
{
    Surface *surface = new Surface();
    return surface;
}

int Surface::width()
{
    return 0;
}

int Surface::height()
{
    return 0;
}

} // Graphics
} // NXE
