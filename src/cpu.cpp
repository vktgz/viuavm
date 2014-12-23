#include <iostream>
#include <vector>
#include "bytecode.h"
#include "cpu.h"
#include "types/object.h"
#include "types/integer.h"
#include "types/boolean.h"
#include "types/byte.h"
using namespace std;


typedef char byte;


CPU& CPU::load(char* bc) {
    /*  Load bytecode into the CPU.
     *  CPU becomes owner of loaded bytecode - meaning it will consider itself responsible for proper
     *  destruction of it, so make sure you have a copy of the bytecode.
     *
     *  Any previously loaded bytecode is freed.
     *  To free bytecode without loading anything new it is possible to call .load(0).
     *
     *  :params:
     *
     *  bc:char*    - pointer to byte array containing bytecode with a program to run
     */
    if (bytecode) { delete[] bytecode; }
    bytecode = bc;
    return (*this);
}

CPU& CPU::bytes(uint16_t sz) {
    /*  Set bytecode size, so the CPU can stop execution even if it doesn't reach HALT instruction but reaches
     *  bytecode address out of bounds.
     */
    bytecode_size = sz;
    return (*this);
}


Object* CPU::fetchRegister(int i, bool nullok) {
    /*  Return pointer to object at given register.
     *  This method safeguards against reaching for out-of-bounds registers and
     *  reading from an empty register.
     *
     *  :params:
     *
     *  i:int       - index of a register to fetch
     *  nullok:bool - disables checking if reading from empty register
     */
    if (i >= reg_count) {
        throw "register access index out of bounds";
    }
    Object* optr = registers[i];
    if (!nullok and optr == 0) {
        throw "read from null register";
    }
    return optr;
}

