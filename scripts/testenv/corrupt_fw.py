import argparse
import os
from ctypes import *
from enum import IntEnum
from random import randint

IMAGE_MAGIC = 0x96f3b83d

class image_version_s(Structure):
    _pack_ = 1
    _fields_ = [
        ("iv_major", c_uint8),
        ("iv_minor", c_uint8),
        ("iv_revision", c_uint16),
        ("iv_build_num", c_uint32),
    ]

class image_header_s(Structure):
    _pack_ = 1
    _fields_ = [
        ("ih_magic", c_uint32),
        ("ih_load_addr", c_uint32),
        ("ih_hdr_size", c_uint16),
        ("ih_protect_tlv_size", c_uint16),
        ("ih_img_size", c_uint32),
        ("ih_flags", c_uint32),
        ("ih_ver", image_version_s),
        ("_pad1", c_uint32),
    ]


class image_tlv_info_s(Structure):
    _pack_ = 1
    _fields_ = [
        ("it_magic", c_uint16),
        ("it_tlv_tot", c_uint16),
    ]

class image_tlv_s(Structure):
    _pack_ = 1
    _fields_ = [
        ("it_type", c_uint16),
        ("it_len", c_uint16),
    ]

def get_offset_for_image_header_field(name):
    print('\tsearch for currupt in %s'%(str(name).upper()))
    offset = 0
    for n, t in image_header_s()._fields_:
        if name in n:
            return offset
        if not isinstance(t(), Structure):
            offset += sizeof(t)
        elif isinstance(t(), image_version_s):
            for nn, tt in image_version_s()._fields_:
                if name in nn:
                    return offset
                offset += sizeof(tt)
    return offset

def get_header(file, offset):
    file.seek(offset)
    hdr = image_header_s()
    for n, t in hdr._fields_:
        if not isinstance(t(), Structure):
            setattr(hdr, n, int.from_bytes(file.read(sizeof(t)), byteorder='little'))
        elif isinstance(t(), image_version_s):
            for nn, tt in hdr.ih_ver._fields_:
                setattr(hdr.ih_ver, nn, int.from_bytes(file.read(sizeof(tt)), byteorder='little'))
    return hdr

def get_tlvInfo(file, offset):
    file.seek(offset)
    tlvInfo = image_tlv_info_s()
    for n, t in tlvInfo._fields_:
        if not isinstance(t(), Structure):
            setattr(tlvInfo, n, int.from_bytes(file.read(sizeof(t)), byteorder='little'))
    return tlvInfo

def corrupt_byte_file(file, offset):
    file.seek(offset)
    byte = int.from_bytes(file.read(1), byteorder='little')
    print("offset %d, byte value:"%(offset), byte)
    file.seek(offset)

    newb = randint(0, 255)
    while(newb == byte): newb = randint(0, 255)
    print("corrupt with:", newb)
    newb = newb.to_bytes(1, byteorder='little')
    file.write(newb)

if __name__=="__main__":
    parser = argparse.ArgumentParser(
                    prog='corruptFw',
                    description='Allow to corrupt the fw binary file.',
                    usage='')
    parser.add_argument('-i', '--input', help="base file")
    parser.add_argument('-o', '--output', help="file to export")
    parser.add_argument('-t', '--type', help="corrupt for [header, trailer, image, signature, wh, wi, ws, more, less], w is wrapper.")
    args = parser.parse_args()

    if (args.input != None):
        fw_in=args.input
    else:
        exit()

    if (args.type != None):
        corrupt_fild=args.type
    else:
        corrupt_fild = 'header'
    corrupt_fild = corrupt_fild.split(' ')
    
    if (args.output != None):
        fw_out=args.output
    else:
        fw_out=fw_in+'_bad_'+'_'.join(corrupt_fild)

    
    with open(fw_in, 'rb') as file:
        file_len = os.path.getsize(fw_in)
        
        hdr = wrapper_hdr = get_header(file, 0)

        if (IMAGE_MAGIC == hdr.ih_magic):
            hdr = get_header(file, hdr.ih_hdr_size)
            if (IMAGE_MAGIC == hdr.ih_magic):
                print("Image wrapper found. Full size = %d"%(file_len))
            else:
                hdr = wrapper_hdr
                print("Image found (without wrapper). Full size = %d"%(file_len))
        # print("version:", hdr.ih_ver)  
        file.seek(0)
        new_data = file.read()

        tlvInfoProt = get_tlvInfo(file, hdr.ih_hdr_size + hdr.ih_img_size)
        tlvInfo = get_tlvInfo(file, hdr.ih_hdr_size + hdr.ih_img_size + hdr.ih_protect_tlv_size)
        
    with open(fw_out, 'wb') as file:
        if corrupt_fild[0] == "less": # equal signature corrupt
            file.write(new_data[:-2])
        else:
            file.write(new_data)

    if (hdr == wrapper_hdr and "w" in corrupt_fild[0]):
        print("Expected wrapper image inside binary file! Can't do it.")
        exit()

    if (hdr != wrapper_hdr):
        wrapper_off = wrapper_hdr.ih_hdr_size
    else:
        wrapper_off = 0

    change_off = 0
    match corrupt_fild[0]:
        case "header":
            change_off = wrapper_off + get_offset_for_image_header_field(corrupt_fild[1] if len(corrupt_fild) > 1 else "header")
            pass
        case "image":
            change_off = wrapper_off + hdr.ih_hdr_size + randint(0, hdr.ih_img_size-1)
            pass
        case "signature":
            change_off = wrapper_off + hdr.ih_hdr_size + hdr.ih_img_size + hdr.ih_protect_tlv_size + tlvInfo.it_tlv_tot - randint(1, 64)
            pass
        case "trailerInfoProt":
            change_off = wrapper_off + hdr.ih_hdr_size + hdr.ih_img_size + randint(0, 3)
            pass
        case "trailerInfo":
            change_off = wrapper_off + hdr.ih_hdr_size + hdr.ih_img_size + hdr.ih_protect_tlv_size + randint(0, 3)
            pass

        case "wHeader":
            change_off = get_offset_for_image_header_field(corrupt_fild[1] if len(corrupt_fild) > 1 else "header")
            pass
        case "wTrailerInfoProt":
            change_off = wrapper_hdr.ih_hdr_size + wrapper_hdr.ih_img_size + randint(0, 3)
            pass
        case "wTrailerInfo":
            change_off = wrapper_hdr.ih_hdr_size + wrapper_hdr.ih_img_size + wrapper_hdr.ih_protect_tlv_size + randint(0, 3)
            pass
        case "wSignature":
            change_off = file_len - randint(1, 64)
            pass

        case "less":
            exit()    # already done
        case "more":
            change_off = file_len+1 # apply more bytes  # equal nothing to happens
            pass
        case _:
            pass

    with open(fw_out, 'rb+') as file:
        corrupt_byte_file(file, change_off)

    print("result in:", fw_out, end='\n\n')