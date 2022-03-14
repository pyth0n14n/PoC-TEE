# Copyright 2022 Shoei Nashimoto
# SPDX-License-Identifier: MIT/Apache-2.0

import serial
from time import sleep

# -----------------------------------------
#       Constant Variables
# -----------------------------------------
BAUDRATE = 115200
PORT = "COM8" #"/dev/ttyUSB2"  # <- Change here

DEBUG = True

CMD_BYTE = 4
AES_BYTE = 16
AES_BITS = 128
ACK_BYTE = 2
SHARED_CMD_BYTE = 2

ACK_SUCCESS = bytes([0x90, 0x00])

# -----------------------------------------
#       Functions
# -----------------------------------------
def debug_print(msg):
    if DEBUG:
        print("[DEBUG] {0}".format(msg))


class UsbRiscv:
    def __init__(self, port=PORT, baud=BAUDRATE):
        self.ser = serial.Serial(port, baud, timeout=2)  # 2sec
        sleep(0.5)
        print("connected serial")

    def __del__(self):
        self.ser.close()
        del(self.ser)
        print("disconnected serial")

    # ----- general functions -----
    def echo(self):
        cmd = [0x80, 0x10, 0x10, 0x00]
        self.ser.write(bytes(cmd))
        sleep(0.01)

        res = self.ser.read(CMD_BYTE)
        sleep(0.01)
        print("res: {0}, {1}".format(res.hex(), res))
        ack = self.ser.read(ACK_BYTE)
        sleep(0.01)
        print("ack:  {0}, {1}".format(ack.hex(), ack))

        return res == bytes(cmd) and ack == ACK_SUCCESS

    def responce(self):
        cmd = [0x80, 0x10, 0x20, 0x00]
        self.ser.write(bytes(cmd))
        sleep(0.01)

        res = self.ser.read(AES_BYTE)
        sleep(0.01)

        ack = self.ser.read(2)
        if ack != ACK_SUCCESS:
            print("Error in responce(), ack: {0}".format(ack))

        return res #int.from_bytes(res, "big")

    def set_buffer(self, buf):
        cmd = [0x80, 0x10, 0x30, 0x00]

        if len(buf) != AES_BYTE:
            print("Error in set_buffer(), buf: {0}".format(buf))
            return

        self.write(cmd, buf)

    # ----- ecall functions -----
    def set_shared_cmd(self, buf):
        cmd = [0x80, 0x20, 0x08, 0x00]

        if len(buf) != SHARED_CMD_BYTE:
            print("Error in set_shared_cmd(), buf: {0}".format(buf))
            return

        self.write(cmd, buf)

    def set_shared(self, size):
        """size <= AES_BYTE"""
        cmd = [0x80, 0x20, 0x10, size]

        if size <= 0 or AES_BYTE < size:
            print("Error in set_shared(), size: {0}".format(size))
            return

        self.write(cmd)

    def get_shared(self, size):
        """size <= AES_BYTE"""
        cmd = [0x80, 0x20, 0x18, size]

        if size <= 0 or AES_BYTE < size:
            print("Error in set_shared(), size: {0}".format(size))
            return

        self.write(cmd)

    def call_sep2(self):
        cmd = [0x80, 0x20, 0x20, 0x00]
        self.write(cmd)

    def call_sep3(self):
        cmd = [0x80, 0x20, 0x30, 0x00]
        self.write(cmd)

    def sep3_set_base_addr(self):
        return self.set_shared_cmd([0x33, 0x10])

    def sep3_get_addr(self):
        return self.set_shared_cmd([0x33, 0x20])

    def sep3_fault_get_addr(self):
        return self.set_shared_cmd([0x33, 0xf1])

    # ---------- 補助関数 ----------
    def write(self, cmd, data=None):
        self.ser.write(bytes(cmd))
        sleep(0.01)

        if data != None:
            self.ser.write(bytes(data))
            sleep(0.01)

        # Blocking
        ack = self.ser.read(2)
        while ack != ACK_SUCCESS:
            ack = self.ser.read(2)
            debug_print("Error in ack: {0}".format(ack))
            sleep(1)
        return


    def synchronize(self):
        msg = b"risc"
        echo = b""
        while True:
            self.ser.write(msg)
            sleep(0.01)
            echo = self.ser.read(CMD_BYTE)
            debug_print("{0}".format(echo))
            if echo != msg:
                self.ser.write(b"0")  # flush output buffer
                self.flush_input_buffer()
                # self.ser.reset_input_buffer()
                # self.ser.reset_output_buffer()
                debug_print("flush")
            else:
                break
        print("finish synchronization")

    def wait_msg(self, msg):
        buf = ""
        while not(msg in buf):
            c = self.ser.read()
            try:
                c = c.decode("utf-8")
                buf += c
                # debug_print(buf)
            except UnicodeDecodeError:
                debug_print("decode error")

        debug_print(buf)
        return

    def flush_input_buffer(self):
        self.ser.timeout = 0.1
        for i in range(10):
            c = self.ser.read()
            if c == b"":
                break

        self.ser.timeout = 2

    def flush(self):
        self.ser.flush()


# -----------------------------------------
#       Main Rountine
# -----------------------------------------
if __name__ == '__main__':
    usb = UsbRiscv()

    sleep(0.5)
    usb.wait_msg("Start Freedom OS")
    usb.synchronize()

    # ==============
    print("--- Call APP2 ---")
    # AES by APP2
    key = 0x000102030405060708090a0b0c0d0e0f  # defined at main2.c
    pt  = 0x00112233445566778899aabbccddeeff
    # send plain text
    usb.set_buffer(pt.to_bytes(AES_BYTE, "big"))
    usb.set_shared(AES_BYTE)
    # enc
    usb.call_sep2()
    # get ct
    usb.get_shared(AES_BYTE)
    val = usb.responce()
    ct = int.from_bytes(val, 'big')
    print("pt:  0x{0:032x}".format(pt))
    print("key: 0x{0:032x}".format(key))
    print("ct:  0x{0:032x}".format(ct))

    # ==============
    # RAM dump by APP3
    print()
    print("--- Call APP3 ---")

    # change mode
    for fault in [False, True]:
        if fault:
            # Invalid access
            # The monitor fills shared memory by 0xff
            print()
            print("== Invalid Access ==", end="")
            usb.sep3_fault_get_addr()
            base_addr = 0x80003000  # APP2
        else:
            # Valid access
            # APP3 can get its memory contents.
            print("== Valid Access ==", end="")
            usb.sep3_get_addr()
            base_addr = 0x80003800  # APP3

        ofs_addr = 0

        for i in range(16):
            # offset addr
            usb.set_buffer([ofs_addr & 0xff] + [(ofs_addr >> 8) & 0xff] + [0x00] * (AES_BYTE - 2))
            usb.set_shared(2)

            usb.call_sep3()
            usb.get_shared(4)

            val = usb.responce()
            if ofs_addr % 16 == 0:
                print("\n{0:08x}: ".format(base_addr + ofs_addr), end="")

            try:
                print("{0:08x} ".format(int.from_bytes(val, 'little')), end="")
            except IndexError:
                print("Index error: {0}".format(val))

            ofs_addr += 4
        print()
    del(usb)

