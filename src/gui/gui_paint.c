/**
 * @file gui_paint.c
 * @brief Paint engine module for monochrome images, based on 1 bit per pixel memory map. 
 * This module allows to draw primitives and widgets on the screen.
 */
#include "gui_paint.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h> //memset() strlen()
#include <math.h>

#define TEXT_ALIGNMENT_ENABLED          /*!< Use text alignment to display */
#define PROGRESS_BAR_FILL_BACKGROUND    /*!< Fill the empty progress part with background color when \
                                           draw */
#define PROGRESS_BAR_OFFSET_PIX (2)     /*!< Progress offset from borders */
#define PROGRESS_BAR_HORIZONTAL         /*!< Enable horizontal progress bar, otherwise vertical */
#define MAX_NUM_LEN (10 + 1)            /*!< 32bit (number) + 1 byte (end of line) */

extern bool font_initializeSymbolBitmapDsc(bitmap_dsc_s *symbol, const font_s * font, uint32_t unicode_letter);

#define RECT_NUM_IN_CHIP_X 4 /*!< Rect number in chip row */
#define RECT_NUM_IN_CHIP_Y 4 /*!< Rect number in chip column */

/** Paint structure that includes an actual frame buffer and drawing parameters. */
paint_param_s paint;

static uint16_t get_centeredX(uint16_t line_width);

void paint_newImage(uint8_t *image, uint16_t width, uint16_t height, uint16_t rotate, color_t color)
{
    paint.image = image;

    paint.widthMemory = width;
    paint.heightMemory = height;
    paint.color = color;
    paint.width_in_bytes = (width % 8 == 0) ? (width / 8) : (width / 8 + 1);
    paint.height_in_bytes = height;

    paint.rotate = rotate;
    paint.mirror = MIRROR_NONE;

    if (rotate == ROTATE_0 || rotate == ROTATE_180) {
        paint.width = width;
        paint.height = height;
    } else {
        paint.width = height;
        paint.height = width;
    }
}

void paint_selectColor(color_t color)
{
    paint.color = color;
}

void paint_setRotate(uint16_t rotate)
{
    if (rotate == ROTATE_0 || rotate == ROTATE_90 || rotate == ROTATE_180 || rotate == ROTATE_270) {
        paint.rotate = rotate;
    }
}

void paint_setPixelInvert(uint16_t Xpoint, uint16_t Ypoint)
{
    uint16_t X, Y;
    uint32_t Addr;
    uint8_t Rdata;
    if (Xpoint > paint.width || Ypoint > paint.height || paint.image == NULL) {
        return;
    }

    switch (paint.rotate) {
        case 0:
            X = Xpoint;
            Y = Ypoint;
            break;
        case 90:
            X = paint.widthMemory - Ypoint - 1;
            Y = Xpoint;
            break;
        case 180:
            X = paint.widthMemory - Xpoint - 1;
            Y = paint.heightMemory - Ypoint - 1;
            break;
        case 270:
            X = Ypoint;
            Y = paint.heightMemory - Xpoint - 1;
            break;

        default:
            return;
    }

    switch (paint.mirror) {
        case MIRROR_NONE:
            break;
        case MIRROR_HORIZONTAL:
            X = paint.widthMemory - X - 1;
            break;
        case MIRROR_VERTICAL:
            Y = paint.heightMemory - Y - 1;
            break;
        case MIRROR_DIAGONAL:
            X = paint.widthMemory - X - 1;
            Y = paint.heightMemory - Y - 1;
            break;
        default:
            return;
    }

    Addr = X / 8 + Y * paint.width_in_bytes;
    if (Addr > paint.width_in_bytes * paint.height) {
        return;
    }

    Rdata = paint.image[Addr];
    if (Rdata & (0x80 >> (X % 8))) {
        paint.image[Addr] = Rdata & ~(0x80 >> (X % 8));
    } else {
        paint.image[Addr] = Rdata | (0x80 >> (X % 8));
    }
}

