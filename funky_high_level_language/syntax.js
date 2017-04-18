// immutable value of primitive type
let answer -> int = 42;

// mutable value of primitive type
let i -> mutable int = 0;

// value of templated type
let binary_digits -> vector<int> = { 0, 1, };   // initialisation from initialiser-list
let binary_digits -> vector<int> = [ 0, 1, ];   // initialisation from array literal

// modifiers are read backwards
// pointers point to values, not addresses
T*      // pointer to T
T&      // reference to T
T**     // pointer to pointer to T
T&&     // illegal
T*&     // illegal, pointer to a reference does not make sense
T*[]    // array of pointers to T
T[]*    // pointer to an array of T, makes sense because we have value semantics for arrays
T**[]   // array of pointers to pointer to T
T[][]*  // pointer to an array of arrays of T
T[]*[]  // array of pointers to arrays of T

T#      // address of a value of type T, a pointer in the C sense, MAYBE!!!

mutable T*  // pointer to mutable T: may modify the value, may not modify the pointer
T mutable*  // mutable pointer to T: may not modify the value, may modify the pointer

mutable vector<int> // mutable vector of mutable ints: may modify the vector, may modify the individual elements
mutable vector<mutable int> // same as above
mutable vector<const int>   // mutable vector of immutable ints
vector<mutable int> // immutable vector of mutable ints: may modify individual elements, may not modify the vector (e.g. push or pop elements)

// Full syntax for value declarations:
//
//      LetExpression = 'let' Name ( '->' TypeSpecifier )? ('=' Expression)?
//
//      Name = /[A-Za-z_][A-Za-z0-9_]*/
//
//      TypeSpecifier = ('const' = default | 'mutable') Name TemplateConcretisation TypeModifier*
//
//      TemplateConcretisation = '<' ( TemplateConcPart ( ',' TemplateConcPart )* ) '>'
//
//      TemplateConcPart = ( 'typename' Name ) | LetExpression
//
//      TypeModifier = '[]' | '*' | '&'
//

// array of pointers to vectors of arrays of int
vector<int[]>*[]

// mutable pointer to array of vector of mutable pointers to int
mutable vector<mutable int*>[]*

// how to do templates?
template < typename T > class vector {
    let array -> mutable! T[]*;

    function at (size_type) -> $mutable T $mutable &;
};

// explicit specialisation
template of class vector<int> {
};

template < typename T > function max (let lhs -> T, let rhs -> T) -> T = {
    return (lhs > rhs) ? lhs : rhs ;
};

template < typename T > function max (let seq -> vector<T>) -> T = {
    // case is an expression so has a "return value"
    return case seq of {
        // of-expressions can be compiled if functions with good signatures are available to the compiler
        // note that some such functions can be automatically generated, or
        // are provided as a standard.
        //
        // matching signatures are displayed on the right, like this:
        //  <expression> = { // <signature required for compilation>
        //
        // case operators receive const pointers to mutable values, in order to:
        //  - prevent unnecessary copying, thus the pointer
        //  - prevent deletion by malevolent code, this the const pointer
        //  - allow mutation of the value, thus pointer to mutable
        //
        // > Why const pointer prevents deletion?
        // > Because if `delete ptr` was used, then the pointer no longer *usefully* points to the same value, and
        // > so it appears that its contents have changed.
        //
        // expressions are tried at runtime, in order.
        // case operators must return (boolean, continuation).
        // the boolean indicates if the pattern matches, the function is called if the predicate was true and
        // must return the deconstructed values.
        //
        // if none of such functions return true, an exception is thrown
        let value -> T, let rest -> vector<T> = {   // [1] case operator (vector<int>*) -> tuple<boolean, function (mutable vector<T> const *) -> tuple<T, vector<T>>>
            max(value, rest);
        },
        [ value | ...rest ] = { // destructuring non-empty arrays, same as [1]
        },
        { value = foo } = {     // destructuring objects: { <bind-as-name> = <key-from-object> }
        },
        let value -> T = {                        // [2] case operator (vector<int>*) -> tuple<boolean, function (mutable vector<T> const *) -> T>
            value;
        }
    };
};

// implements [1] from above
template <typename T> case operator (let seq -> vector<T>*) -> tuple<boolean, function (mutable vector<T> const *) -> tuple<T, vector<T>>> = {
    /*
     * type inference is used for functions inside
     */
    if seq.empty() {
        return { false, function (let seq) = { return {}; }, };
    }
    return { true, function (let seq) = {
        let first = seq.pop_front();
        return { first, seq, };
    } };
};


// "this" keyword in operator definition matches the subject of the operator
// examples:
//      'operator this ++' matches 'i++'
//      'operator ++ this' matches '++i'
//
// in case of conflict, operator with greater arity wins.
// example:
//
//      let n -> int = 0;
//      let v -> vector<int>;
//
//      // prints '[0]'
//      // vector's `operator ++ this` wins over int's `operator this ++`
//      print(n ++ v);
//
// it is a compilation failure when the compiler is unable to resolve the precedence conflict.
//
// user code may define only unary or binary operators (arity of the operator is defined by arity of the implementing function)
template < typename T > operator ++ this (this let v -> vector<T>, let element -> T) -> vector<T> = {
    /*
     * This operator allows writing `T ++ vector<T>` instead of `vector<T>.prepend(T)`.
     */
    return v.prepend(element);
};
