#include <sstream>
#include <viua/types/pointer.h>
using namespace std;


Type* Pointer::pointsTo() const {
    return pointer;
}

string Pointer::type() const {
    return "Pointer";
}

string Pointer::str() const {
    return type();
}
string Pointer::repr() const {
    return type();
}
bool Pointer::boolean() const {
    return true;
}

vector<string> Pointer::bases() const {
    return vector<string>({});
}
vector<string> Pointer::inheritancechain() const {
    return vector<string>({});
}

Pointer* Pointer::copy() const {
    return new Pointer(pointer);
}

Pointer::Pointer(Type *ptr): pointer(ptr) {}
Pointer::~Pointer() { delete pointer; }