void paint_setPixel(uint16_t Xpoint, uint16_t Ypoint, color_t color)
{
    uint16_t X, Y;
    uint32_t Addr;
    uint8_t Rdata;
    if (Xpoint > paint.width || Ypoint > paint.height || paint.image == NULL) {
        return;
    }

    switch (paint.rotate) {
        case 0:
            X = Xpoint;
            Y = Ypoint;
            break;
        case 90:
            X = paint.widthMemory - Ypoint - 1;
            Y = Xpoint;
            break;
        case 180:
            X = paint.widthMemory - Xpoint - 1;
            Y = paint.heightMemory - Ypoint - 1;
            break;
        case 270:
            X = Ypoint;
            Y = paint.heightMemory - Xpoint - 1;
            break;

        default:
            return;
    }

    switch (paint.mirror) {
        case MIRROR_NONE:
            break;
        case MIRROR_HORIZONTAL:
            X = paint.widthMemory - X - 1;
            break;
        case MIRROR_VERTICAL:
            Y = paint.heightMemory - Y - 1;
            break;
        case MIRROR_DIAGONAL:
            X = paint.widthMemory - X - 1;
            Y = paint.heightMemory - Y - 1;
            break;
        default:
            return;
    }

    Addr = X / 8 + Y * paint.width_in_bytes;
    if (Addr > paint.width_in_bytes * paint.height) {
        return;
    }

    Rdata = paint.image[Addr];
    if (color) {
        paint.image[Addr] = Rdata | (0x80 >> (X % 8));
    } else {
        paint.image[Addr] = Rdata & ~(0x80 >> (X % 8));
    }
}

void paint_clear(color_t color)
{
    if (paint.image == NULL) {
        return;
    }
    uint16_t X, Y;
    uint32_t Addr;
    for (Y = 0; Y < paint.height_in_bytes; Y++) {
        for (X = 0; X < paint.width_in_bytes; X++) { // 8 pixel =  1 byte
            Addr = X + Y * paint.width_in_bytes;
            paint.image[Addr] = color;
        }
    }
}

void paint_clearWindow(uint16_t x1, uint16_t y1, uint16_t width, uint16_t height)
{
    uint16_t X, Y;
    for (Y = y1; Y <= y1+height; Y++) {
        for (X = x1; X <= x1+width; X++) {
            paint_setPixel(X, Y, !paint.color);
        }
    }
}

uint16_t paint_drawChar(uint16_t Xpoint, uint16_t Ypoint, const char ascii_char_num, font_s *Font,
                        paint_draw_style_e text_style)
{
    if (Xpoint > paint.width || Ypoint > paint.height) {
        return 0;
    }

    bitmap_dsc_s symbol;
    if (!font_initializeSymbolBitmapDsc(&symbol, Font, ascii_char_num)) {
        return 0;
    }
    uint16_t sym_width = symbol.width;

    /* Draw bit map of character */
    paint_drawBitMap(Xpoint + symbol.ofs_x,
                     Ypoint + (Font->line_height - Font->base_line) - symbol.height - symbol.ofs_y,
                     &symbol, text_style);

    if (text_style & PAINT_BACKGROUND) {
        /* Fill space via background color */
        uint16_t row, column;
        for (row = 0; row < FONT_GET_HEIGHT(Font); row++) {
            for (column = sym_width; column < sym_width + symbol.sym_offset; column++) {
                paint_setPixel(Xpoint + column, Ypoint + row, !paint.color);
            }
        }
        for (row = 0; row < symbol.ofs_y; row++) {
            for (column = 0; column < symbol.ofs_x; column++) {
                paint_setPixel(Xpoint + column, Ypoint + row, !paint.color);
            }
        }
    }

    if (text_style & PAINT_UNDERLINE) {
        uint16_t column;
        for (column = symbol.ofs_x; column < symbol.ofs_x + sym_width + symbol.sym_offset;
             column++) {
            paint_setPixelInvert(Xpoint + column, Ypoint + FONT_GET_HEIGHT(Font) - Font->base_line -
                                                      Font->underline_position);
        }
    }

    return sym_width + symbol.sym_offset;
}

