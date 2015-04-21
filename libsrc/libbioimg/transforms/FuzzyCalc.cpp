/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*                                                                               */
/*    Copyright (C) 2007 Open Microscopy Environment                             */
/*         Massachusetts Institue of Technology,                                 */
/*         National Institutes of Health,                                        */
/*         University of Dundee                                                  */
/*                                                                               */
/*                                                                               */
/*                                                                               */
/*    This library is free software; you can redistribute it and/or              */
/*    modify it under the terms of the GNU Lesser General Public                 */
/*    License as published by the Free Software Foundation; either               */
/*    version 2.1 of the License, or (at your option) any later version.         */
/*                                                                               */
/*    This library is distributed in the hope that it will be useful,            */
/*    but WITHOUT ANY WARRANTY; without even the implied warranty of             */
/*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU          */
/*    Lesser General Public License for more details.                            */
/*                                                                               */
/*    You should have received a copy of the GNU Lesser General Public           */
/*    License along with this library; if not, write to the Free Software        */
/*    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA  */
/*                                                                               */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*                                                                               */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* Written by:  Lior Shamir <shamirl [at] mail [dot] nih [dot] gov>              */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/


#include <cstdio>
#include <cmath>
#include <cstring>
#include <cstdlib>

#include "FuzzyCalc.h"

trapez_function color_functions[] = {
    { "red",            5,  0,   0,   0,   11 },
    { "dark_orange",    9,  0,   17,  17,  27 },
    { "light_orange",   10, 20,  27,  27,  37 },
    { "yellow",         11, 27,  39,  39,  47 },
    { "light_green",    13, 40,  50,  50,  80 },
    { "dark_green",     14, 50,  80,  80,  120 },
    { "aqua",           16, 80,  120, 120, 160 },
    { "blue",           17, 120, 160, 160, 200 },
    { "dark_fucia",     18, 160, 200, 200, 220 },
    { "light_fucia",    19, 200, 220, 220, 230 },
    { "red",            5,  220, 240, 240, 240 },
    { "",               -1, 0,   0,   0,   0 },
};

color_type colors[] = {
    { "",             (TColor)0x00000000 }, // 0
    { "white",        (TColor)0x00FFFFFF }, // 1
    { "light_grey",   (TColor)0x00BDBEBD }, // 2
    { "dark_grey",    (TColor)0x007B7D7B }, // 3
    { "black",        (TColor)0x00000000 }, // 4
    { "red",          (TColor)0x0000FF }, // 5
    { "pink",         (TColor)0x008080FF }, // 6
    { "dark_brown",   (TColor)0x0018497B }, // 7
    { "light_brown",  (TColor)0x005A8ABD }, // 8
    { "dark_orange",  (TColor)0x006DFF }, // 9
    { "light_orange", (TColor)0x0000AEFF }, // 10
    { "yellow",       (TColor)0x0000FFFF }, // 11
    { "olive",        (TColor)0x00008080 }, // 12
    { "light_green",  (TColor)0x0000FF00 }, // 13
    { "dark_green",   (TColor)0x00008000 }, // 14
    { "teal",         (TColor)0x00808000 }, // 15
    { "aqua",         (TColor)0x00FFFF00 }, // 16
    { "blue",         (TColor)0x00FF0000 }, // 17
    { "dark_fucia",   (TColor)0x00800080 }, // 18
    { "light_fucia",  (TColor)0x00FF00FF }, // 19
    { "",             (TColor)-1 } // 20
};

