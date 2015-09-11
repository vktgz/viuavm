.signature: std::string::representPtr

.function: main
    ; this instruction links "std::string" module into running machine
    link std::string

    ; create new object to test stringification
    new 1 Object

    ; obtain string representation of the object
    ; note the pass-by-pointer used to avoid copying since
    ; we want to get string representation of exactly the same object
    ptr 2 1
    frame ^[(param 0 2)]
    call 3 std::string::representPtr

    ; this should print two, exactly same lines
    print 1
    print 3

    izero 0
    end
.end