int CPU::run() {
    /*  VM CPU implementation.
     *
     *  A giant switch-in-while which iterates over bytecode and executes encoded instructions.
     */
    if (!bytecode) {
        throw "null bytecode (maybe not loaded?)";
    }
    int return_code = 0;

    int addr = 0;
    bool halt = false;
    bool branched;
    byte* bptr = 0;
    int* iptr = 0;

    while (true) {
        branched = false;
        if (debug) cout << "CPU: bytecode at 0x" << hex << addr << dec << ": ";

        try {
            switch (bytecode[addr]) {
                case ISTORE:
                    if (debug) cout << "ISTORE " << ((int*)(bytecode+addr+1))[0] << " " << ((int*)(bytecode+addr+1))[1] << endl;
                    registers[ ((int*)(bytecode+addr+1))[0] ] = new Integer(((int*)(bytecode+addr+1))[1]);
                    addr += 2 * sizeof(int);
                    break;
                case IADD:
                    if (debug) cout << "IADD " << ((int*)(bytecode+addr+1))[0] << " " << ((int*)(bytecode+addr+1))[1] << " " << ((int*)(bytecode+addr+1))[2] << endl;
                    registers[ ((int*)(bytecode+addr+1))[2] ] = new Integer( static_cast<Integer*>( fetchRegister( ((int*)(bytecode+addr+1))[0] ) )->value() +
                                                                             static_cast<Integer*>( fetchRegister( ((int*)(bytecode+addr+1))[1] ) )->value()
                                                                             );
                    addr += 3 * sizeof(int);
                    break;
                case ISUB:
                    if (debug) cout << "ISUB " << ((int*)(bytecode+addr+1))[0] << " " << ((int*)(bytecode+addr+1))[1] << " " << ((int*)(bytecode+addr+1))[2] << endl;
                    registers[ ((int*)(bytecode+addr+1))[2] ] = new Integer( static_cast<Integer*>( fetchRegister( ((int*)(bytecode+addr+1))[0] ) )->value() -
                                                                             static_cast<Integer*>( fetchRegister( ((int*)(bytecode+addr+1))[1] ) )->value()
                                                                             );
                    addr += 3 * sizeof(int);
                    break;
                case IMUL:
                    if (debug) cout << "IMUL " << ((int*)(bytecode+addr+1))[0] << " " << ((int*)(bytecode+addr+1))[1] << " " << ((int*)(bytecode+addr+1))[2] << endl;
                    registers[ ((int*)(bytecode+addr+1))[2] ] = new Integer( static_cast<Integer*>( fetchRegister( ((int*)(bytecode+addr+1))[0] ) )->value() *
                                                                             static_cast<Integer*>( fetchRegister( ((int*)(bytecode+addr+1))[1] ) )->value()
                                                                             );
                    addr += 3 * sizeof(int);
                    break;
                case IDIV:
                    if (debug) cout << "IDIV " << ((int*)(bytecode+addr+1))[0] << " " << ((int*)(bytecode+addr+1))[1] << " " << ((int*)(bytecode+addr+1))[2] << endl;
                    registers[ ((int*)(bytecode+addr+1))[2] ] = new Integer( static_cast<Integer*>( fetchRegister( ((int*)(bytecode+addr+1))[0] ) )->value() /
                                                                             static_cast<Integer*>( fetchRegister( ((int*)(bytecode+addr+1))[1] ) )->value()
                                                                             );
                    addr += 3 * sizeof(int);
                    break;
                case IINC:
                    if (debug) cout << "IINC " << ((int*)(bytecode+addr+1))[0] << endl;
                    (static_cast<Integer*>( fetchRegister( ((int*)(bytecode+addr+1))[0] ) )->value())++;
                    addr += sizeof(int);
                    break;
                case IDEC:
                    if (debug) cout << "IDEC " << ((int*)(bytecode+addr+1))[0] << endl;
                    (static_cast<Integer*>( fetchRegister( ((int*)(bytecode+addr+1))[0] ) )->value())--;
                    addr += sizeof(int);
                    break;
                case ILT:
                    if (debug) cout << "ILT " << ((int*)(bytecode+addr+1))[0] << " " << ((int*)(bytecode+addr+1))[1] << " " << ((int*)(bytecode+addr+1))[2] << endl;
                    registers[ ((int*)(bytecode+addr+1))[2] ] = new Boolean( static_cast<Integer*>( fetchRegister( ((int*)(bytecode+addr+1))[0] ) )->value() <
                                                                             static_cast<Integer*>( fetchRegister( ((int*)(bytecode+addr+1))[1] ) )->value()
                                                                             );
                    addr += 3 * sizeof(int);
                    break;
                case ILTE:
                    if (debug) cout << "ILTE " << ((int*)(bytecode+addr+1))[0] << " " << ((int*)(bytecode+addr+1))[1] << " " << ((int*)(bytecode+addr+1))[2] << endl;
                    registers[ ((int*)(bytecode+addr+1))[2] ] = new Boolean( static_cast<Integer*>( fetchRegister( ((int*)(bytecode+addr+1))[0] ) )->value() <=
                                                                             static_cast<Integer*>( fetchRegister( ((int*)(bytecode+addr+1))[1] ) )->value()
                                                                             );
                    addr += 3 * sizeof(int);
                    break;
                case IGT:
                    if (debug) cout << "IGT " << ((int*)(bytecode+addr+1))[0] << " " << ((int*)(bytecode+addr+1))[1] << " " << ((int*)(bytecode+addr+1))[2] << endl;
                    registers[ ((int*)(bytecode+addr+1))[2] ] = new Boolean( static_cast<Integer*>( fetchRegister( ((int*)(bytecode+addr+1))[0] ) )->value() >
                                                                             static_cast<Integer*>( fetchRegister( ((int*)(bytecode+addr+1))[1] ) )->value()
                                                                             );
                    addr += 3 * sizeof(int);
                    break;
                case IGTE:
                    if (debug) cout << "IGTE " << ((int*)(bytecode+addr+1))[0] << " " << ((int*)(bytecode+addr+1))[1] << " " << ((int*)(bytecode+addr+1))[2] << endl;
                    registers[ ((int*)(bytecode+addr+1))[2] ] = new Boolean( static_cast<Integer*>( fetchRegister( ((int*)(bytecode+addr+1))[0] ) )->value() >=
                                                                             static_cast<Integer*>( fetchRegister( ((int*)(bytecode+addr+1))[1] ) )->value()
                                                                             );
                    addr += 3 * sizeof(int);
                    break;
                case IEQ:
                    if (debug) cout << "IEQ " << ((int*)(bytecode+addr+1))[0] << " " << ((int*)(bytecode+addr+1))[1] << " " << ((int*)(bytecode+addr+1))[2] << endl;
                    registers[ ((int*)(bytecode+addr+1))[2] ] = new Boolean( static_cast<Integer*>( fetchRegister( ((int*)(bytecode+addr+1))[0] ) )->value() ==
                                                                             static_cast<Integer*>( fetchRegister( ((int*)(bytecode+addr+1))[1] ) )->value()
                                                                             );
                    addr += 3 * sizeof(int);
                    break;
                case BSTORE:
                    if (debug) cout << "BSTORE " << ((int*)(bytecode+addr+1))[0] << " " <<
                        (int)(((bytecode+addr+1+sizeof(int)))[0]) << endl;
                    registers[ ((int*)(bytecode+addr+1))[0] ] = new Byte(*(bytecode+addr+1+sizeof(int)));
                    addr += sizeof(int) + sizeof(char);
                    break;
                case PRINT:
                    if (debug) cout << "PRINT " << ((int*)(bytecode+addr+1))[0] << endl;
                    cout << fetchRegister(*((int*)(bytecode+addr+1)))->str() << endl;
                    addr += sizeof(int);
                    break;
                case ECHO:
                    if (debug) cout << "ECHO " << ((int*)(bytecode+addr+1))[0] << endl;
                    cout << fetchRegister(*((int*)(bytecode+addr+1)))->str();
                    addr += sizeof(int);
                    break;
                case BRANCH:
                    if (debug) cout << "BRANCH 0x" << hex << *(int*)(bytecode+addr+1) << dec << endl;
                    if ((*(int*)(bytecode+addr+1)) == addr) {
                        throw "aborting: BRANCH instruction pointing to itself";
                    }
                    addr = *(int*)(bytecode+addr+1);
                    branched = true;
                    break;
                case BRANCHIF:
                    iptr = (int*)(bytecode+addr+1);
                    if (debug) cout << "BRANCHIF " << *iptr << " " << hex
                         << "0x" << *(iptr+1) << " "
                         << "0x" << *(iptr+2);
                    if (debug) cout << endl;
                    if (debug) cout << static_cast<Boolean*>( fetchRegister( *iptr ) )->str() << endl;
                    if (static_cast<Boolean*>( fetchRegister( *iptr ) )->value()) {
                        addr = *(iptr+1);
                    } else {
                        addr = *(iptr+2);
                    }
                    branched = true;
                    break;
                case RET:
                    if (debug) cout << "RET " << *(int*)(bytecode+addr+1) << endl;
                    if (fetchRegister(*((int*)(bytecode+addr+1)))->type() == "Integer") {
                        registers[0] = new Integer( static_cast<Integer*>( fetchRegister( ((int*)(bytecode+addr+1))[0] ) )->value() );
                    } else {
                        throw ("invalid return value: must be Integer but was: " + fetchRegister(*((int*)(bytecode+addr+1)))->str());
                    }
                    addr += sizeof(int);
                    break;
                case HALT:
                    if (debug) cout << "HALT" << endl;
                    halt = true;
                    break;
                case PASS:
                    if (debug) cout << "PASS" << endl;
                    break;
                default:
                    ostringstream error;
                    error << "unrecognised instruction (bytecode value: " << (int)bytecode[addr] << ")";
                    throw error.str().c_str();
            }
        } catch (const char* &e) {
            return_code = 1;
            cout << (debug ? "\n" : "") <<  "exception: " << e << endl;
            break;
        }

        if (!branched) ++addr;
        if (halt) break;

        if (addr >= bytecode_size and bytecode_size) {
            cout << "CPU: aborting: bytecode address out of bound and no HALT instruction reached" << endl;
            return_code = 1;
            break;
        }
    }

    if (return_code == 0 and registers[0]) {
        // if return code if the default one and
        // return register is not unused
        // copy value of return register as return code
        return_code = static_cast<Integer*>(registers[0])->value();
    }

    return return_code;
}