void paint_drawStringEn(uint16_t x1, uint16_t y1, const char *pString, font_s *Font,
                        text_align_e text_align, paint_draw_style_e text_style)
{
    uint16_t Xpoint = x1;
    uint16_t Ypoint = y1;

    if (x1 > paint.width || y1 > paint.height || pString == NULL) {
        return;
    }

#ifdef TEXT_ALIGNMENT_ENABLED
    uint8_t last_sym_line_before = 0;
    uint8_t num_sym = 0;
    uint8_t last_space = 0;
    /* We should fit in edges, so compute pixels per line. Todo: drawLabel (with edges)  */
    uint16_t num_pix_per_line = paint.width - ((text_align == TEXT_ALIGN_LEFT) ? x1 : x1 * 2);
    uint16_t line_width = 0;
    uint16_t sym_width;
    while (pString) {
        sym_width = (uint16_t)FONT_GET_CHAR_WIDTH(Font, pString[num_sym]);
        if (sym_width < paint.width) {
            line_width += sym_width + FONT_GET_SYM_OFFSET(Font, pString[num_sym]);
        }
        if (line_width > num_pix_per_line || pString[num_sym] == '\n' || pString[num_sym] == '\0') {

            if (pString[num_sym] != ' ' && pString[num_sym] != '\n' && pString[num_sym] != '\0' &&
                last_space != 0) {
                while (num_sym > last_space) {
                    sym_width = (uint16_t)FONT_GET_CHAR_WIDTH(Font, pString[num_sym]);
                    if (sym_width < paint.width) {
                        line_width -= sym_width + FONT_GET_SYM_OFFSET(Font, pString[num_sym]);
                    }
                    num_sym -= 1;
                }
                // at this point: num_sym = last_space; so current symbol is space
            }

            /*draw*/
            if (text_align == TEXT_ALIGN_CENTER) {
                if (pString[num_sym] == ' ') {
                    /* Don't including space width in center calculation */
                    sym_width = (uint16_t)FONT_GET_CHAR_WIDTH(Font, pString[num_sym]);
                    line_width -= sym_width + FONT_GET_SYM_OFFSET(Font, pString[num_sym]);
                }
                Xpoint = get_centeredX(line_width);
                if (Xpoint == 0) {
                    Xpoint = x1;
                }
            } else {
                Xpoint = x1;
            }
            for (int i = last_sym_line_before; i < num_sym; i++) {
                Xpoint += paint_drawChar(Xpoint, Ypoint, pString[i], Font, text_style);
            }

            if ((Ypoint + FONT_GET_HEIGHT(Font)) > paint.height || pString[num_sym] == '\0') {
                return;
            }
            /* sym offset below like 'line_space' in lvgl */
            Ypoint += FONT_GET_HEIGHT(Font) + FONT_GET_SYM_OFFSET(Font, pString[num_sym]);

            line_width = 0;
            last_space = 0;
            last_sym_line_before = num_sym + 1; // next symbol
        } else if (pString[num_sym] == ' ') {
            last_space = num_sym;
        }
        num_sym++;
    };
#else
    while (*pString != '\0') {
        if (*pString <= FONT_GET_MAX_SYMBOL_NUM(Font)) {

            if ((Xpoint + FONT_GET_WIDTH(Font)) > paint.width || *pString == '\n') {
                Xpoint = x1;
                Ypoint += FONT_GET_HEIGHT(Font) + FONT_GET_SYM_OFFSET(Font, pString);
                if (*pString == '\n') {
                    pString++;
                    continue;
                }
            }

            if ((Ypoint + FONT_GET_HEIGHT(Font)) > paint.height) {
                Xpoint = x1;
                Ypoint = y1;
            }
            Xpoint += paint_drawChar(Xpoint, Ypoint, *pString, Font, text_style);
        }

        // The next character of the address
        pString++;
    }
#endif
}

