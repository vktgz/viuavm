; This script tests integer multiplication.

.def: main 0
    istore 1 4
    istore 2 4
    istore 3 -15
    imul 1 2 4
    iadd 4 3 3
    print 3
    end
.end

frame 0
call main
halt
