/**
 * @file gui_paint.h
 * @brief Header for paint engine.
 */
#ifndef _GUI_PAINT_H_
#define _GUI_PAINT_H_

#include <stdint.h>

#include "fonts.h"
#include "gui_widgets.h"

/**
 * Display rotate
 */
#define ROTATE_0   0
#define ROTATE_90  90
#define ROTATE_180 180
#define ROTATE_270 270

#define CHIP_IMG_FILLED_EPISODED 8  /*!< The episode with the fully painted over chip */
#define CHIP_IMG_EPISODES_NUM 16    /*!< Episodes while drawing the progress */

/**
 * Mirror display
 */
typedef enum {
    MIRROR_NONE = 0x00,       /*!< Display as is, no mirror */
    MIRROR_HORIZONTAL = 0x01, /*!< Display using horizontal flip */
    MIRROR_VERTICAL = 0x02,   /*!< Display using vertical flip */
    MIRROR_DIAGONAL = 0x03,   /*!< Display using diagonal flip */
} paint_mirror_image_e;

/**
 * Image attributes
 */
typedef struct {
    uint8_t *image;           /*!< Picture image framebuffer */
    uint16_t width;           /*!< Picture width */
    uint16_t height;          /*!< Picture height */
    uint16_t widthMemory;     /*!< Picture width in memory */
    uint16_t heightMemory;    /*!< Picture height in memory */
    color_t color;            /*!< Picture color */
    uint16_t rotate;          /*!< Picture rotate */
    uint16_t mirror;          /*!< Display picture mirror */
    uint16_t width_in_bytes;  /*!< Picture width_in_bytes */
    uint16_t height_in_bytes; /*!< Picture height_in_bytes */
} paint_param_s;

/** Type allows to describe a method for printing objects on screen */
typedef enum {
    PAINT_COLOR = (0x01 << 0),        /*!< Fill color part for picture (default style) */
    PAINT_BACKGROUND = (0x01 << 1),   /*!< Fill picture (simbols) background */
    PAINT_COLOR_INVERT = (0x01 << 2), /*!< Invert picture (text) relative to current color at place
                                       */
    PAINT_UNDERLINE = (0x01 << 3),    /*!< Underline the output text */
    PAINT_BOLD = (0x01 << 4),         /*!< Double the size for each point */
} paint_draw_style_e;

/**
 * @brief Create image
 * @param[in] image Pointer to the image cache
 * @param width     The width of the picture
 * @param height    The height of the picture
 * @param rotate    Whether the picture is inverted
 * @param color     Main color.
 * @return None
 */
void paint_newImage(uint8_t *image, uint16_t width, uint16_t height, uint16_t rotate,
                    color_t color);
/**
 * @brief	Select image color
 * @param color   New main color to draw
 * @return None
 */
void paint_selectColor(color_t color);

/**
 * @brief	Select image Rotate
 * @param rotate      0,90,180,270
 * @return None
 * @note Not used.
 */
void paint_setRotate(uint16_t rotate);

/**
 * @brief	Draw the pixel by x,y coordinates.
 * @param Xpoint  At point X
 * @param Ypoint  At point Y
 * @param color   Painted colors
 * @return None
 */
void paint_setPixel(uint16_t Xpoint, uint16_t Ypoint, color_t color);

/**
 * @brief	Invert the pixel by x,y coordinates.
 * @param Xpoint  At point X
 * @param Ypoint  At point Y
 * @return None
 */
void paint_setPixelInvert(uint16_t Xpoint, uint16_t Ypoint);

/**
 * @brief	Clear the color of the picture
 * @param color      Painted colors, should be background color.
 * @return None
 */
void paint_clear(color_t color);

/**
 * @brief	Fill window with background color.
 * @param x1     X starting point
 * @param y1     Y starting point
 * @param width  The width of the window
 * @param height The height of the window
 * @return None
 */
