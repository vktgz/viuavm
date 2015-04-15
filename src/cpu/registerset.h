#ifndef WUDOO_REGISTERSET_H
#define WUDOO_REGISTERSET_H

#pragma once


typedef unsigned char mask_t;

enum REGISTER_MASKS: mask_t {
    REFERENCE       = (1 << 0),
    COPY_ON_WRITE   = (1 << 1),
    KEEP            = (1 << 2),
};


class RegisterSet {
    unsigned registerset_size;
    Object** registers;
    mask_t*  masks;

    public:
        // basic access to registers
        Object* set(unsigned, Object*);
        Object* get(unsigned);
        Object* at(unsigned);

        // register modifications
        void move(unsigned, unsigned);
        void swap(unsigned, unsigned);
        void empty(unsigned);
        void free(unsigned);

        // mask inspection and manipulation
        void flag(unsigned, mask_t);
        void unflag(unsigned, mask_t);
        void clear(unsigned);
        bool isflagged(unsigned, mask_t);
        void setmask(unsigned, mask_t);
        mask_t getmask(unsigned);

        inline unsigned size() { return registerset_size; }

        RegisterSet(unsigned sz);
        ~RegisterSet();
};


#endif
