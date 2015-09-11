.function: pass_by_pointer
    empty (print (deptr 1 (arg 1 0)))
    end
.end

.function: main
    new 1 Object
    print 1

    frame ^[(param 0 (ptr 2 1))]
    call pass_by_pointer

    frame ^[(paptr 0 1)]
    call pass_by_pointer

    izero 0
    end
.end