fuzzy_rule fuzzy_rules[] = {
    {5, SATURATION_GREY, VALUE_DARK, 4},
    {5, SATURATION_GREY, VALUE_ALMOST_DARK, 4},
    {5, SATURATION_GREY, VALUE_TEND_DARK, 3},
    {5, SATURATION_GREY, VALUE_TEND_LIGHT, 2},
    {5, SATURATION_GREY, VALUE_LIGHT, 1},
    {5, SATURATION_ALMOST_GREY, VALUE_DARK, 4},
    {5, SATURATION_ALMOST_GREY, VALUE_ALMOST_DARK, 3},
    {5, SATURATION_ALMOST_GREY, VALUE_TEND_DARK, 3},
    {5, SATURATION_ALMOST_GREY, VALUE_TEND_LIGHT, 19},
    {5, SATURATION_ALMOST_GREY, VALUE_LIGHT, 6},
    {5, SATURATION_TEND_GREY, VALUE_DARK, 4},
    {5, SATURATION_TEND_GREY, VALUE_ALMOST_DARK, 7},
    {5, SATURATION_TEND_GREY, VALUE_TEND_DARK, 18},
    {5, SATURATION_TEND_GREY, VALUE_TEND_LIGHT, 6},
    {5, SATURATION_TEND_GREY, VALUE_LIGHT, 6},
    {5, SATURATION_MEDIUM_GREY, VALUE_DARK, 4},
    {5, SATURATION_MEDIUM_GREY, VALUE_ALMOST_DARK, 7},
    {5, SATURATION_MEDIUM_GREY, VALUE_TEND_DARK, 8},
    {5, SATURATION_MEDIUM_GREY, VALUE_TEND_LIGHT, 19},
    {5, SATURATION_MEDIUM_GREY, VALUE_LIGHT, 6},
    {5, SATURATION_TEND_CLEAR, VALUE_DARK, 4},
    {5, SATURATION_TEND_CLEAR, VALUE_ALMOST_DARK, 7},
    {5, SATURATION_TEND_CLEAR, VALUE_TEND_DARK, 7},
    {5, SATURATION_TEND_CLEAR, VALUE_TEND_LIGHT, 5},
    {5, SATURATION_TEND_CLEAR, VALUE_LIGHT, 5},
    {5, SATURATION_CLEAR, VALUE_DARK, 4},
    {5, SATURATION_CLEAR, VALUE_ALMOST_DARK, 4},
    {5, SATURATION_CLEAR, VALUE_TEND_DARK, 5},
    {5, SATURATION_CLEAR, VALUE_TEND_LIGHT, 5},
    {5, SATURATION_CLEAR, VALUE_LIGHT, 5},
    {9, SATURATION_GREY, VALUE_DARK, 4},
    {9, SATURATION_GREY, VALUE_ALMOST_DARK, 3},
    {9, SATURATION_GREY, VALUE_TEND_DARK, 3},
    {9, SATURATION_GREY, VALUE_TEND_LIGHT, 2},
    {9, SATURATION_GREY, VALUE_LIGHT, 1},
    {9, SATURATION_ALMOST_GREY, VALUE_DARK, 4},
    {9, SATURATION_ALMOST_GREY, VALUE_ALMOST_DARK, 3},
    {9, SATURATION_ALMOST_GREY, VALUE_TEND_DARK, 3},
    {9, SATURATION_ALMOST_GREY, VALUE_TEND_LIGHT, 2},
    {9, SATURATION_ALMOST_GREY, VALUE_LIGHT, 1},
    {9, SATURATION_TEND_GREY, VALUE_DARK, 4},
    {9, SATURATION_TEND_GREY, VALUE_ALMOST_DARK, 7},
    {9, SATURATION_TEND_GREY, VALUE_TEND_DARK, 3},
    {9, SATURATION_TEND_GREY, VALUE_TEND_LIGHT, 8},
    {9, SATURATION_TEND_GREY, VALUE_LIGHT, 6},
    {9, SATURATION_MEDIUM_GREY, VALUE_DARK, 4},
    {9, SATURATION_MEDIUM_GREY, VALUE_ALMOST_DARK, 7},
    {9, SATURATION_MEDIUM_GREY, VALUE_TEND_DARK, 7},
    {9, SATURATION_MEDIUM_GREY, VALUE_TEND_LIGHT, 8},
    {9, SATURATION_MEDIUM_GREY, VALUE_LIGHT, 10},
    {9, SATURATION_TEND_CLEAR, VALUE_DARK, 4},
    {9, SATURATION_TEND_CLEAR, VALUE_ALMOST_DARK, 7},
    {9, SATURATION_TEND_CLEAR, VALUE_TEND_DARK, 7},
    {9, SATURATION_TEND_CLEAR, VALUE_TEND_LIGHT, 8},
    {9, SATURATION_TEND_CLEAR, VALUE_LIGHT, 9},
    {9, SATURATION_CLEAR, VALUE_DARK, 4},
    {9, SATURATION_CLEAR, VALUE_ALMOST_DARK, 7},
    {9, SATURATION_CLEAR, VALUE_TEND_DARK, 7},
    {9, SATURATION_CLEAR, VALUE_TEND_LIGHT, 8},
    {9, SATURATION_CLEAR, VALUE_LIGHT, 9},
    {10, SATURATION_GREY, VALUE_DARK, 4},
    {10, SATURATION_GREY, VALUE_ALMOST_DARK, 3},
    {10, SATURATION_GREY, VALUE_TEND_DARK, 3},
    {10, SATURATION_GREY, VALUE_TEND_LIGHT, 2},
    {10, SATURATION_GREY, VALUE_LIGHT, 1},
    {10, SATURATION_ALMOST_GREY, VALUE_DARK, 4},
    {10, SATURATION_ALMOST_GREY, VALUE_ALMOST_DARK, 3},
    {10, SATURATION_ALMOST_GREY, VALUE_TEND_DARK, 3},
    {10, SATURATION_ALMOST_GREY, VALUE_TEND_LIGHT, 2},
    {10, SATURATION_ALMOST_GREY, VALUE_LIGHT, 1},
    {10, SATURATION_TEND_GREY, VALUE_DARK, 4},
    {10, SATURATION_TEND_GREY, VALUE_ALMOST_DARK, 3},
    {10, SATURATION_TEND_GREY, VALUE_TEND_DARK, 3},
    {10, SATURATION_TEND_GREY, VALUE_TEND_LIGHT, 2},
    {10, SATURATION_TEND_GREY, VALUE_LIGHT, 10},
    {10, SATURATION_MEDIUM_GREY, VALUE_DARK, 4},
    {10, SATURATION_MEDIUM_GREY, VALUE_ALMOST_DARK, 7},
    {10, SATURATION_MEDIUM_GREY, VALUE_TEND_DARK, 8},
    {10, SATURATION_MEDIUM_GREY, VALUE_TEND_LIGHT, 8},
    {10, SATURATION_MEDIUM_GREY, VALUE_LIGHT, 10},
    {10, SATURATION_TEND_CLEAR, VALUE_DARK, 4},
    {10, SATURATION_TEND_CLEAR, VALUE_ALMOST_DARK, 7},
    {10, SATURATION_TEND_CLEAR, VALUE_TEND_DARK, 8},
    {10, SATURATION_TEND_CLEAR, VALUE_TEND_LIGHT, 8},
    {10, SATURATION_TEND_CLEAR, VALUE_LIGHT, 10},
    {10, SATURATION_CLEAR, VALUE_DARK, 4},
    {10, SATURATION_CLEAR, VALUE_ALMOST_DARK, 7},
    {10, SATURATION_CLEAR, VALUE_TEND_DARK, 8},
    {10, SATURATION_CLEAR, VALUE_TEND_LIGHT, 8},
    {10, SATURATION_CLEAR, VALUE_LIGHT, 10},
    {11, SATURATION_GREY, VALUE_DARK, 4},
    {11, SATURATION_GREY, VALUE_ALMOST_DARK, 3},
    {11, SATURATION_GREY, VALUE_TEND_DARK, 3},
    {11, SATURATION_GREY, VALUE_TEND_LIGHT, 2},
    {11, SATURATION_GREY, VALUE_LIGHT, 1},
    {11, SATURATION_ALMOST_GREY, VALUE_DARK, 4},
    {11, SATURATION_ALMOST_GREY, VALUE_ALMOST_DARK, 3},
    {11, SATURATION_ALMOST_GREY, VALUE_TEND_DARK, 3},
    {11, SATURATION_ALMOST_GREY, VALUE_TEND_LIGHT, 2},
    {11, SATURATION_ALMOST_GREY, VALUE_LIGHT, 1},
    {11, SATURATION_TEND_GREY, VALUE_DARK, 4},
    {11, SATURATION_TEND_GREY, VALUE_ALMOST_DARK, 3},
    {11, SATURATION_TEND_GREY, VALUE_TEND_DARK, 12},
    {11, SATURATION_TEND_GREY, VALUE_TEND_LIGHT, 12},
    {11, SATURATION_TEND_GREY, VALUE_LIGHT, 11},
    {11, SATURATION_MEDIUM_GREY, VALUE_DARK, 4},
    {11, SATURATION_MEDIUM_GREY, VALUE_ALMOST_DARK, 12},
    {11, SATURATION_MEDIUM_GREY, VALUE_TEND_DARK, 12},
    {11, SATURATION_MEDIUM_GREY, VALUE_TEND_LIGHT, 12},
    {11, SATURATION_MEDIUM_GREY, VALUE_LIGHT, 11},
    {11, SATURATION_TEND_CLEAR, VALUE_DARK, 4},
    {11, SATURATION_TEND_CLEAR, VALUE_ALMOST_DARK, 12},
    {11, SATURATION_TEND_CLEAR, VALUE_TEND_DARK, 12},
    {11, SATURATION_TEND_CLEAR, VALUE_TEND_LIGHT, 12},
    {11, SATURATION_TEND_CLEAR, VALUE_LIGHT, 11},
    {11, SATURATION_CLEAR, VALUE_DARK, 4},
    {11, SATURATION_CLEAR, VALUE_ALMOST_DARK, 12},
    {11, SATURATION_CLEAR, VALUE_TEND_DARK, 12},
    {11, SATURATION_CLEAR, VALUE_TEND_LIGHT, 12},
    {11, SATURATION_CLEAR, VALUE_LIGHT, 11},
    {13, SATURATION_GREY, VALUE_DARK, 4},
    {13, SATURATION_GREY, VALUE_ALMOST_DARK, 3},
    {13, SATURATION_GREY, VALUE_TEND_DARK, 3},
    {13, SATURATION_GREY, VALUE_TEND_LIGHT, 2},
    {13, SATURATION_GREY, VALUE_LIGHT, 1},
    {13, SATURATION_ALMOST_GREY, VALUE_DARK, 4},
    {13, SATURATION_ALMOST_GREY, VALUE_ALMOST_DARK, 3},
    {13, SATURATION_ALMOST_GREY, VALUE_TEND_DARK, 3},
    {13, SATURATION_ALMOST_GREY, VALUE_TEND_LIGHT, 2},
    {13, SATURATION_ALMOST_GREY, VALUE_LIGHT, 1},
    {13, SATURATION_TEND_GREY, VALUE_DARK, 4},
    {13, SATURATION_TEND_GREY, VALUE_ALMOST_DARK, 12},
    {13, SATURATION_TEND_GREY, VALUE_TEND_DARK, 12},
    {13, SATURATION_TEND_GREY, VALUE_TEND_LIGHT, 13},
    {13, SATURATION_TEND_GREY, VALUE_LIGHT, 13},
    {13, SATURATION_MEDIUM_GREY, VALUE_DARK, 4},
    {13, SATURATION_MEDIUM_GREY, VALUE_ALMOST_DARK, 14},
    {13, SATURATION_MEDIUM_GREY, VALUE_TEND_DARK, 14},
    {13, SATURATION_MEDIUM_GREY, VALUE_TEND_LIGHT, 13},
    {13, SATURATION_MEDIUM_GREY, VALUE_LIGHT, 13},
    {13, SATURATION_TEND_CLEAR, VALUE_DARK, 4},
    {13, SATURATION_TEND_CLEAR, VALUE_ALMOST_DARK, 14},
    {13, SATURATION_TEND_CLEAR, VALUE_TEND_DARK, 14},
    {13, SATURATION_TEND_CLEAR, VALUE_TEND_LIGHT, 13},
    {13, SATURATION_TEND_CLEAR, VALUE_LIGHT, 13},
    {13, SATURATION_CLEAR, VALUE_DARK, 4},
    {13, SATURATION_CLEAR, VALUE_ALMOST_DARK, 14},
    {13, SATURATION_CLEAR, VALUE_TEND_DARK, 14},
    {13, SATURATION_CLEAR, VALUE_TEND_LIGHT, 13},
    {13, SATURATION_CLEAR, VALUE_LIGHT, 13},
    {14, SATURATION_GREY, VALUE_DARK, 4},
    {14, SATURATION_GREY, VALUE_ALMOST_DARK, 3},
    {14, SATURATION_GREY, VALUE_TEND_DARK, 3},
    {14, SATURATION_GREY, VALUE_TEND_LIGHT, 2},
    {14, SATURATION_GREY, VALUE_LIGHT, 1},
    {14, SATURATION_ALMOST_GREY, VALUE_DARK, 4},
    {14, SATURATION_ALMOST_GREY, VALUE_ALMOST_DARK, 3},
    {14, SATURATION_ALMOST_GREY, VALUE_TEND_DARK, 3},
    {14, SATURATION_ALMOST_GREY, VALUE_TEND_LIGHT, 13},
    {14, SATURATION_ALMOST_GREY, VALUE_LIGHT, 13},
    {14, SATURATION_TEND_GREY, VALUE_DARK, 4},
    {14, SATURATION_TEND_GREY, VALUE_ALMOST_DARK, 14},
    {14, SATURATION_TEND_GREY, VALUE_TEND_DARK, 14},
    {14, SATURATION_TEND_GREY, VALUE_TEND_LIGHT, 13},
    {14, SATURATION_TEND_GREY, VALUE_LIGHT, 13},
    {14, SATURATION_MEDIUM_GREY, VALUE_DARK, 4},
    {14, SATURATION_MEDIUM_GREY, VALUE_ALMOST_DARK, 14},
    {14, SATURATION_MEDIUM_GREY, VALUE_TEND_DARK, 14},
    {14, SATURATION_MEDIUM_GREY, VALUE_TEND_LIGHT, 13},
    {14, SATURATION_MEDIUM_GREY, VALUE_LIGHT, 13},
    {14, SATURATION_TEND_CLEAR, VALUE_DARK, 4},
    {14, SATURATION_TEND_CLEAR, VALUE_ALMOST_DARK, 14},
    {14, SATURATION_TEND_CLEAR, VALUE_TEND_DARK, 14},
    {14, SATURATION_TEND_CLEAR, VALUE_TEND_LIGHT, 13},
    {14, SATURATION_TEND_CLEAR, VALUE_LIGHT, 13},
    {14, SATURATION_CLEAR, VALUE_DARK, 4},
    {14, SATURATION_CLEAR, VALUE_ALMOST_DARK, 14},
    {14, SATURATION_CLEAR, VALUE_TEND_DARK, 14},
    {14, SATURATION_CLEAR, VALUE_TEND_LIGHT, 13},
    {14, SATURATION_CLEAR, VALUE_LIGHT, 13},
    {16, SATURATION_GREY, VALUE_DARK, 4},
    {16, SATURATION_GREY, VALUE_ALMOST_DARK, 3},
    {16, SATURATION_GREY, VALUE_TEND_DARK, 3},
    {16, SATURATION_GREY, VALUE_TEND_LIGHT, 2},
    {16, SATURATION_GREY, VALUE_LIGHT, 1},
    {16, SATURATION_ALMOST_GREY, VALUE_DARK, 4},
    {16, SATURATION_ALMOST_GREY, VALUE_ALMOST_DARK, 3},
    {16, SATURATION_ALMOST_GREY, VALUE_TEND_DARK, 3},
    {16, SATURATION_ALMOST_GREY, VALUE_TEND_LIGHT, 15},
    {16, SATURATION_ALMOST_GREY, VALUE_LIGHT, 16},
    {16, SATURATION_TEND_GREY, VALUE_DARK, 4},
    {16, SATURATION_TEND_GREY, VALUE_ALMOST_DARK, 15},
    {16, SATURATION_TEND_GREY, VALUE_TEND_DARK, 17},
    {16, SATURATION_TEND_GREY, VALUE_TEND_LIGHT, 17},
    {16, SATURATION_TEND_GREY, VALUE_LIGHT, 16},
    {16, SATURATION_MEDIUM_GREY, VALUE_DARK, 4},
    {16, SATURATION_MEDIUM_GREY, VALUE_ALMOST_DARK, 15},
    {16, SATURATION_MEDIUM_GREY, VALUE_TEND_DARK, 17},
    {16, SATURATION_MEDIUM_GREY, VALUE_TEND_LIGHT, 16},
    {16, SATURATION_MEDIUM_GREY, VALUE_LIGHT, 16},
    {16, SATURATION_TEND_CLEAR, VALUE_DARK, 4},
    {16, SATURATION_TEND_CLEAR, VALUE_ALMOST_DARK, 15},
    {16, SATURATION_TEND_CLEAR, VALUE_TEND_DARK, 17},
    {16, SATURATION_TEND_CLEAR, VALUE_TEND_LIGHT, 16},
    {16, SATURATION_TEND_CLEAR, VALUE_LIGHT, 16},
    {16, SATURATION_CLEAR, VALUE_DARK, 4},
    {16, SATURATION_CLEAR, VALUE_ALMOST_DARK, 15},
    {16, SATURATION_CLEAR, VALUE_TEND_DARK, 15},
    {16, SATURATION_CLEAR, VALUE_TEND_LIGHT, 16},
    {16, SATURATION_CLEAR, VALUE_LIGHT, 16},
    {17, SATURATION_GREY, VALUE_DARK, 4},
    {17, SATURATION_GREY, VALUE_ALMOST_DARK, 3},
    {17, SATURATION_GREY, VALUE_TEND_DARK, 3},
    {17, SATURATION_GREY, VALUE_TEND_LIGHT, 2},
    {17, SATURATION_GREY, VALUE_LIGHT, 1},
    {17, SATURATION_ALMOST_GREY, VALUE_DARK, 4},
    {17, SATURATION_ALMOST_GREY, VALUE_ALMOST_DARK, 3},
    {17, SATURATION_ALMOST_GREY, VALUE_TEND_DARK, 2},
    {17, SATURATION_ALMOST_GREY, VALUE_TEND_LIGHT, 16},
    {17, SATURATION_ALMOST_GREY, VALUE_LIGHT, 16},
    {17, SATURATION_TEND_GREY, VALUE_DARK, 4},
    {17, SATURATION_TEND_GREY, VALUE_ALMOST_DARK, 18},
    {17, SATURATION_TEND_GREY, VALUE_TEND_DARK, 19},
    {17, SATURATION_TEND_GREY, VALUE_TEND_LIGHT, 19},
    {17, SATURATION_TEND_GREY, VALUE_LIGHT, 16},
    {17, SATURATION_MEDIUM_GREY, VALUE_DARK, 4},
    {17, SATURATION_MEDIUM_GREY, VALUE_ALMOST_DARK, 17},
    {17, SATURATION_MEDIUM_GREY, VALUE_TEND_DARK, 17},
    {17, SATURATION_MEDIUM_GREY, VALUE_TEND_LIGHT, 17},
    {17, SATURATION_MEDIUM_GREY, VALUE_LIGHT, 17},
    {17, SATURATION_TEND_CLEAR, VALUE_DARK, 4},
    {17, SATURATION_TEND_CLEAR, VALUE_ALMOST_DARK, 17},
    {17, SATURATION_TEND_CLEAR, VALUE_TEND_DARK, 17},
    {17, SATURATION_TEND_CLEAR, VALUE_TEND_LIGHT, 17},
    {17, SATURATION_TEND_CLEAR, VALUE_LIGHT, 17},
    {17, SATURATION_CLEAR, VALUE_DARK, 4},
    {17, SATURATION_CLEAR, VALUE_ALMOST_DARK, 17},
    {17, SATURATION_CLEAR, VALUE_TEND_DARK, 17},
    {17, SATURATION_CLEAR, VALUE_TEND_LIGHT, 17},
    {17, SATURATION_CLEAR, VALUE_LIGHT, 17},
    {18, SATURATION_GREY, VALUE_DARK, 4},
    {18, SATURATION_GREY, VALUE_ALMOST_DARK, 3},
    {18, SATURATION_GREY, VALUE_TEND_DARK, 3},
    {18, SATURATION_GREY, VALUE_TEND_LIGHT, 2},
    {18, SATURATION_GREY, VALUE_LIGHT, 1},
    {18, SATURATION_ALMOST_GREY, VALUE_DARK, 4},
    {18, SATURATION_ALMOST_GREY, VALUE_ALMOST_DARK, 3},
    {18, SATURATION_ALMOST_GREY, VALUE_TEND_DARK, 18},
    {18, SATURATION_ALMOST_GREY, VALUE_TEND_LIGHT, 19},
    {18, SATURATION_ALMOST_GREY, VALUE_LIGHT, 19},
    {18, SATURATION_TEND_GREY, VALUE_DARK, 4},
    {18, SATURATION_TEND_GREY, VALUE_ALMOST_DARK, 18},
    {18, SATURATION_TEND_GREY, VALUE_TEND_DARK, 18},
    {18, SATURATION_TEND_GREY, VALUE_TEND_LIGHT, 19},
    {18, SATURATION_TEND_GREY, VALUE_LIGHT, 19},
    {18, SATURATION_MEDIUM_GREY, VALUE_DARK, 4},
    {18, SATURATION_MEDIUM_GREY, VALUE_ALMOST_DARK, 18},
    {18, SATURATION_MEDIUM_GREY, VALUE_TEND_DARK, 18},
    {18, SATURATION_MEDIUM_GREY, VALUE_TEND_LIGHT, 19},
    {18, SATURATION_MEDIUM_GREY, VALUE_LIGHT, 19},
    {18, SATURATION_TEND_CLEAR, VALUE_DARK, 4},
    {18, SATURATION_TEND_CLEAR, VALUE_ALMOST_DARK, 18},
    {18, SATURATION_TEND_CLEAR, VALUE_TEND_DARK, 18},
    {18, SATURATION_TEND_CLEAR, VALUE_TEND_LIGHT, 19},
    {18, SATURATION_TEND_CLEAR, VALUE_LIGHT, 19},
    {18, SATURATION_CLEAR, VALUE_DARK, 4},
    {18, SATURATION_CLEAR, VALUE_ALMOST_DARK, 7},
    {18, SATURATION_CLEAR, VALUE_TEND_DARK, 18},
    {18, SATURATION_CLEAR, VALUE_TEND_LIGHT, 18},
    {18, SATURATION_CLEAR, VALUE_LIGHT, 19},
    {19, SATURATION_GREY, VALUE_DARK, 4},
    {19, SATURATION_GREY, VALUE_ALMOST_DARK, 3},
    {19, SATURATION_GREY, VALUE_TEND_DARK, 3},
    {19, SATURATION_GREY, VALUE_TEND_LIGHT, 2},
    {19, SATURATION_GREY, VALUE_LIGHT, 1},
    {19, SATURATION_ALMOST_GREY, VALUE_DARK, 4},
    {19, SATURATION_ALMOST_GREY, VALUE_ALMOST_DARK, 3},
    {19, SATURATION_ALMOST_GREY, VALUE_TEND_DARK, 2},
    {19, SATURATION_ALMOST_GREY, VALUE_TEND_LIGHT, 19},
    {19, SATURATION_ALMOST_GREY, VALUE_LIGHT, 19},
    {19, SATURATION_TEND_GREY, VALUE_DARK, 4},
    {19, SATURATION_TEND_GREY, VALUE_ALMOST_DARK, 18},
    {19, SATURATION_TEND_GREY, VALUE_TEND_DARK, 18},
    {19, SATURATION_TEND_GREY, VALUE_TEND_LIGHT, 19},
    {19, SATURATION_TEND_GREY, VALUE_LIGHT, 19},
    {19, SATURATION_MEDIUM_GREY, VALUE_DARK, 4},
    {19, SATURATION_MEDIUM_GREY, VALUE_ALMOST_DARK, 18},
    {19, SATURATION_MEDIUM_GREY, VALUE_TEND_DARK, 18},
    {19, SATURATION_MEDIUM_GREY, VALUE_TEND_LIGHT, 19},
    {19, SATURATION_MEDIUM_GREY, VALUE_LIGHT, 19},
    {19, SATURATION_TEND_CLEAR, VALUE_DARK, 4},
    {19, SATURATION_TEND_CLEAR, VALUE_ALMOST_DARK, 7},
    {19, SATURATION_TEND_CLEAR, VALUE_TEND_DARK, 18},
    {19, SATURATION_TEND_CLEAR, VALUE_TEND_LIGHT, 19},
    {19, SATURATION_TEND_CLEAR, VALUE_LIGHT, 19},
    {19, SATURATION_CLEAR, VALUE_DARK, 4},
    {19, SATURATION_CLEAR, VALUE_ALMOST_DARK, 7},
    {19, SATURATION_CLEAR, VALUE_TEND_DARK, 18},
    {19, SATURATION_CLEAR, VALUE_TEND_LIGHT, 18},
    {19, SATURATION_CLEAR, VALUE_LIGHT, 19},
    {-1, -1, -1, -1}
};



