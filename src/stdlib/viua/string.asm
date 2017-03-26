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

; This module contains various string utility functions.
; It is a part of standard Viua runtime library.

.function: std::string::stringify/1
    frame ^[(param %0 (ptr %2 (strstore %1 ""))) (param %1 (arg %3 %0))]
    msg void stringify/2
    move %0 %1
    return
.end

.function: std::string::represent/1
    frame ^[(param %0 (ptr %2 (strstore %1 ""))) (param %1 (arg %3 %0))]
    msg void represent/2
    move %0 %1
    return
.end
