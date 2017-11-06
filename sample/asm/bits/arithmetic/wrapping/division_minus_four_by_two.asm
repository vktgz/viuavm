;
;   Copyright (C) 2017 Marek Marecki <marekjm@ozro.pw>
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

.function: main/0
    bits (.name: %iota dividend) local 0b00000100
    bits (.name: %iota divisor) local  0b00000010

    ; make -4 from 4
    bitnot %dividend local %dividend local
    wrapincrement %dividend local

    wrapdiv (.name: %iota wrap_quotinent) local %dividend local %divisor local

    print %dividend local
    print %divisor local
    print %wrap_quotinent local

    izero %0 local
    return
.end
