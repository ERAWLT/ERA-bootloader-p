import argparse
import os
import hashlib
import serial.tools.list_ports
import tqdm
from math import ceil, floor

fw_size = 0
fw_hash = ''
start_address = 0
com_port = None
logf = None

ACK = b'\x79'
NACK = b'\x1F'


FLASH_SECTOR_SIZE = (128 * 1024)
# Actually, there are no pages in MCU FLASH. Program/Read operations can be done by smaller chunks.
# This macro is introduced just for simplifying protocol implementation.
FLASH_PAGE_SIZE = 256
LOG_FILE_NAME = 'flasher.log'
GET_TIMEOUT = 1.0
ERASE_TIMEOUT = 30.0
READ_MEMORY_TIMEOUT = 1.0
WRITE_MEMORY_TIMEOUT = 30.0
GO_TIMEOUT = 10.0
ACK_TIMEOUT = 10.0

MAX_FW_SIZE = 128 * 1024 * 14  # 0 sector blr core, 1 blr meta. 1792 kB

BLR_PROTOCOL_VERSION = b'\x31'


def wait_ack(com, timeout):
    com.timeout = timeout
    ack = com.read(1)

    if ack != ACK:
        raise TypeError(f"No ACK received: {ack.hex()}!")

    log_bytes(1, ack)


def add_xor(data):
    xor = 0

    for byte in data:
        xor ^= byte

    data += xor.to_bytes(1, 'big')
    return data


def cmd_get(com):
    log_str('\n-------- GET')
    log_bytes(0, b'\x00\xFF')

    com.write(b'\x00\xFF')
    wait_ack(com, ACK_TIMEOUT)

    com.timeout = GET_TIMEOUT
    size = com.read(1)

    log_bytes(1, size)
    commands = com.read(int.from_bytes(size, byteorder='big') + 1)
    wait_ack(com, ACK_TIMEOUT)

    bl_version = commands[0:1]
    log_bytes(1, commands)
    if bl_version != BLR_PROTOCOL_VERSION:
        raise TypeError(f'Invalid Bootloader version: {commands[0:1]}!')

    commands = commands[1:]

    if b'\x00' not in commands:
        raise TypeError(f'No Get command supported!')
    if b'\x11' not in commands:
        raise TypeError(f'No Read Memory command supported!')
    if b'\x31' not in commands:
        raise TypeError(f'No Write Memory command supported!')
    if b'\x43' not in commands:
        raise TypeError(f'No Erase command supported!')
    if b'\x21' not in commands:
        raise TypeError(f'No Go command supported!')
    if b'\xC0' not in commands:
        raise TypeError(f'No Done command supported!')


def cmd_erase(com, eall):
    log_str('\n-------- ERASE')
    log_bytes(0, b'\x43\xBC')

    com.write(b'\x43\xBC')
    wait_ack(com, ACK_TIMEOUT)

    if eall is True:
        log_bytes(0, b'\xFF\x00')
        com.write(b'\xFF\x00')
    else:
        # Count sectors and add value to data array
        first = 2 + floor((start_address - 0x08040000) / FLASH_SECTOR_SIZE)
        last = 2 + floor((start_address - 0x08040000 + fw_size - 1) / FLASH_SECTOR_SIZE)
        num = last - first + 1

        data = num.to_bytes(1, 'big')

        # Add sectors to data array
        for x in range(num):
            data += (2 + x).to_bytes(1, 'big')

        data = add_xor(data)
        log_bytes(0, data)
        com.write(data)

    wait_ack(com, ERASE_TIMEOUT)


def cmd_write_memory(com, addr, num, data):
    log_str('\n-------- WRITE_MEMORY')
    log_bytes(0, b'\x31\xCE')

    com.write(b'\x31\xCE')
    wait_ack(com, ACK_TIMEOUT)

    adr = addr.to_bytes(4, byteorder='big')
    adr = add_xor(adr)
    log_bytes(0, adr)

    com.write(adr)
    wait_ack(com, ACK_TIMEOUT)

    data = (num - 1).to_bytes(1, 'big') + data
    data = add_xor(data)
    log_bytes(0, data)

    com.write(data)
    wait_ack(com, WRITE_MEMORY_TIMEOUT)


def cmd_read_memory(com, addr, num):
    log_str('\n-------- READ_MEMORY')
    log_bytes(0, b'\x11\xEE')

    com.write(b'\x11\xEE')
    wait_ack(com, ACK_TIMEOUT)

    adr = addr.to_bytes(4, byteorder='big')
    adr = add_xor(adr)
    log_bytes(0, adr)
    com.write(adr)
    wait_ack(com, ACK_TIMEOUT)

    v = (num - 1).to_bytes(1, 'big')
    v += ((num - 1) ^ 0xFF).to_bytes(1, 'big')

    com.write(v)
    log_bytes(0, v)
    wait_ack(com, ACK_TIMEOUT)

    com.timeout = READ_MEMORY_TIMEOUT

    data = com.read(num)
    log_bytes(0, data)
    if len(data) != num:
        raise TypeError(f'Invalid read data length: {len(data)}!')

    return data


