from __future__ import print_function
import sys
import glob
import serial
import time
import datetime
import struct
import os

SER_TIMEOUT = .1 # Serial timeout, seconds

def set_gmt_offset(offset):
    global gmt_offset
    gmt_offset = offset

def getSerialports():
    if sys.platform.startswith('win'): # windows
        out = []
        # print 'HARD CODED HACK ON NEXT LINE'
        return ['COM6'] 
        import scanwin32
        for order, port, desc, hwid in sorted(scanwin32.comports()):
            print( "%-10s: %s (%s) ->" % (port, desc, hwid),)
            try:
                s = serial.Serial(port) # test open
                s.close()
            except serial.serialutil.SerialException:
                print( "can't be opened")
            else:
                print("Ready")
                out.append(port)
    elif sys.platform.startswith('darwin'): # mac
        out = glob.glob('/dev/tty.usb*')
        out.sort()
    else: # assume linux 
        out = glob.glob('/dev/ttyUSB*')
        out.sort()
    return out

def connect(serialport='/dev/ttyUSB0', _gmt_offset=None):
    if _gmt_offset is None:
        local_time = time.localtime()
        _gmt_offset = (local_time.tm_isdst * 3600 - time.timezone)
    global ser
    set_gmt_offset(_gmt_offset)

    try:
        ser.close() # need to close serial port on windows.
    except:
        pass
        
    # raw_input('...')
    print( 'serialport', serialport)
    ser = serial.Serial(serialport,
                        baudrate=112500,
                        timeout=SER_TIMEOUT)
    return ser
        

gmt_offset = -7 * 3600
def flush():
    dat = ser.read(1000)
    while len(dat) > 0:
        dat = ser.read(1000)

ABS_TIME_REQ = 1
ABS_TIME_SET = 2
TRIGGER_BUTTON = 3
PING = 4
N_MSG_TYPE = 5
BUTTON_NONE = 0;
BUTTON_UP = 1;
BUTTON_DOWN = 2;
BUTTON_MIDDLE = 3;
BUTTON_LEFT = 4;
BUTTON_RIGHT = 5;

def time_req():
    flush()
    ser.write(chr((ABS_TIME_REQ)))
    id = ser.read(1)
    assert id == chr(ABS_TIME_SET), "got wrong response '%s'" % id
    dat = ser.read(4)
    if len(dat) < 4:
        out = 0
    else:
        out = c3_to_wall_clock(dat)
    return out

def time_set(now=None):
    flush()
    if now is None:
        now = int(round(time.time()) + gmt_offset)
    # now = time.mktime(time.localtime())
    dat = wall_clock_to_c3(now)
    ser.write(chr(ABS_TIME_SET))
    ser.write(dat)

def press_button(button_val):
    ser.write("%s%s" %(chr(TRIGGER_BUTTON), chr(button_val)))
    time.sleep(.1)

def press_RESET():
    '''
    reset the ATMEGA. WARNING! start time will be lost
    '''
    ser.setDTR(True); time.sleep(.1); ser.setDTR(False)

def press_LEFT():
    press_button(BUTTON_LEFT)
def press_RIGHT():
    press_button(BUTTON_RIGHT)
def press_UP():
    press_button(BUTTON_UP)
def press_DOWN():
    press_button(BUTTON_DOWN)
def press_MIDDLE():
    press_button(BUTTON_MIDDLE)
    
def c3_to_wall_clock(bytes):
    return struct.unpack('<I', bytes)[0]

def wall_clock_to_c3(t):
    return struct.pack('<I', int(round(t)))

def to_gmt(t):
    return t + gmt_offset

def from_gmt(t):
    return t - gmt_offset

def fmt_time(when):
    return '%02d/%02d/%04d %d:%02d:%02d' % (when.tm_mday, when.tm_mon, when.tm_year,
                                            when.tm_hour, when.tm_min, when.tm_sec)

def scroll_msg(msg):
    msg_len = len(msg) + 2
    out_msg = str(SERIAL_SCROLL) + struct.pack('B', msg_len) + msg
    assert len(out_msg) == msg_len
    ser.write(out_msg)
    print('OUT: "%s"' % out_msg)

def main():
    ser.flush()
    now = time_req()

    now = time.gmtime(time_req())
    year = now.tm_year
    print('year', year)

    time_set()
    print (time_req())

if __name__ == '__main__':
    connect(getSerialports()[0])
    if len(sys.argv) > 1:
        for arg in sys.argv[1:]:
            if arg == 'set':
                time_set()
                print ('      Kandinsky time',  fmt_time(time.gmtime(time_req())))
            elif arg == 'time':
                print ('      Kandinsky time',  fmt_time(time.gmtime(time_req())))
            elif arg == 'pc_time':
                print ('     PC TIME:', fmt_time(time.gmtime(to_gmt(time.time()))))
            else:
                print ('huh?', arg)
        else:
            # read_write_test()
            main()