double color_value(int color_index, double color) {
    int FunctionCounter=0;
    double res=0;
    while (color_functions[FunctionCounter].color>0) { //Chris - changed it from >= to > or else infinite loop
        if (color_index==color_functions[FunctionCounter].color) {
            if (!res
                && (color<color_functions[FunctionCounter].start
                || color>color_functions[FunctionCounter].end)
                )
                res = 0;           /* out of the range */
            else if (!res
                && color>=color_functions[FunctionCounter].maximum1
                && color<=color_functions[FunctionCounter].maximum2
                )
                res = 1; /* the top of the trapez maximum */
            else if (!res && color<color_functions[FunctionCounter].maximum1)
                res = (color-color_functions[FunctionCounter].start)/(color_functions[FunctionCounter].maximum1-color_functions[FunctionCounter].start);
            else if (!res && color>color_functions[FunctionCounter].maximum2)
                res = 1-(color-color_functions[FunctionCounter].maximum2)/(color_functions[FunctionCounter].end-color_functions[FunctionCounter].maximum2);
            if (res>0) return(res);
        }
        FunctionCounter++;
    }
    return(res);
}

/***************************************/
/* membership functions for saturation */
/***************************************/
/* membership function for "grey" */
double grey_value(double value) {
    if (value<MIN_GREY) return(0);   /* minimum */
    if (value<GREY_START || value>GREY_END) return(0);   /* out of the range      */
    if (value<GREY_MAX) return((value-GREY_START)/(GREY_MAX-GREY_START));
    else return(1-((value-GREY_MAX)/(GREY_END-GREY_MAX)));
}
/* membership function for "almost_grey" */
double almost_grey_value(double value) {
    if (value<ALMOST_GREY_START || value>ALMOST_GREY_END) return(0);   /* out of the range      */
    if (value<ALMOST_GREY_MAX) return((value-ALMOST_GREY_START)/(ALMOST_GREY_MAX-ALMOST_GREY_START));
    else return(1-((value-ALMOST_GREY_MAX)/(ALMOST_GREY_END-ALMOST_GREY_MAX)));
}
/* membership function for "tend_grey" */
double tend_grey_value(double value) {
    if (value<TEND_GREY_START || value>TEND_GREY_END) return(0);   /* out of the range      */
    if (value<TEND_GREY_MAX) return((value-TEND_GREY_START)/(TEND_GREY_MAX-TEND_GREY_START));
    else return(1-((value-TEND_GREY_MAX)/(TEND_GREY_END-TEND_GREY_MAX)));
}
/* membership function for "medium_grey" */
double medium_grey_value(double value) {
    if (value<MEDIUM_GREY_START || value>MEDIUM_GREY_END) return(0);   /* out of the range      */
    if (value<MEDIUM_GREY_MAX) return((value-MEDIUM_GREY_START)/(MEDIUM_GREY_MAX-MEDIUM_GREY_START));
    else return(1-((value-MEDIUM_GREY_MAX)/(MEDIUM_GREY_END-MEDIUM_GREY_MAX)));
}
/* membership function for "tend_clear" */
double tend_clear_value(double value) {
    if (value<TEND_CLEAR_START || value>TEND_CLEAR_END) return(0);   /* out of the range      */
    if (value<TEND_CLEAR_MAX) return((value-TEND_CLEAR_START)/(TEND_CLEAR_MAX-TEND_CLEAR_START));
    else return(1-((value-TEND_CLEAR_MAX)/(TEND_CLEAR_END-TEND_CLEAR_MAX)));
}
/* membership function for "clear" */
double clear_value(double value) {
    if (value<CLEAR_START || value>CLEAR_END) return(0);   /* out of the range      */
    if (value<=CLEAR_MAX) return((value-CLEAR_START)/(CLEAR_MAX-CLEAR_START));
    else return(1-((value-CLEAR_MAX)/(CLEAR_END-CLEAR_MAX)));
}

