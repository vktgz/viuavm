;
;   Copyright (C) 2015, 2016, 2017 Marek Marecki
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

.function: greetings/1
    echo (string %1 "Hello ")
    echo (arg %1 %0)
    print (string %1 '!')
    return
.end

.signature: std::functional::apply/2

.function: main/1
    import "std::functional"

    function %1 greetings/1

    frame ^[(param %0 %1) (param %1 (string %2 "World"))]
    call std::functional::apply/2

    frame ^[(param %0 %1) (param %1 (string %2 "Joe"))]
    call std::functional::apply/2

    frame ^[(param %0 %1) (param %1 (string %2 "Mike"))]
    call std::functional::apply/2

    izero %0 local
    return
.end
