#ifndef __LIBAPL_H_DEFINED__
#define __LIBAPL_H_DEFINED__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Application Program Interface for GNU APL

The following facilities are provided:

`apl_exec`
~   pass a C string to the interpreter for immediate execution as APL code.
`apl_command`
~   pass an APL command to the command processor and return its output.
`APL`
~   a container class wrapping an APL value, allowing access to its rank, 
    shape and ravel.
`apl_get`
~   construct a read-only object of class `APL` given the name of a 
    variable in the workspace.
`apl_set`
~   change the value of a variable in the workspace to a value wrapped
    by an object of class `APL`.

All the action takes place in a private implementation class. No GNU APL
header is exposed.

Vague details of the GNU APL implementation
-------------------------------------------

Although the implementation is hidden from the API, the programmer
needs to know a little about it.

Only one workspace, simply known as "the workspace", is active at any
time. The workspace may be cleared, named, saved and restored by calls
to `apl_command`.

The workspace contains a collection of symbols of various kinds. Apart 
from `apl_exec` and `apl_command`, which behave as if entered from the 
keyboard in an interactive APL session, this API gives access only to 
APL variables, i.e. symbols associated with values.

A value is a multidimensional array of cells. It has three visible
components: rank, shape and ravel.

The shape is a vector of integers, giving the number of elements along 
each axis of the array. Thus the number of elements in the array is
given by the product of the shape items. The number of shape items is
known as the rank. An empty product is of course equal to 1, so that 
a scalar has rank 0.

The ravel is a vector of cells, accessed in APL by a multi-index but in
the interface by a single index starting at 0. As one progresses along 
the ravel, the multi-index is ordered lexicographically, e.g. in a clear 
workspace, the multi-index of an array of shape `2 3` would take the 
values `1 1`, `1 2`, `1 3`, `2 1`, `2 2`, `2 3`. The index origin in APL 
may be changed by `apl_exec`, but in the interface the ravel is always 
indexed from 0.

A cell can hold any of several kinds of objects:

1. A scalar value, i.e. either a number or a single 16-bit Unicode character.
   The number may be stored internally as a 64-bit integer, a `double`, or
   a `complex<double>`.
2. A smart pointer to a value. This allows nested arrays to be represented.
3. Nothing. This can only appear in objects of type `newAPL` that have
   not been initialized fully. 
4. None of the above, i.e. information not accessible from the API.

Class `APL`
-----------

The user can see rank, shape items and ravel elements. They are 
constructed in four ways:

1.  Extracting a snapshot of a variable from the workspace using 
    `apl_get`. The ravel is extracted "by name". That is to say, 
    changes to the elements of the ravel made by `apl_exec` will be 
    visible in the state. 
2.  Newly created scalars and character vectors.
3.  The copy constructor makes a deep copy. 
4.  Creating a new value of given shape in which all cells contain
    nothing. The user is responsible for filling these cells before 
    calling `apl_set`.

Ravel elements can be changed except in the case of objects created 
by `apl_get`, which returns a constant pointer. Overriding this by
a typecast is strongly deprecated and may have unpredictable side 
effects.

*/

/// LOC is a macro that expands to the source filename and line number as 
/// a C string literal. It should (but does not have to) be used as the
/// 'const char * loc' argument of the functions below. That simplifies
/// the troubleshooting of value-related problems.
///
/// Stringify x.
#define STR(x) #x
/// The current location in the source file.
#define LOC Loc(__FILE__, __LINE__)
/// The line l in file f.
#define Loc(f, l) f ":" STR(l)


/// Pass `line` to the interpreter for immediate execution as APL code.
void apl_exec(const char * line_utf8);

/// Pass `command` to the command processor and return its output.
const char * apl_command(const char* command_utf8);

struct Value;
typedef struct Value * APL_value;

/******************************************************************************
   1. APL value constructor functions. The APL_value returned must be released
      with release_value() below at some point in time (unless it is 0)
 */

/// the current value of a variable, 0 if the variable has no value
APL_value get_var_value(const char * var_name_utf8, const char * loc);  

/// A new integer scalar.
APL_value int_scalar(int64_t val, const char * loc);

/// A new floating point scalar.
APL_value double_scalar(double val, const char * loc);

/// A new complex scalar.
APL_value complex_scalar(double real, double imag, const char * loc);

/// A new character scalar.
APL_value char_scalar(int unicode, const char * loc);