void paint_drawNum(uint16_t Xpoint, uint16_t Ypoint, int32_t Number, font_s *Font,
                   text_align_e text_align, paint_draw_style_e text_style)
{
    uint8_t num_byte = MAX_NUM_LEN;
    uint8_t pStr[MAX_NUM_LEN];

    if (Xpoint > paint.width || Ypoint > paint.height) {
        return;
    }

    // Converts a number to a string in inverted order
    pStr[--num_byte] = '\0'; /*end of line*/
    do {
        pStr[--num_byte] = Number % 10 + '0';
        Number /= 10;
    } while (Number && num_byte);

    paint_drawStringEn(Xpoint, Ypoint, (const char *)(pStr + num_byte), Font, text_align,
                       text_style);
}

void paint_drawBitMap(uint16_t Xpoint, uint16_t Ypoint, const bitmap_dsc_s *img_dsc,
                      paint_draw_style_e draw_style)
{
    uint16_t row;
    uint16_t column;
    if (img_dsc == NULL || img_dsc->img_bitmap == NULL || Xpoint >= paint.width || Ypoint >= paint.height) {
        return;
    }
    const unsigned char *img_bitmap = img_dsc->img_bitmap;
    if (img_dsc->sym_offset) {
        /* symbol drawing */
        for (row = 0; row < img_dsc->height; row++) {
            for (column = 0; column < img_dsc->width; column++) {
                /* All pixels go continuously in bitmap, without byte alignment */
                uint16_t id = row * img_dsc->width + column;
                if (*img_bitmap & (0x80 >> (id % 8))) {
                    if (draw_style & PAINT_COLOR_INVERT) {
                        paint_setPixelInvert(Xpoint + column, Ypoint + row);
                    } else {
                        paint_setPixel(Xpoint + column, Ypoint + row, paint.color);
                    }
                } else {
                    if (draw_style & PAINT_BACKGROUND) {
                        paint_setPixel(Xpoint + column, Ypoint + row, !paint.color);
                    }
                }
                /* One pixel is 1 bits, so 8 pix in one byte */
                if (id % 8 == 7) {
                    img_bitmap++;
                }
            }
        }
    } else {
        for (row = 0; row < img_dsc->height; row++) {
            for (column = 0; column < img_dsc->width; column++) {

                if (*img_bitmap & (0x80 >> (column % 8))) {
                    if (draw_style & PAINT_COLOR_INVERT) {
                        paint_setPixelInvert(Xpoint + column, Ypoint + row);
                    } else {
                        paint_setPixel(Xpoint + column, Ypoint + row, paint.color);
                    }
                } else {
                    if (draw_style & PAINT_BACKGROUND) {
                        paint_setPixel(Xpoint + column, Ypoint + row, !paint.color);
                    }
                }
                // One pixel is 1 bits, so 8 pix in one byte
                if (column % 8 == 7) {
                    img_bitmap++;
                }
            }

            if (img_dsc->width % 8 != 0) { // same: column % 8 != 0
                img_bitmap++;
            }
        }
    }
}

