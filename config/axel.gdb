target remote localhost:1234
symbol-file axel.sym

set disassembly-flavor intel
set history save on
set history size 10000
set history filename .gdb_history

tui enable
layout src
layout regs
tabset 4
refresh