/**********************************/
/* memnership functions for value */
/**********************************/
/* membership function for "dark" */
double dark_value(double value) {
    if (value<MIN_DARK) return(0);  /* below the minimum */
    if (value<DARK_START || value>DARK_END) return(0);   /* out of the range      */
    if (value<DARK_MAX) return((value-DARK_START)/(DARK_MAX-DARK_START));
    else return(1-((value-DARK_MAX)/(DARK_END-DARK_MAX)));
}
/* membership function for "almost_dark" */
double almost_dark_value(double value) {
    if (value<ALMOST_DARK_START || value>ALMOST_DARK_END) return(0);   /* out of the range      */
    if (value<ALMOST_DARK_MAX) return((value-ALMOST_DARK_START)/(ALMOST_DARK_MAX-ALMOST_DARK_START));
    else return(1-((value-ALMOST_DARK_MAX)/(ALMOST_DARK_END-ALMOST_DARK_MAX)));
}
/* membership function for "tend_dark" */
double tend_dark_value(double value) {
    if (value<TEND_DARK_START || value>TEND_DARK_END) return(0);   /* out of the range      */
    if (value<TEND_DARK_MAX) return((value-TEND_DARK_START)/(TEND_DARK_MAX-TEND_DARK_START));
    else return(1-((value-TEND_DARK_MAX)/(TEND_DARK_END-TEND_DARK_MAX)));
}
/* membership function for "tend_light" */
double tend_light_value(double value) {
    if (value<TEND_LIGHT_START || value>TEND_LIGHT_END) return(0);   /* out of the range      */
    if (value<TEND_LIGHT_MAX) return((value-TEND_LIGHT_START)/(TEND_LIGHT_MAX-TEND_LIGHT_START));
    else return(1-((value-TEND_LIGHT_MAX)/(TEND_LIGHT_END-TEND_LIGHT_MAX)));
}
/* membership function for "light" */
double light_value(double value) {
    if (value<LIGHT_START || value>LIGHT_END) return(0);   /* out of the range      */
    if (value<=LIGHT_MAX) return((value-LIGHT_START)/(LIGHT_MAX-LIGHT_START));
    else return(1-((value-LIGHT_MAX)/(LIGHT_END-LIGHT_MAX)));
}