void paint_drawProgressBar(guiWidget_bar_s *bar, int progressPercent)
{
    if (bar == NULL) {
        return;
    }
    uint16_t x1 = bar->position.x;               // vert_coord;
    uint16_t y1 = bar->position.y;               // horiz_coord;
    uint16_t x2 = bar->position.x + bar->width;  // vert_coord + side_v;
    uint16_t y2 = bar->position.y + bar->height; // horiz_coord - side_h;
    if (x1 > paint.width || y1 > paint.height || x2 > paint.width || y2 > paint.height) {
        return;
    }

    paint_drawRectangle(x1, y1, x2, y2, paint.color);
    if ((x2 - x1 > PROGRESS_BAR_OFFSET_PIX * 2) && (y2 - y1 > PROGRESS_BAR_OFFSET_PIX * 2)) {
        x1 += PROGRESS_BAR_OFFSET_PIX;
        y1 += PROGRESS_BAR_OFFSET_PIX;
        x2 -= PROGRESS_BAR_OFFSET_PIX;
        y2 -= PROGRESS_BAR_OFFSET_PIX;
    }

    if (progressPercent > 100) {
        progressPercent = 100;
    } else if (progressPercent < 0) {
        progressPercent = 0;
    }
#ifdef PROGRESS_BAR_HORIZONTAL
    uint16_t xStartFilling = x1;
    uint16_t xEndFilling = (x2 - x1) * progressPercent / 100 + x1;
    for (uint16_t Xpoint = xStartFilling; Xpoint <= xEndFilling; Xpoint++) {
        paint_drawLine(Xpoint, y1, Xpoint, y2, paint.color);
    }
    #ifdef PROGRESS_BAR_FILL_BACKGROUND
    for (uint16_t Xpoint = xEndFilling + 1; Xpoint <= x2; Xpoint++) {
        paint_drawLine(Xpoint, y1, Xpoint, y2, !paint.color);
    }
    #endif
#else /* PROGRESS_BAR_HORIZONTAL */
    uint16_t yStartFilling = y1;
    uint16_t yEndFilling = (y2 - y1) * progressPercent / 100 + y1;
    for (uint16_t Ypoint = yStartFilling; Ypoint <= yEndFilling; Ypoint++) {
        paint_drawLine(x1, Ypoint, x2, Ypoint, paint.color);
    }
    #ifdef PROGRESS_BAR_FILL_BACKGROUND
    for (uint16_t Ypoint = yEndFilling + 1; Ypoint <= y2; Ypoint++) {
        paint_drawLine(x1, Ypoint, x2, Ypoint, !paint.color);
    }
    #endif
#endif /* PROGRESS_BAR_HORIZONTAL */
}

void paint_drawBattery(uint16_t x, uint16_t y, int batteryPercent)
{
    /** The battery progress */
    guiWidget_bar_s widget_progress = {
        .position.x = x,
        .position.y = y,
        .width = 12,
        .height = 7,
    };
    paint_drawProgressBar(&widget_progress, batteryPercent);
    paint_drawLine(x+13, y+2, x+13, y+5, paint.color);
}

void paint_drawFilledRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, color_t color)
{
    if (x1 > paint.width || y1 > paint.height || x2 > paint.width || y2 > paint.height) {
        return;
    }

    for (uint16_t Ypoint = y1; Ypoint <= y2; Ypoint++) {
        for (uint16_t Xpoint = x1; Xpoint <= x2; Xpoint++) {
            paint_setPixel(Xpoint, Ypoint, color);
        }
    }
}

void paint_drawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, color_t color)
{
    if (x1 > paint.width || y1 > paint.height || x2 > paint.width || y2 > paint.height) {
        return;
    }

    paint_drawLine(x1, y1, x2, y1, color);
    paint_drawLine(x1, y1, x1, y2, color);
    paint_drawLine(x2, y2, x2, y1, color);
    paint_drawLine(x2, y2, x1, y2, color);
}

void paint_drawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, color_t color)
{
    uint16_t xAddr, yAddr;
    int x_direct, y_direct;
    int dx, dy;
    int Esp;

    if (x1 > paint.width || y1 > paint.height || x2 > paint.width || y2 > paint.height) {
        return;
    }

    xAddr = x1;
    yAddr = y1;
    dx = (int)x2 - (int)x1 >= 0 ? x2 - x1 : x1 - x2;
    dy = (int)y2 - (int)y1 <= 0 ? y2 - y1 : y1 - y2;

    // Increment direction, 1 is positive, -1 is counter;
    x_direct = x1 < x2 ? 1 : -1;
    y_direct = y1 < y2 ? 1 : -1;

    // Cumulative error
    Esp = dx + dy;

    while (true) {
        paint_setPixel(xAddr, yAddr, color);
        if (2 * Esp >= dy) {
            if (xAddr == x2) {
                break;
            }
            Esp += dy;
            xAddr += x_direct;
        }
        if (2 * Esp <= dx) {
            if (yAddr == y2) {
                break;
            }
            Esp += dx;
            yAddr += y_direct;
        }
    }
}

