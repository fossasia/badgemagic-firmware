target ext :3333
monitor init
monitor reset
monitor halt

add-symbol-file build/mcuboot.elf
add-symbol-file build/badgemagic-ch582.elf
break src/boot-entry.c:HardFault_Handler
# break src/boot-entry.c:start_app
# break src/boot-entry.c:boot

break src/mcuboot.c:110


# break *0x10200

layout asm
layout split

# c
# n
# n
# n
# n
# print rv
# set rv=0
# symbol-file build/badgemagic-ch582.elf
# c
# break app.startup.S:162
# break app.startup.S:184
# break src/main.c:main
# break src/main.c:HardFault_Handler

# c

# jump src/main.c:main