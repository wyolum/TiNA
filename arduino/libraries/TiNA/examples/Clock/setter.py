from __future__ import print_function
from Tkinter import *
from interface import *
connect(getSerialports()[0])

DELAY_MS = 200

def do_set(*args, **kw):
    for i in range(10):
        try:
            while(time.time() % 1 < .99):
                pass
            time_set(time.time() - int(time.timezone))
            break
        except AssertionError:
            print('here')
            time.sleep(1000)

def do_get(*args):
    global current_time

    for i in range(10):
        try:
            start = time_req()
            while(time_req() == start):
                pass
            current_time = time_req()
            kandy_time_l.config(text= "Kandy Time: " + fmt_time(time.gmtime(current_time)))
            pc_time_l.config(text= "   PC Time: " + fmt_time(time.localtime(time.time())))
            break
        except AssertionError:
            time.sleep(.1)

def curry(func, arg):
    def out():
        return func(arg)
    return out

def reconnect(port):
    print(port)
    connect(serialport=port)

tk = Tk()
f = Frame(tk)
Button(f, text="Transfer to Kandy", command=do_set).grid(row=0, column=0)
kandy_time_l = Label(f, width=35, text="Kandy Time:", anchor=W, font=('Courier', 30))
kandy_time_l.grid(row=0, column=1)
# Button(f, text="Transfer from Kandy", command=do_get).grid(row=1, column=0)
pc_time_l = Label(f, width=35, text="PC Time:", anchor=W, font=('Courier', 20))
pc_time_l.grid(row=1, column=1)
f.grid(column=1, row=1)

keypad = Frame(tk)
up_button = Button(keypad, text="Up", command=press_UP)
up_button.grid(row=1, column=2)
left_button = Button(keypad, text="Left", command=press_LEFT)
left_button.grid(row=2, column=1)
ok_button = Button(keypad, text="Ok", command=press_MIDDLE)
ok_button.grid(row=2, column=2)
right_button = Button(keypad, text="Right", command=press_RIGHT)
right_button.grid(row=2, column=3)
down_button = Button(keypad, text="Down", command=press_DOWN)
down_button.grid(row=3, column=2)
Button(keypad, text="Reset", command=press_RESET).grid(row=4, column=4)

keypad.grid(row=2, column=1)


menubar = Menu(tk)
usbmenu = Menu(menubar, tearoff=0)

for sp in getSerialports():
    usbmenu.add_command(label=str(sp), command=curry(reconnect, sp))
menubar.add_cascade(label="USB", menu=usbmenu)
tk.config(menu=menubar)

current_time = time.time() - time.timezone
def tick():
    global current_time
    tk.after(DELAY_MS, tick)
    try:
        current_time = time_req()
    except:
        current_time += DELAY_MS / 1000.
    # kandy_time_l.config(text= "Kandy Time: " + fmt_time(time.gmtime(current_time)))
    if current_time > 4234567890:
        current_time = 4294967296 - current_time
        kandy_time_l.config(text= "Kandy Time:        %02d:%02d:%02d" % (current_time // 3600, (current_time % 3600) / 60, current_time % 60), foreground='red')
    elif current_time < 315662400:
        kandy_time_l.config(text= "Kandy Time:        %02d:%02d:%02d" % (current_time // 3600, (current_time % 3600) / 60, current_time % 60), foreground='green')
    else:
        kandy_time_l.config(text= "Kandy Time: " + fmt_time(time.gmtime(current_time)), foreground='blue')
    pc_time_l.config(text= "   PC Time: " + fmt_time(time.localtime(time.time())))
tk.after(100, tick)
tk.mainloop()
