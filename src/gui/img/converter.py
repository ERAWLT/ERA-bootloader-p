from PIL import Image
from os import listdir, mkdir
from os.path import isfile, join, getmtime, isdir

# png_path = 'src/gui/png/gears_1.png'

FILE_IN_DIR = 'src/gui/png/'
FILE_OUT_DIR = 'src/gui/img/'

PREFIX_NAME = "img_"

INVERT = True
AVERAGE_VALUE = True

def create_c_arr(png_path):
    name = PREFIX_NAME + png_path.split('/')[-1].split('.')[0]
    im = Image.open(png_path) # Can be many different formats.
    pix = im.load()
    print('_'*20, png_path, sep='\n')
    print (im.size)  # Get the width and hight of the image for iterating over
    print (pix[0,0])  # Get the RGBA Value of the a pixel of an image

    w,h = im.size
    w_in_bytes = w//8 if w%8 == 0 else w//8 + 1
    arr = [[0 for j in range(w_in_bytes)] for i in range(h)]
    max_v = sum(pix[0,0])
    min_v = sum(pix[0,0])
    for y in range(h):
        for x in range(w):
            value = sum(pix[x, y])
            if value > max_v:
                max_v = value
            elif value < min_v:
                min_v = value
    mid_v = (max_v - min_v) / 2 + min_v
    print("max = %d, min = %d, mid = %d"%(max_v, min_v, mid_v))
    if not AVERAGE_VALUE:
        mid_v = min_v
    for y in range(h):
        for x in range(w):
            value = not INVERT if sum(pix[x, y]) > mid_v else INVERT
            # addr = x / 8 + y * w_in_bytes;
            arr[y][x//8] |= 0x80 >> (x % 8) if value else 0
            print('0' if value else '-', end='')
        print('')

    macro1 = '#include "gui_widgets.h"\
    \n\
    \nconst uint8_t %s_map[] = {\
    \n'%(name)

    macro2 = '};\
    \n\
    \nconst bitmap_dsc_s %s = {\
    \n'%(name)

    macro3 = '    .img_bitmap = %s_map,\
    \n};\n'%(name)
    
    c_name = FILE_OUT_DIR + name + '_array.c'
    with open(c_name, 'w') as f:
        f.write(macro1)
        for y in range(h):
            line = '    '
            for x in range(w_in_bytes):
                line += '0x%2.2X, '%(arr[y][x])
            f.write(line+'\n')

        f.write(macro2)
        line = '    .width = %2.1d,\n'%(w)
        f.write(line)
        line = '    .height = %2.1d,\n'%(h)
        f.write(line)

        f.write(macro3)

if __name__ == "__main__":
    if not isdir(FILE_IN_DIR):
        exit()
    if not isdir(FILE_OUT_DIR):
        mkdir(FILE_OUT_DIR)
    onlyfiles = [ FILE_IN_DIR+f for f in listdir(FILE_IN_DIR) if (isfile(join(FILE_IN_DIR, f)) and (".png" in f)) ]
    onlyfiles.sort(key = getmtime)
    for png_path in list(onlyfiles):
        create_c_arr(png_path)
