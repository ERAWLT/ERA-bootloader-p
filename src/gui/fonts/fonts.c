/**
 * @file fonts.c
 * @brief The file contains methods to support lvgl's fonts in the project.
 */
#include "fonts.h"
#include "gui_widgets.h"

#define MAX_HEIGHT (32) /*!< Maximux height for used fonts */
#define MAX_WIDTH  (32) /*!< Maximum width for used fonts */

/**
 * @brief Get width of symbol.
 * @param font lvgl font @ref font_s
 * @param unicode_letter UNICODE letter code
 * @return uint16_t Width of the symbol.
 * @note 'letter_next' is the same 'letter' now, 
 *       font_dsc->kern_dsc is NULL for ERA fonts, so letter_next is not used.
 */
uint16_t font_getWidth(const font_s * font, uint32_t unicode_letter)
{
    if (font == NULL) {
        return 0;
    }
    static lv_font_glyph_dsc_t dsc_out;
    if (!lv_font_get_glyph_dsc_fmt_txt(font, &dsc_out, unicode_letter, unicode_letter)) {
        return 0;
    }
    return dsc_out.box_w;
}

/**
 * @brief Get minimal offset to next symbol
 * @param font lvgl font @ref font_s
 * @param unicode_letter UNICODE letter code
 * @return uint16_t Offset to the next symbol.
 */
uint16_t font_getSymOffset(const font_s * font, uint32_t unicode_letter)
{
    if (font == NULL) {
        return 0;
    }
    static lv_font_glyph_dsc_t dsc_out;
    if (!lv_font_get_glyph_dsc_fmt_txt(font, &dsc_out, unicode_letter, unicode_letter)) {
        return 0;
    }
    uint16_t off = dsc_out.adv_w - dsc_out.box_w;
    if (font == HWLT_FONT_8) {
        off -= 1; /* like letter_space in lvgl font's style */
    }
    return off < MAX_WIDTH ? off : 1;
}

/**
 * @brief Fill in all fields of the character descriptor to draw.
 * @param symbol @ref bitmap_dsc_s
 * @param font lvgl font @ref font_s
 * @param unicode_letter UNICODE letter code
 * @return true on success, false otherwise.
 */
bool font_initializeSymbolBitmapDsc(bitmap_dsc_s *symbol, const font_s * font, uint32_t unicode_letter)
{
    if (font == NULL || symbol == NULL) {
        return false;
    }
    static lv_font_glyph_dsc_t dsc_out;
    if (!lv_font_get_glyph_dsc_fmt_txt(font, &dsc_out, unicode_letter, unicode_letter)) {
        return false;
    }
    symbol->img_bitmap = FONT_GET_CHAR_BITMAP(font, unicode_letter);
    symbol->width = dsc_out.box_w;
    symbol->height = dsc_out.box_h;
    symbol->ofs_x = dsc_out.ofs_x;
    symbol->ofs_y = dsc_out.ofs_y;
    uint16_t off = dsc_out.adv_w - dsc_out.box_w;
    if (font == HWLT_FONT_8) {
        off -= 1; /* like letter_space in lvgl font's style */
    }
    symbol->sym_offset = off < MAX_WIDTH ? off : 1;
    return true;
}