def cmd_go(com):
    log_str('\n-------- GO')
    log_bytes(0, b'\x21\xDE')

    com.write(b'\x21\xDE')
    wait_ack(com, ACK_TIMEOUT)

    data = start_address.to_bytes(4, 'big')
    data = add_xor(data)
    log_bytes(0, data)
    com.write(data)
    wait_ack(com, GO_TIMEOUT)


def cmd_done(com):
    log_str('\n-------- GO')
    log_bytes(0, b'\xC0\x3F')

    com.write(b'\xC0\x3F')
    wait_ack(com, ACK_TIMEOUT)

    data = start_address.to_bytes(4, 'big')
    data += fw_size.to_bytes(4, 'big')
    data += bytearray.fromhex(fw_hash)
    data = add_xor(data)
    log_bytes(0, data)
    com.write(data)
    wait_ack(com, GO_TIMEOUT)


def get_file_sha1(file):
    sha1 = hashlib.sha1()

    with open(file, 'rb') as f:
        sha1.update(f.read())

    return sha1.hexdigest()


def open_com():
    com = serial.Serial(port=com_port, baudrate=115200, parity=serial.PARITY_EVEN, stopbits=serial.STOPBITS_ONE,
                        bytesize=serial.EIGHTBITS)
    if com.isOpen() is False:
        raise TypeError('COM-port open error!')

    com.write(b'\x7F')
    wait_ack(com, ACK_TIMEOUT)

    return com


def flash(file, start_addr, eall, verify, port, log):
    global fw_size, fw_hash, start_address, com_port, logf

    fw_size = os.stat(file).st_size
    fw_hash = get_file_sha1(file)
    start_address = int(start_addr, 16)
    com_port = port

    if log:
        logf = open(LOG_FILE_NAME, 'w+')

    print('\r')
    print(f'сom={com_port}')
    print(f'file={file} size={fw_size} sha1={fw_hash}')
    print(f'flash_start_addr={hex(int(start_addr, 16))}')
    print('\r')

    # Get, Erase, n-Write, n-Read (optional) + Done.
    n_ops = 1 + 1 + ceil(fw_size / FLASH_PAGE_SIZE) + \
            (ceil(fw_size / FLASH_PAGE_SIZE) if verify is True else 0) + 1

    com = open_com()

    log_str('\n\nFlashing ...\n\n')

    print('Flashing ...')

    pbar = tqdm.tqdm(range(n_ops))

    cmd_get(com)
    pbar.update(1)
    cmd_erase(com, eall)
    pbar.update(1)

    with open(file, 'rb') as f:
        size = fw_size
        addr = start_address

        while size > 0:
            num = 256 if size >= 256 else size

            data = f.read(num)

            while len(data) % 4 > 0:
                data += 0xFF.to_bytes(1, 'big')
                num += 1

            cmd_write_memory(com, addr, num, data)

            size -= num
            addr += num
            pbar.update(1)

    if verify is True:
        with open(file, 'rb') as f:
            size = fw_size
            addr = start_address

            while size > 0:
                num = 256 if size >= 256 else size

                data = f.read(num)

                while len(data) % 4 > 0:
                    data += 0xFF.to_bytes(1, 'big')
                    num += 1

                read = cmd_read_memory(com, addr, num)

                if data != read:
                    raise TypeError(f'Read data miscompare detected!')

                size -= num
                addr += num
                pbar.update(1)

    cmd_done(com)
    pbar.update(1)

    log_str('\n\nFlashing DONE')

    print('\nFlashing DONE')

    com.close()


def log_str(s):
    if logf:
        logf.write(s)


def log_bytes(dir: object, bytes: object) -> object:
    if logf:
        logf.write(f"\n{'>' if dir == 0 else '<'} ")
        for v in bytes:
            logf.write(f'{"%0.2X" % v} ')


def list_ports():
    ports = serial.tools.list_ports.comports()
    print(f'Total ports found: {len(ports)}')
    for port, desc, hwid in sorted(ports):
        print(f'{port}: {desc} [{hwid}]')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='FW Flasher for Simplified Bootloader.')

    parser.add_argument('-f', '--file', help='an input file (.bin only).')
    parser.add_argument('-sa', '--start_addr', default="0x08040000",
                        help='flash start address in hex, default/min. 0x08040000.')
    parser.add_argument('-lp', '--list_ports', action='store_true', help='list of COM-ports available.')
    parser.add_argument('-p', '--port', help='COM-port.')
    parser.add_argument('-eall', '--erase_all', action='store_true', help='Erase all FLASH except bootloader area.')
    parser.add_argument('-v', '--verify', action='store_true', help='Verify FW written properly by read it back'
                                                                    'before sending Done command.')
    parser.add_argument('-l', '--log', action='store_true', help='Enable logging.')

    args = parser.parse_args()

    if args.list_ports is True:
        list_ports()
    else:
        if int(args.start_addr, 16) < 0x08040000:
            raise TypeError('Invalid start address!')
        if os.stat(args.file).st_size > MAX_FW_SIZE:
            raise TypeError(f'FW size too big: {os.stat(args.file).st_size} > {MAX_FW_SIZE}')

        flash(args.file, args.start_addr, args.erase_all, args.verify, args.port, args.log)
