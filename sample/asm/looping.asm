;
;   Copyright (C) 2015, 2016 Marek Marecki
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

; This script is just a simple loop.
; It displays numbers from 0 to 10.

.function: main/1
    istore 1 0
    istore 2 10

    ; mark the beginning of the loop
    .mark: loop
    lt int64 3 1 2
    ; invert value to use short form of branch instruction, i.e.: branch <cond> <true>
    ; and expliot the fact that it will default false to "next instruction"
    not 3
    if 3 final_print
    print 1
    iinc 1
    jump loop
    .mark: final_print
    print 1
    izero 0
    return
.end
