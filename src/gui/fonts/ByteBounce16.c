/*******************************************************************************
 * Size: 16 px
 * Bpp: 1
 * Opts: --font ByteBounce.ttf -r 0x20-0x7F --size 16 --format lvgl --bpp 1 --no-compress -o ByteBounce16.c
 ******************************************************************************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

#ifndef BYTEBOUNCE16
#define BYTEBOUNCE16 1
#endif

#if BYTEBOUNCE16

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */
    0x0,

    /* U+0021 "!" */
    0xff, 0x30,

    /* U+0022 "\"" */
    0xde, 0xd2,

    /* U+0023 "#" */
    0x6d, 0xfd, 0xb3, 0x6f, 0xed, 0x80,

    /* U+0024 "$" */
    0x31, 0xff, 0x1e, 0x3f, 0xf7, 0x8c,

    /* U+0025 "%" */
    0xc7, 0x98, 0x61, 0x86, 0x78, 0xc0,

    /* U+0026 "&" */
    0x7b, 0x37, 0xbf, 0xd9, 0xf0,

    /* U+0027 "'" */
    0xf4,

    /* U+0028 "(" */
    0x7b, 0x6d, 0xb3,

    /* U+0029 ")" */
    0xcd, 0xb6, 0xde,

    /* U+002A "*" */
    0xcd, 0xe7, 0xb3,

    /* U+002B "+" */
    0x30, 0xcf, 0xcc, 0x30,

    /* U+002C "," */
    0xf4,

    /* U+002D "-" */
    0xfc,

    /* U+002E "." */
    0xf0,

    /* U+002F "/" */
    0x2, 0xc, 0x30, 0xc3, 0xc, 0x30, 0x40,

    /* U+0030 "0" */
    0x7b, 0x3c, 0xf3, 0xcd, 0xe0,

    /* U+0031 "1" */
    0x6e, 0x66, 0x6f,

    /* U+0032 "2" */
    0x7b, 0x30, 0xde, 0xc3, 0xf0,

    /* U+0033 "3" */
    0x7b, 0x31, 0x83, 0xcd, 0xe0,

    /* U+0034 "4" */
    0x18, 0xe7, 0xb6, 0xfc, 0x60,

    /* U+0035 "5" */
    0xff, 0xf, 0x83, 0xcd, 0xe0,

    /* U+0036 "6" */
    0x7b, 0xf, 0xb3, 0xcd, 0xe0,

    /* U+0037 "7" */
    0xfc, 0x31, 0x8c, 0x30, 0xc0,

    /* U+0038 "8" */
    0x7b, 0x37, 0xb3, 0xcd, 0xe0,

    /* U+0039 "9" */
    0x7b, 0x3c, 0xdf, 0xd, 0xe0,

    /* U+003A ":" */
    0xf3, 0xc0,

    /* U+003B ";" */
    0xf3, 0xd0,

    /* U+003C "<" */
    0x19, 0x99, 0x86, 0x18, 0x60,

    /* U+003D "=" */
    0xfc, 0xf, 0xc0,

    /* U+003E ">" */
    0xc3, 0xc, 0x33, 0x33, 0x0,

    /* U+003F "?" */
    0x7b, 0x31, 0x8c, 0x0, 0xc0,

    /* U+0040 "@" */
    0x7e, 0xc3, 0xdc, 0xf7, 0xf7, 0xdf, 0xc3, 0x7e,

    /* U+0041 "A" */
    0x7b, 0x3c, 0xff, 0xcf, 0x30,

    /* U+0042 "B" */
    0xfb, 0x3f, 0xb3, 0xcf, 0xe0,

    /* U+0043 "C" */
    0x7b, 0x3c, 0x30, 0xcd, 0xe0,

    /* U+0044 "D" */
    0xfb, 0x3c, 0xf3, 0xcf, 0xe0,

    /* U+0045 "E" */
    0xff, 0xf, 0x30, 0xc3, 0xf0,

    /* U+0046 "F" */
    0xff, 0xf, 0x30, 0xc3, 0x0,

    /* U+0047 "G" */
    0x7b, 0x3c, 0x37, 0xcd, 0xe0,

    /* U+0048 "H" */
    0xcf, 0x3f, 0xf3, 0xcf, 0x30,

    /* U+0049 "I" */
    0xf6, 0x66, 0x6f,

    /* U+004A "J" */
    0xc, 0x30, 0xc3, 0xcd, 0xe0,

    /* U+004B "K" */
    0xcf, 0x6f, 0x3c, 0xdb, 0x30,

    /* U+004C "L" */
    0xc3, 0xc, 0x30, 0xc3, 0xf0,

    /* U+004D "M" */
    0xc3, 0xe7, 0xff, 0xdb, 0xc3, 0xc3,

    /* U+004E "N" */
    0xcf, 0xbf, 0xf7, 0xcf, 0x30,

    /* U+004F "O" */
    0x7b, 0x3c, 0xf3, 0xcd, 0xe0,

    /* U+0050 "P" */
    0xfb, 0x3c, 0xfe, 0xc3, 0x0,

    /* U+0051 "Q" */
    0x7b, 0x3c, 0xf3, 0xd9, 0xb0,

    /* U+0052 "R" */
    0xfb, 0x3c, 0xfe, 0xdb, 0x30,

    /* U+0053 "S" */
    0x7b, 0x7, 0x83, 0xcd, 0xe0,

    /* U+0054 "T" */
    0xfc, 0xc3, 0xc, 0x30, 0xc0,

    /* U+0055 "U" */
    0xcf, 0x3c, 0xf3, 0xcd, 0xe0,

    /* U+0056 "V" */
    0xcf, 0x3c, 0xf3, 0x78, 0xc0,

    /* U+0057 "W" */
    0xc3, 0xdb, 0xdb, 0xdb, 0xdb, 0x7e,

    /* U+0058 "X" */
    0xcd, 0xe3, 0xc, 0x7b, 0x30,

    /* U+0059 "Y" */
    0xcf, 0x3c, 0xde, 0x30, 0xc0,

    /* U+005A "Z" */
    0xfc, 0x73, 0x9c, 0xe3, 0xf0,

    /* U+005B "[" */
    0xfc, 0xcc, 0xcc, 0xcf,

    /* U+005C "\\" */
    0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81,

    /* U+005D "]" */
    0xf3, 0x33, 0x33, 0x3f,

    /* U+005E "^" */
    0x31, 0xec, 0xc0,

    /* U+005F "_" */
    0xfc,

    /* U+0060 "`" */
    0xf4,

    /* U+0061 "a" */
    0x78, 0x37, 0xf3, 0x7c,

    /* U+0062 "b" */
    0xc3, 0xec, 0xf3, 0xcf, 0xe0,

    /* U+0063 "c" */
    0x7b, 0x3c, 0x33, 0x78,

    /* U+0064 "d" */
    0xd, 0xfc, 0xf3, 0xcd, 0xf0,

    /* U+0065 "e" */
    0x7b, 0x3f, 0xb0, 0x78,

    /* U+0066 "f" */
    0x7c, 0xfc, 0xcc,

    /* U+0067 "g" */
    0x7f, 0x3c, 0xdf, 0xd, 0xe0,

    /* U+0068 "h" */
    0xc3, 0xec, 0xf3, 0xcf, 0x30,

    /* U+0069 "i" */
    0xcf, 0xf0,

    /* U+006A "j" */
    0x61, 0xb6, 0xf0,

    /* U+006B "k" */
    0xc3, 0x3d, 0xbc, 0xdb, 0x30,

    /* U+006C "l" */
    0xdb, 0x6c, 0xc0,

    /* U+006D "m" */
    0xfe, 0xdb, 0xdb, 0xdb, 0xdb,

    /* U+006E "n" */
    0xfb, 0x3c, 0xf3, 0xcc,

    /* U+006F "o" */
    0x7b, 0x3c, 0xf3, 0x78,

    /* U+0070 "p" */
    0xfb, 0x3c, 0xf3, 0xfb, 0x0,

    /* U+0071 "q" */
    0x7f, 0x3c, 0xf3, 0x7c, 0x30,

    /* U+0072 "r" */
    0x7c, 0xcc, 0xc0,

    /* U+0073 "s" */
    0x7b, 0x7, 0x83, 0xf8,

    /* U+0074 "t" */
    0x6f, 0x66, 0x63,

    /* U+0075 "u" */
    0xcf, 0x3c, 0xf3, 0x78,

    /* U+0076 "v" */
    0xcf, 0x3c, 0xde, 0x30,

    /* U+0077 "w" */
    0xc3, 0xdb, 0xdb, 0xdb, 0x7e,

    /* U+0078 "x" */
    0xcd, 0xe3, 0x1e, 0xcc,

    /* U+0079 "y" */
    0xcf, 0x3c, 0xdf, 0xd, 0xe0,

    /* U+007A "z" */
    0xfc, 0x63, 0x18, 0xfc,

    /* U+007B "{" */
    0x36, 0x6c, 0xc6, 0x63,

    /* U+007C "|" */
    0xff, 0xff,

    /* U+007D "}" */
    0xc6, 0x63, 0x36, 0x6c,

    /* U+007E "~" */
    0x77, 0xb8
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 80, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1, .adv_w = 48, .box_w = 2, .box_h = 6, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 3, .adv_w = 96, .box_w = 5, .box_h = 3, .ofs_x = 0, .ofs_y = 5},
    {.bitmap_index = 5, .adv_w = 128, .box_w = 7, .box_h = 6, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 11, .adv_w = 112, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 17, .adv_w = 128, .box_w = 7, .box_h = 6, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 23, .adv_w = 112, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 28, .adv_w = 48, .box_w = 2, .box_h = 3, .ofs_x = 0, .ofs_y = 5},
    {.bitmap_index = 29, .adv_w = 64, .box_w = 3, .box_h = 8, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 32, .adv_w = 64, .box_w = 3, .box_h = 8, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 35, .adv_w = 112, .box_w = 6, .box_h = 4, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 38, .adv_w = 112, .box_w = 6, .box_h = 5, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 42, .adv_w = 48, .box_w = 2, .box_h = 3, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 43, .adv_w = 112, .box_w = 6, .box_h = 1, .ofs_x = 0, .ofs_y = 5},
    {.bitmap_index = 44, .adv_w = 48, .box_w = 2, .box_h = 2, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 45, .adv_w = 128, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 52, .adv_w = 112, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 57, .adv_w = 80, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 60, .adv_w = 112, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 65, .adv_w = 112, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 70, .adv_w = 112, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 75, .adv_w = 112, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 80, .adv_w = 112, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 85, .adv_w = 112, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 90, .adv_w = 112, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 95, .adv_w = 112, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 100, .adv_w = 48, .box_w = 2, .box_h = 5, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 102, .adv_w = 48, .box_w = 2, .box_h = 6, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 104, .adv_w = 96, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 109, .adv_w = 112, .box_w = 6, .box_h = 3, .ofs_x = 0, .ofs_y = 4},
    {.bitmap_index = 112, .adv_w = 96, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 117, .adv_w = 112, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 122, .adv_w = 144, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 130, .adv_w = 112, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 135, .adv_w = 112, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 140, .adv_w = 112, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 145, .adv_w = 112, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 150, .adv_w = 112, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 155, .adv_w = 112, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 160, .adv_w = 112, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 165, .adv_w = 112, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 170, .adv_w = 80, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 173, .adv_w = 112, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 178, .adv_w = 112, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 183, .adv_w = 112, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 188, .adv_w = 144, .box_w = 8, .box_h = 6, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 194, .adv_w = 112, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 199, .adv_w = 112, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 204, .adv_w = 112, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 209, .adv_w = 112, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 214, .adv_w = 112, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 219, .adv_w = 112, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 224, .adv_w = 112, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 229, .adv_w = 112, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 234, .adv_w = 112, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 239, .adv_w = 144, .box_w = 8, .box_h = 6, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 245, .adv_w = 112, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 250, .adv_w = 112, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 255, .adv_w = 112, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 260, .adv_w = 80, .box_w = 4, .box_h = 8, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 264, .adv_w = 128, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 271, .adv_w = 80, .box_w = 4, .box_h = 8, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 275, .adv_w = 112, .box_w = 6, .box_h = 3, .ofs_x = 0, .ofs_y = 5},
    {.bitmap_index = 278, .adv_w = 112, .box_w = 6, .box_h = 1, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 279, .adv_w = 48, .box_w = 2, .box_h = 3, .ofs_x = 0, .ofs_y = 5},
    {.bitmap_index = 280, .adv_w = 112, .box_w = 6, .box_h = 5, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 284, .adv_w = 112, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 289, .adv_w = 112, .box_w = 6, .box_h = 5, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 293, .adv_w = 112, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 298, .adv_w = 112, .box_w = 6, .box_h = 5, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 302, .adv_w = 80, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 305, .adv_w = 112, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 310, .adv_w = 112, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 315, .adv_w = 48, .box_w = 2, .box_h = 6, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 317, .adv_w = 64, .box_w = 3, .box_h = 7, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 320, .adv_w = 112, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 325, .adv_w = 64, .box_w = 3, .box_h = 6, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 328, .adv_w = 144, .box_w = 8, .box_h = 5, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 333, .adv_w = 112, .box_w = 6, .box_h = 5, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 337, .adv_w = 112, .box_w = 6, .box_h = 5, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 341, .adv_w = 112, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 346, .adv_w = 112, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 351, .adv_w = 80, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 354, .adv_w = 112, .box_w = 6, .box_h = 5, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 358, .adv_w = 80, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 361, .adv_w = 112, .box_w = 6, .box_h = 5, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 365, .adv_w = 112, .box_w = 6, .box_h = 5, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 369, .adv_w = 144, .box_w = 8, .box_h = 5, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 374, .adv_w = 112, .box_w = 6, .box_h = 5, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 378, .adv_w = 112, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 383, .adv_w = 112, .box_w = 6, .box_h = 5, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 387, .adv_w = 80, .box_w = 4, .box_h = 8, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 391, .adv_w = 48, .box_w = 2, .box_h = 8, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 393, .adv_w = 80, .box_w = 4, .box_h = 8, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 397, .adv_w = 128, .box_w = 7, .box_h = 2, .ofs_x = 0, .ofs_y = 4}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/



/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 32, .range_length = 95, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    }
};



/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LV_VERSION_CHECK(8, 0, 0)
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 1,
    .bpp = 1,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LV_VERSION_CHECK(8, 0, 0)
    .cache = &cache
#endif
};


/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LV_VERSION_CHECK(8, 0, 0)
const lv_font_t ByteBounce16 = {
#else
lv_font_t ByteBounce16 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 10,          /*The maximum line height required by the font*/
    .base_line = 0,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -2,
    .underline_thickness = 1,
#endif
    .dsc = &font_dsc           /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
};



#endif /*#if BYTEBOUNCE16*/

