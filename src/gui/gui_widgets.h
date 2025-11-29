/**
 * @file gui_widgets.h
 * @brief GUI widgets header.
 */

#ifndef _GUI_WIDGETS_H_
#define _GUI_WIDGETS_H_

#include "stdint.h"
#include "stdbool.h"
#include "stdio.h"
#include "stddef.h"
#include "fonts.h"

/** Type of an image bitmap descriptor, 1 bit per pixel only */
typedef struct {
    const uint8_t *img_bitmap; /*!< Pointer to img bitmap */
    uint16_t width;            /*!< Bitmap width */
    uint16_t height;           /*!< Bitmap height */
    int8_t ofs_x;              /*!< x offset of the left pad */
    int8_t ofs_y;              /*!< y offset of the top of the line*/
    uint8_t sym_offset;        /*!< Minimal offset to next symbol */
} bitmap_dsc_s;

/** Type for supported color, black or white now */
typedef uint8_t color_t;

/** Type allows to describe a method for printing text lines */
typedef enum {
    TEXT_ALIGN_CENTER, /*!< Flag to center text lines by width */
    TEXT_ALIGN_LEFT,   /*!< Flag to align text lines left */
} text_align_e;

/** Coordinates to start drawing an object, always constant now */
typedef struct {
    uint16_t x; /*!< Horizontal position */
    uint16_t y; /*!< Vertical position */
} coordinate_s;

typedef struct {
    coordinate_s position;   /*!< @ref coordinate_s */
    const char *text;        /*!< Text to show */
    font_s *font;            /*!< Font to draw text */
    text_align_e text_align; /*!< @ref text_align_e */
} guiWidget_label_s;

typedef struct {
    coordinate_s position;       /*!< @ref coordinate_s */
    char *(*getLabelText)(void); /*!< Function to get text to show */
    font_s *font;                /*!< Font to draw text */
    text_align_e text_align;     /*!< @ref text_align_e */
} guiWidget_labelAnimation_s;

typedef struct {
    coordinate_s position;       /*!< @ref coordinate_s */
    const bitmap_dsc_s *img_dsc; /*!< Pointer to @ref bitmap_dsc_s */
} guiWidget_img_s;

typedef struct {
    coordinate_s position;             /*!< @ref coordinate_s */
    uint8_t cur_num;                   /*!< Current drawing pointer number */
    const uint8_t img_num_in_sequence; /*!< Length of pointer array */
    const bitmap_dsc_s **img_dsc_seq;  /*!< Array of pointers to @ref bitmap_dsc_s */
} guiWidget_imgAnimation_s;

typedef struct {
    coordinate_s position;    /*!< @ref coordinate_s */
    uint16_t width;           /*!< Progress bar width */
    uint16_t height;          /*!< Progress bar height */
    int (*getProgress)(void); /*!< Function to get current progress. Return value in range 0 to 100
                                 by default */
} guiWidget_bar_s;

typedef struct {
    coordinate_s position;          /*!< @ref coordinate_s */
    uint16_t width;                 /*!< Button width */
    uint16_t height;                /*!< Button height */
    const char *text;               /*!< Text on button */
    bool pressed;                   /*!< Flag - whether the button is pressed now */
    color_t color;                  /*!< Base color of button, inverts when pressed */
    void (*pressedCallback)(void);  /*!< Function to call when button is released */
    void (*releasedCallback)(void); /*!< Function to call when button is released */
} guiWidget_button_s;

#endif /* _GUI_WIDGETS_H_ */
