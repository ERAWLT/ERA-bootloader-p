/**
 * @file fonts.h
 * @brief The header file that allows to integrate fonts based on lvgl (version 8) into the project.
 */

#ifndef __FONTS_H
#define __FONTS_H

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stddef.h>

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
    #include "lvgl.h"
#else
    #include "lvgl/lvgl.h"
#endif

/* Exported macro ------------------------------------------------------------*/
#define FONT_GET_SYM_OFFSET(pFont, unicode)  (font_getSymOffset(pFont, unicode))
#define FONT_GET_MAX_SYMBOL_NUM(pFont)       (get_unicode_last(pFont))
#define FONT_GET_HEIGHT(pFont)               (pFont->line_height)
#define FONT_GET_CHAR_BITMAP(pFont, unicode) (lv_font_get_bitmap_fmt_txt(pFont, unicode))
#define FONT_GET_CHAR_WIDTH(pFont, unicode)  (font_getWidth(pFont, unicode))

#ifdef REDUCE_FLASH_USAGE_BY_REMOVING_FONTS
    #define DEFAULT_FONT      (&PixelOperator8)
    #define HWLT_FONT_8       DEFAULT_FONT
    #define HWLT_FONT_8_BOLD  DEFAULT_FONT
    #define HWLT_FONT_16_BOLD DEFAULT_FONT
#else
    #define HWLT_FONT_8       (&PixelOperator8)
    #define HWLT_FONT_8_BOLD  (&ByteBounce16)
    #define HWLT_FONT_16_BOLD (&ByteBounce32)
    #define DEFAULT_FONT      HWLT_FONT_16_BOLD
#endif

/* Main font type */
typedef lv_font_t font_s;

/* Exported variables --------------------------------------------------------*/
extern font_s ByteBounce32;
extern font_s ByteBounce16;
extern font_s PixelOperator8;

uint16_t font_getWidth(const font_s *font, uint32_t unicode_letter);
uint16_t font_getSymOffset(const font_s *font, uint32_t unicode_letter);

static inline uint16_t get_unicode_last(const font_s * font)
{
    if (font == NULL) {
        return 0;
    }
    lv_font_fmt_txt_dsc_t * fdsc = (lv_font_fmt_txt_dsc_t *)font->dsc;
    return fdsc->cmaps[0].range_start + fdsc->cmaps[0].range_length;
}

#endif /* __FONTS_H */