/// A new APL value with given rank and shape. All ravel elements are
/// initialized to integer 0.
APL_value apl_value(int rank, const int64_t * shape, const char * loc);

/// a new scalar with ravel 0
APL_value apl_scalar(const char * loc)
   { return apl_value(0, 0, loc); }

/// a new vector with a ravel of length len and ravel elements are initialized to 0
APL_value apl_vector(int64_t len, const char * loc)
   { return apl_value(1, &len, loc); }

/// a new matrix. all ravel elements are initialized to integer 0
APL_value apl_maxtrix(int64_t rows, int64_t cols, const char * loc)
   { const int64_t sh[] = { rows, cols };   return apl_value(2, sh, loc); }

/// a new 3-dimensional value. all ravel elements are initialized to integer 0
APL_value apl_cube(int64_t blocks, int64_t rows, int64_t cols, const char *loc)
   { const int64_t sh[] = { blocks, rows, cols };
     return apl_value(3, sh, loc); }

/******************************************************************************
   2. APL value destructor function. All non-0 APL_values must be released
      at some point in time (even const ones). release_value(0) is not needed
      but accepted.
 */
void release_value(const APL_value val, const char * loc);

/******************************************************************************
   3. read access to APL values. All ravel indices count from ⎕IO←0.
 */

/// return ⍴⍴val
int get_rank(const APL_value val);

/// return (⍴val)[axis]
int64_t get_axis(const APL_value val, unsigned int axis);

/// return ×/⍴val
uint64_t get_element_count(const APL_value val);

/// return the type of (,val)[idx]
int get_type(const APL_value val, uint64_t idx);

/// return non-0 if val[idx] is a character
inline int is_char(const APL_value val, uint64_t idx)
   { return get_type(val, idx) == 0x02; }

/// return non-0 if val[idx] is an integer
inline int is_int(const APL_value val, uint64_t idx)
   { return get_type(val, idx) == 0x10; }

/// return non-0 if val[idx] is a double
inline int is_double(const APL_value val, uint64_t idx)
   { return get_type(val, idx) == 0x20; }

/// return non-0 if val[idx] is a complex
inline int is_complex(const APL_value val, uint64_t idx)
   { return get_type(val, idx) == 0x40; }

/// return non-0 if val[idx] is a (nested) value
inline int is_value(const APL_value val, uint64_t idx)
   { return get_type(val, idx) == 0x04; }

/// return the character val[idx] (after having checked is_char())
int get_char(const APL_value val, uint64_t idx);

/// return the integer val[idx] (after having checked is_int())
int64_t get_int(const APL_value val, uint64_t idx);

/// return the double val[idx] (after having checked is_double())
double get_real(const APL_value val, uint64_t idx);

/// return the complex val[idx] (after having checked is_complex())
double get_imag(const APL_value val, uint64_t idx);

/// return the (nested) value val[idx] (after having checked is_value()).
/// The APL_value returned must be released with release_value() later on.
///
APL_value get_value(const APL_value val, uint64_t idx);

/******************************************************************************
   4. write access to APL values. All ravel indices count from ⎕IO←0.
 */
/// val[idx]←unicode
void set_char(int unicode, APL_value val, uint64_t idx);

/// val[idx]←new_int
void set_int(int64_t new_double, APL_value val, uint64_t idx);

/// val[idx]←new_double
void set_double(double new_real, double new_imag, APL_value val, uint64_t idx);

/// val[idx]←new_real J new_imag
void set_complex(double new_real, double new_imag, APL_value val, uint64_t idx);

/// val[idx]←new_value
void set_value(APL_value new_value, APL_value val, uint64_t idx);


/******************************************************************************
   5. other
 */

/// var_name_utf8 ← new_value. If new_value is 0 then no assignment is made but
/// var_name_utf8 is checked. Return non-0 on error.
int set_var_value(const char * var_name_utf8, const APL_value new_value,
                  const char * loc);

/// print value
void print_value(APL_value value, FILE * out);

/// UTF8 string to one Unicode. If len is non-0 then the number of characters
/// used in utf is returned in * length.
int UTF8_to_Unicode(const char * utf, int * length);

/// One Unicode to UTF8. Provide at least 7 bytes for dest. dest will be
/// 0-teminated
void Unicode_to_UTF8(int unicode, char * dest, int * length);


#ifdef __cplusplus
}
#endif

#endif // __LIBAPL_H_DEFINED__
