;
;   Copyright (C) 2017 Marek Marecki
;
;   This file is part of Viua VM.
;
;   Viua VM is free software: you can redistribute it and/or modify
;   it under the terms of the GNU General Public License as published by
;   the Free Software Foundation, either version 3 of the License, or
;   (at your option) any later version.
;
;   Viua VM is distributed in the hope that it will be useful,
;   but WITHOUT ANY WARRANTY; without even the implied warranty of
;   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;   GNU General Public License for more details.
;
;   You should have received a copy of the GNU General Public License
;   along with Viua VM.  If not, see <http://www.gnu.org/licenses/>.
;

.function: global_printer/1
    send (arg %1 %0) (self %2)

    ; switch to global register set
    ress global

    ; wait until a message arrives
    receive %2 10s

    ; print contents of first register
    ; this should throw an exception because this process
    ; did not set any global registers
    print %1

    return
.end

.function: global_writer/2
    ; switch to global register set
    ress global

    ; put second parameter (whatever it is) in
    ; global register 1
    arg %1 %1

    ; switch to local register set
    ress local

    ; send message to printer process to trigger it to
    ; print contents of global register 1
    ; it should cause an exception
    .name: 1 printer_process_handle
    send (arg %printer_process_handle %0) (izero %3)

    return
.end

.signature: std::misc::cycle/1

.function: main/0
    ; spawn printer process
    ; it immediately waits for a message to arrive
    ; first message it receives should crash it
    frame ^[(pamv %0 (self %iota))]
    process void global_printer/1

    .name: %iota printer_pid
    receive %printer_pid local 10s
    print %printer_pid local

    ; spawn two independent writer processes
    ; whichever triggers the printer process is not important
    frame ^[(param %0 %printer_pid) (pamv %1 (string %2 "Hello World"))]
    process void global_writer/2

    ; this is the second writer process
    frame ^[(param %0 %printer_pid) (pamv %1 (string %2 "broken"))]
    process void global_writer/2

    izero %0 local
    return
.end