/*********************************/
/* colors membership functions   */
/*********************************/
/* membership function for "red" */
double hue_red_value(double value) {
    if (value<HUE_RED_START || value>HUE_RED_END) return(0); /* out of the range */
    if (value<HUE_RED_MAX) return((value-HUE_RED_START)/(HUE_RED_MAX-HUE_RED_START));
    else return(1-((value-HUE_RED_MAX)/(HUE_RED_END-HUE_RED_MAX)));
}
/* membership function for "dark orange" */
double hue_dark_orange_value(double value) {
    if (value<HUE_DARK_ORANGE_START || value>HUE_DARK_ORANGE_END) return(0);   /* out of the range      */
    if (value>=HUE_DARK_ORANGE_MAX1 && value<=HUE_DARK_ORANGE_MAX2) return(1); /* the top of the trapez */
    if (value<HUE_DARK_ORANGE_MAX1) return((value-HUE_DARK_ORANGE_START)/(HUE_DARK_ORANGE_MAX1-HUE_DARK_ORANGE_START));
    else return(1-((value-HUE_DARK_ORANGE_MAX2)/(HUE_DARK_ORANGE_END-HUE_DARK_ORANGE_MAX2)));
}
/* membership function for "light orange" */
double hue_light_orange_value(double value) {
    if (value<HUE_LIGHT_ORANGE_START || value>HUE_LIGHT_ORANGE_END) return(0);   /* out of the range      */
    if (value>=HUE_LIGHT_ORANGE_MAX1 && value<=HUE_LIGHT_ORANGE_MAX2) return(1); /* the top of the trapez */
    if (value<HUE_LIGHT_ORANGE_MAX1) return((value-HUE_LIGHT_ORANGE_START)/(HUE_LIGHT_ORANGE_MAX1-HUE_LIGHT_ORANGE_START));
    else return(1-((value-HUE_LIGHT_ORANGE_MAX2)/(HUE_LIGHT_ORANGE_END-HUE_LIGHT_ORANGE_MAX2)));
}
/* membership function for "yellow" */
double hue_yellow_value(double value) {
    if (value<HUE_YELLOW_START || value>HUE_YELLOW_END) return(0);   /* out of the range      */
    if (value<=HUE_YELLOW_MAX) return((value-HUE_YELLOW_START)/(HUE_YELLOW_MAX-HUE_YELLOW_START));
    else return(1-((value-HUE_YELLOW_MAX)/(HUE_YELLOW_END-HUE_YELLOW_MAX)));
}
//---------------------------------------------------------------------------
double CalculateRules2(double hue, double saturation, double value, int color) {
    double ret_val,lower_sum=0;
    int RulesCounter=0;
    while (fuzzy_rules[RulesCounter].hue>=0) {

        // calculate only rules of the same color
        if (fuzzy_rules[RulesCounter].color == color) {
            /* hue functions */
            ret_val=color_value(fuzzy_rules[RulesCounter].hue,hue);
            if (ret_val==0) {
                RulesCounter++;
                continue;
            }
            /* saturation function */
            if (fuzzy_rules[RulesCounter].saturation==SATURATION_GREY) ret_val=ret_val*grey_value(saturation);
            if (fuzzy_rules[RulesCounter].saturation==SATURATION_ALMOST_GREY) ret_val=ret_val*almost_grey_value(saturation);
            if (fuzzy_rules[RulesCounter].saturation==SATURATION_TEND_GREY) ret_val=ret_val*tend_grey_value(saturation);
            if (fuzzy_rules[RulesCounter].saturation==SATURATION_MEDIUM_GREY) ret_val=ret_val*medium_grey_value(saturation);
            if (fuzzy_rules[RulesCounter].saturation==SATURATION_TEND_CLEAR) ret_val=ret_val*tend_clear_value(saturation);
            if (fuzzy_rules[RulesCounter].saturation==SATURATION_CLEAR) ret_val=ret_val*clear_value(saturation);
            if (ret_val==0) {
                RulesCounter++;
                continue;
            }
            /* value functions */
            if (fuzzy_rules[RulesCounter].value==VALUE_DARK) ret_val=ret_val*dark_value(value);
            if (fuzzy_rules[RulesCounter].value==VALUE_ALMOST_DARK) ret_val=ret_val*almost_dark_value(value);
            if (fuzzy_rules[RulesCounter].value==VALUE_TEND_DARK) ret_val=ret_val*tend_dark_value(value);
            if (fuzzy_rules[RulesCounter].value==VALUE_TEND_LIGHT) ret_val=ret_val*tend_light_value(value);
            if (fuzzy_rules[RulesCounter].value==VALUE_LIGHT) ret_val=ret_val*light_value(value);
            /* add the rule values */
            lower_sum=lower_sum+ret_val;
        }
        RulesCounter++; //chris - need to add counter or infinite loop  
    }
    return(lower_sum);
}

// need to call to SetRules before calling FindColor
long FindColor(short hue, short saturation, short value) {
    double max_membership = 0;
    int res = COLOR_LIGHT_GREY;
    for (int color_index=COLOR_WHITE; color_index<=COLOR_LIGHT_FUCIA; color_index++) {
        if (colors[color_index].color>=0) {
            double membership=CalculateRules2(hue,saturation,value,color_index);
            if (membership>max_membership) {
                max_membership=membership;
                res=color_index;
            }
        }
    }
    return(res);
}
//---------------------------------------------------------------------------


