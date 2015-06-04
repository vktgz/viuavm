#ifndef VIUA_TYPE_CLOSURE_H
#define VIUA_TYPE_CLOSURE_H

#pragma once

#include <string>
#include "../bytecode/bytetypedef.h"
#include "../cpu/registerset.h"
#include "function.h"


class Closure : public Function {
    /** Type representing a closure.
     */
    public:
        Type** arguments;
        bool* argreferences;
        int arguments_size;

        std::string function_name;

        RegisterSet* regset;

        std::string type() const;
        std::string str() const;
        std::string repr() const;

        bool boolean() const;

        Type* copy() const;

        // FIXME: implement real dtor
        Closure();
        ~Closure();
};


#endif