void paint_drawButton(guiWidget_button_s *btn)
{
    uint16_t color_enabled = btn->pressed ? !(btn->color) : btn->color;
    paint_drawFilledRectangle(btn->position.x, btn->position.y, btn->position.x + btn->width,
                              btn->position.y + btn->height,
                              color_enabled ? paint.color : !paint.color);
    /* align text at center of button */
#ifdef TEXT_ALIGNMENT_ENABLED
    const int text_off_x = 0;
#else
    int text_off_x = (btn->width - strlen(btn->text) * FONT_GET_WIDTH(DEFAULT_FONT)) / 2;
#endif
    const char *pString = btn->text;
    uint8_t line_num = 1;
    while (*pString != '\0') {
        if (*pString++ == '\n') {
            line_num += 1;
        }
    }
    int text_off_y = (btn->height - FONT_GET_HEIGHT(DEFAULT_FONT)*line_num) / 2;
    if (text_off_y < 0) {
        text_off_y = 0;
    }
    paint_drawStringEn(btn->position.x + text_off_x, btn->position.y + text_off_y, btn->text, DEFAULT_FONT,
                       TEXT_ALIGN_CENTER, PAINT_COLOR_INVERT);
}

void paint_drawChip(coordinate_s position, uint8_t episode)
{
    paint_clearWindow(position.x, position.y, 44, 44);

    if (episode >= CHIP_IMG_EPISODES_NUM) {
        return;
    }

    /* rect_fill_size_e */
    enum {
        RECT_NONE = 0,
        RECT_1 = 1,
        RECT_2 = 3,
        RECT_3 = 5,
        RECT_4 = 7,
        RECT_5 = 9,
        RECT_6 = 11,
        MAX_RECT_SIZE = RECT_6,
    };

    const uint8_t episode_rect_size[CHIP_IMG_EPISODES_NUM]
                                   [RECT_NUM_IN_CHIP_Y*RECT_NUM_IN_CHIP_X] = {
        {   
            RECT_2, RECT_1, RECT_2, RECT_3, RECT_3, RECT_2, RECT_1, RECT_2,
            RECT_4, RECT_3, RECT_2, RECT_1, RECT_5, RECT_4, RECT_3, RECT_2,
        }, // 1
        {   
            RECT_3, RECT_2, RECT_1, RECT_2, RECT_4, RECT_3, RECT_2, RECT_1,
            RECT_5, RECT_4, RECT_3, RECT_2, RECT_6, RECT_5, RECT_4, RECT_3,
        }, // 2
        {   
            RECT_4, RECT_3, RECT_2, RECT_1, RECT_5, RECT_4, RECT_3, RECT_2,
            RECT_6, RECT_5, RECT_4, RECT_3, RECT_6, RECT_6, RECT_5, RECT_4,
        }, // 3
        {   
            RECT_5, RECT_4, RECT_3, RECT_2, RECT_6, RECT_5, RECT_4, RECT_3,
            RECT_6, RECT_6, RECT_5, RECT_4, RECT_6, RECT_6, RECT_6, RECT_5,
        }, // 4
        {   
            RECT_6, RECT_5, RECT_4, RECT_3, RECT_6, RECT_6, RECT_5, RECT_4,
            RECT_6, RECT_6, RECT_6, RECT_6, RECT_6, RECT_6, RECT_6, RECT_6,
        }, // 5
        {   
            RECT_6, RECT_6, RECT_5, RECT_4, RECT_6, RECT_6, RECT_6, RECT_5,
            RECT_6, RECT_6, RECT_6, RECT_6, RECT_6, RECT_6, RECT_6, RECT_6,
        }, // 6
        {   
            RECT_6, RECT_6, RECT_6, RECT_5, RECT_6, RECT_6, RECT_6, RECT_6,
            RECT_6, RECT_6, RECT_6, RECT_6, RECT_6, RECT_6, RECT_6, RECT_6,
        }, // 7
        {   
            RECT_6, RECT_6, RECT_6, RECT_6, RECT_6, RECT_6, RECT_6, RECT_6,
            RECT_6, RECT_6, RECT_6, RECT_6, RECT_6, RECT_6, RECT_6, RECT_6,
        }, // 8
        {
            RECT_6, RECT_6, RECT_6, RECT_6, RECT_6, RECT_6, RECT_6, RECT_6, 
            RECT_6, RECT_6, RECT_6, RECT_6, RECT_5, RECT_6, RECT_6, RECT_6,
        }, // 9
        {
            RECT_6, RECT_6, RECT_6, RECT_6, RECT_6, RECT_6, RECT_6, RECT_6, 
            RECT_5, RECT_6, RECT_6, RECT_6, RECT_4, RECT_5, RECT_6, RECT_6,
        }, // 10
        {
            RECT_6, RECT_6, RECT_6, RECT_6, RECT_5, RECT_6, RECT_6, RECT_6, 
            RECT_4, RECT_5, RECT_6, RECT_6, RECT_3, RECT_4, RECT_5, RECT_6,
        }, // 11
        {
            RECT_5, RECT_6, RECT_6, RECT_6, RECT_4, RECT_5, RECT_6, RECT_6,
            RECT_3, RECT_4, RECT_5, RECT_6, RECT_2, RECT_3, RECT_4, RECT_5,
        }, // 12
        {   
            RECT_4, RECT_5, RECT_6, RECT_6, RECT_3, RECT_4, RECT_5, RECT_6,
            RECT_2, RECT_3, RECT_4, RECT_5, RECT_1, RECT_2, RECT_3, RECT_4,
        }, // 13
        {
            RECT_3, RECT_4, RECT_5, RECT_6, RECT_2, RECT_3, RECT_4, RECT_5, 
            RECT_1, RECT_2, RECT_3, RECT_4, RECT_2, RECT_1, RECT_2, RECT_3,
        }, // 14
        {   
            RECT_2, RECT_3, RECT_4, RECT_5, RECT_1, RECT_2, RECT_3, RECT_4,
            RECT_2, RECT_1, RECT_2, RECT_3, RECT_3, RECT_2, RECT_1, RECT_2,
        }, // 15
        {
            RECT_1, RECT_2, RECT_3, RECT_4, RECT_2, RECT_1, RECT_2, RECT_3, 
            RECT_3, RECT_2, RECT_1, RECT_2, RECT_4, RECT_3, RECT_2, RECT_1,
        }, // 16
    };
        
    coordinate_s next_rect = {.x = position.x, .y = position.y};
    /* Go to center of rectangle on Oy at next column */
    next_rect.y += MAX_RECT_SIZE / 2 + 1;
    for (int i = 0; i < RECT_NUM_IN_CHIP_Y; i++) {
        /* Go to center of rectangle on Ox at next row */
        next_rect.x = position.x + MAX_RECT_SIZE / 2 + 1;
        for (int j = 0; j < RECT_NUM_IN_CHIP_X; j++) {
            uint8_t rect_size = episode_rect_size[episode][i*RECT_NUM_IN_CHIP_X + j];
            paint_drawFilledRectangle(next_rect.x - rect_size / 2, next_rect.y - rect_size / 2,
                                      next_rect.x + rect_size / 2, next_rect.y + rect_size / 2,
                                      paint.color);
            next_rect.x += MAX_RECT_SIZE;
        }
        next_rect.y += MAX_RECT_SIZE;
    }
}

/**
 * @brief Get the centered X coordinate by line width.
 * @param line_width Width of line
 * @return uint16_t Centered X coordinate
 */
static uint16_t get_centeredX(uint16_t line_width)
{
    uint16_t new_x = (paint.width - line_width) / 2;
    if (new_x < paint.width) {
        return new_x;
    }
    return 0;
}