void paint_clearWindow(uint16_t x1, uint16_t y1, uint16_t width, uint16_t height);

/**
 * @brief	Show English characters
 * @param Xpoint           X coordinate
 * @param Ypoint           Y coordinate
 * @param ascii_char_num   To display the English character number
 * @param[in] Font         Pointer to font to display characters
 * @param text_style       Method to draw the character, @ref paint_draw_style_e.
 * @return uint16_t The width of character that has been drawn
 */
uint16_t paint_drawChar(uint16_t x1, uint16_t y1, const char ascii_char_num, font_s *Font,
                        paint_draw_style_e text_style);

/**
 * @brief	Display the string
 * @param x1          X coordinate
 * @param y1          Y coordinate
 * @param pString     The first address of the English string to be displayed
 * @param[in] Font    Pointer to font to display characters
 * @param text_align  Text align.
 * @param text_style  Method to draw characters, @ref paint_draw_style_e.
 * @return None
 */
void paint_drawStringEn(uint16_t x1, uint16_t y1, const char *pString, font_s *Font,
                        text_align_e text_align, paint_draw_style_e text_style);

/**
 * @brief	Display number
 * @param Xpoint      X coordinate
 * @param Ypoint      Y coordinate
 * @param Number      The number displayed
 * @param[in] Font    Pointer to font to display characters
 * @param text_align  Text align.
 * @param text_style  Method to draw characters, @ref paint_draw_style_e.
 * @return None
 * @note Not used.
 */
void paint_drawNum(uint16_t Xpoint, uint16_t Ypoint, int32_t Nummber, font_s *Font,
                   text_align_e text_align, paint_draw_style_e text_style);

/**
 * @brief Display monochrome bitmap at choosed position.
 * @param Xpoint        X coordinate
 * @param Ypoint        Y coordinate
 * @param[in] img_dsc   A picture data converted to a bitmap
 * @param draw_style    Method to draw image, @ref paint_draw_style_e.
 * @return None
 */
void paint_drawBitMap(uint16_t Xpoint, uint16_t Ypoint, const bitmap_dsc_s *img_dsc,
                      paint_draw_style_e draw_style);

/**
 * @brief	Draw line from x1,y1 to x2,y2.
 * @param x1    X starting point
 * @param y1    Y starting point
 * @param x2    X end point
 * @param y2    Y end point
 * @param color Line color
 * @return None
 */
void paint_drawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, color_t color);

/**
 * @brief Draw a filled rectangle
 * @param x1    X starting point
 * @param y1    Y starting point
 * @param x2    X end point
 * @param y2    Y end point
 * @param color Fill color
 * @return None
 */
void paint_drawFilledRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, color_t color);

/**
 * @brief Draw a rectangle
 * @param x1    X starting point
 * @param y1    Y starting point
 * @param x2    X end point
 * @param y2    Y end point
 * @param color Line color
 * @return None
 */
void paint_drawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, color_t color);

/**
 * @brief Draw the button widget.
 * @param[in] btn Pointer to the button
 * @return None
 */
void paint_drawButton(guiWidget_button_s *btn);

/**
 * @brief Draw the progress bar widget.
 * @param[in] bar Pointer to the progress bar.
 * @param progressPercent Fill percentage
 * @return None
 */
void paint_drawProgressBar(guiWidget_bar_s *bar, int progressPercent);

/**
 * @brief Draw the battery level.
 * @param x    X starting point
 * @param y    Y starting point
 * @param batteryPercent Fill battery percentage
 * @return None
 */
void paint_drawBattery(uint16_t x, uint16_t y, int batteryPercent);

/**
 * @brief Draw the chip's image.
 * @param position Coordinates to start drawing an chip object.
 * @param episode Episode to draw (from 0 to 7).
 * @return None.
 * @note Field size: 44 x 44
 */
void paint_drawChip(coordinate_s position, uint8_t episode);

#endif /*_GUI_PAINT_H_*/
