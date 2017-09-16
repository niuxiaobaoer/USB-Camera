set prompt (arm-gdb)
# This connects to a target via netsiliconLibRemote
# listening for commands on a TCP port on the local machine.
# 2331 if the Segger J-Link GDB Server is being used
# 3333 if OpenOCD is being used
# If OpenOCD is being used, the CPU should be halted
# using the "monitor halt" command.
# Uncomment the appropriate line below:
target remote localhost:2331
# target remote localhost:3333
# monitor halt
monitor speed 1000
monitor endian little
set endian little
monitor reset
# Set the processor to SVC mode
monitor reg cpsr =0xd3
# Disable all interrupts
monitor memU32 0xFFFFF014 =0xFFFFFFFF
# Enable the TCMs
monitor memU32 0x40000000 =0xE3A00015
monitor memU32 0x40000004 =0xEE090F31
monitor memU32 0x40000008 =0xE240024F
monitor memU32 0x4000000C =0xEE090F11
# Change the FX3 SYSCLK setting based on
# input clock frequency. Update with
# correct value from list below.
# Clock input is 19.2 MHz: Value = 0x00080015
# Clock input is 26.0 MHz: Value = 0x00080010
# Clock input is 38.4 MHz: Value = 0x00080115
# Clock input is 52.0 MHz: Value = 0x00080110
monitor memU32 0xE0052000 = 0x00080015
# Add a delay to let the clock stabilize.
monitor sleep 1000
set $pc =0x40000000
si
si
si
si