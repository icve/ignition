

# TODO set date

import socket
import datetime
from time import sleep

IP_ADDR = '192.168.4.1'
PORT = 6666
IS_12_HOUR = False
"""
prefix command with w

code from tcp parser
// write to rtc register
            //format: AVV (address in hex, value in hex)
"""

current_time = datetime.datetime.now()


def get_hour_value_str(current_time):
    hour_str = current_time.strftime("%H")
    upper = (IS_12_HOUR << 2) | int(hour_str[0])
    return hex(upper).replace('0x', "").upper() + hour_str[1]

addr_val_list = [
    ("0", current_time.strftime("%S")),
    ("1", current_time.strftime("%M")),
    ("2", get_hour_value_str(current_time))
    # (0, current_time.strftime("%M")),
    # (0, current_time.strftime("%M")),
    # (0, current_time.strftime("%M")),
]


msgs = []
for addr, val in addr_val_list:
    msg = "w" + addr + val
    msgs.append(msg)
    print("will be sending:", msg)

print("connection to server")
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect((IP_ADDR, PORT))
for m in msgs:
    sleep(0.3)
    s.send((m+'\n').encode())
    print(m, "sent")

print("checking time in rtc")
s.send(b't\n')
print(s.recv(100))

# second
# cmd_list.append("S0"+  current_time.strftime("%"))