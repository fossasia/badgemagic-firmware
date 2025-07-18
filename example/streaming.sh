#!/usr/bin/expect -f

set device "5C:53:10:B8:3D:8F"
set timeout 10

spawn bluetoothctl
expect "Agent registered"

# 1. Scan
# This should be run only once externally,
# as bluetoothctl would remember the last connected device.
# So comment the 2 lines below would speed up process.
send -- "scan on\r"
expect "$device"

# 2. Connect
send -- "connect $device\r"
expect "Connection successful"

# 3. Select attribute (characteristic 0xf057)
send -- "gatt.select-attribute 0000f057-0000-1000-8000-00805f9b34fb\r"

# 4. Enter streaming mode
# (0x02 = streaming_setting, 0x00 = enter streaming mode)
send -- "gatt.write '02 00'\r"
expect "Attempting to write"
sleep 1

# 5. Write bitmap content to the badge
# this will draw 2 first lines on the screen
send -- "gatt.write '03 0xff 0xff 0xff 0xff'\r"
expect "Attempting to write"
sleep 5

# Write an entire screen with increment values
send -- "gatt.write '03 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 \
22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 \
48 49 50 51 52 53 54 55 56 57 58 59 60 61 62 63 64 65 66 67 68 69 70 71 72 73 \
74 75 76 77 78 79 80 81 82 83 84 85 86 87 88'\r"
expect "Attempting to write"
sleep 5

# 6. Exit streaming mode (if needed)
# (0x02 = streaming_setting, 0x01 = leave streaming mode)
send -- "gatt.write '02 01'\r"
expect "Attempting to write"
sleep 1

send -- "exit\r"
expect eof