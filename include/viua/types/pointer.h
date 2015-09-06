#ifndef VIUA_TYPES_POINTER_H
#define VIUA_TYPES_POINTER_H

#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <viua/types/type.h>

class Pointer: public Type {
    Type *pointer;

    public:
        virtual std::string type() const;
        virtual std::string str() const;
        virtual std::string repr() const;
        virtual bool boolean() const;

        virtual std::vector<std::string> bases() const;
        virtual std::vector<std::string> inheritancechain() const;

        virtual Pointer* copy() const;
        virtual Type* pointsTo() const;

        Pointer(Type *ptr);
        virtual ~Pointer();
};


#endif
