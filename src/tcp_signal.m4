/*
   This file is part of GNU APL, a free implementation of the
   ISO/IEC Standard 13751, "Programming Language APL, Extended"
 
   Copyright (C) 2008-2014  Dr. Jürgen Sauermann
 
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
 
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
 
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


/********

Usage:

    m4 -D protocol=def_file < tcp_signal.m4 > $@

This reads a protocol specification from file def_file.def and prints
a header file on stdout that defines (the serialization of) signals according
to def_file.def.

In a Makefile, you would use it, for example, like this:

    my_signal.hh:  tcp_signal.m4 my_signal.def
            m4 -D protocol=tcp_signal $< > $@

to produce my_signal.hh from my_signal.def. After that, my_signal.hh
can be used to send the signals defined in tcp_signal.def from one process
to another process.

That is:

                      my_signal.def
                            |
                            |
                            | tcp_signal.m4
                            |
                            V
                      my_signal.hh
                            |
                            |
                            |
                            |
                            V
                         my_client_program.cc
                         my_server_program.cc
                                   |
                                   |
                                   |    compile
                                   |    & link
                                   V
                                   my_client_program
                                   my_server_program


and then:

    my_client_program  ---  signals defined in ---> my_server_program
    my_server_program  ---  in my_signal.def   ---> my_client_program

********/

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

#include <string>
#include <iostream>
#include <iomanip>

using namespace std;

//-----------------------------------------------------------------------------
/// an integer signal item of size \b bytes
template<typename T, int bytes>
class Sig_item_int
{
public:
   /// construct an item with value \b v
   Sig_item_int(T v) : value(v & get_mask()) {}

   /// construct (deserialize) this item from a (received) buffer
   Sig_item_int(const uint8_t * & buffer)
      {
        value = 0;
        for (int b = 0; b < bytes; ++b)
            value = value << 8 | *buffer++;

        if (bytes == 6 && (value & 0x000080000000000ULL))
           value |= 0xFFFF000000000000ULL;
      }

   /// return the value of the item
   T get_value() const   { return value; }

   /// store (aka. serialize) this item into a string
   void store(string & buffer) const
      {
        for (int b = bytes; b > 0;)   buffer += char(value >> (8*--b));
      }

   /// print the item
   ostream & print(ostream & out) const
      {
        return out << value;
      }

   /// return the mask of the item, e.g. 0x00FFFFFF for a 3-byte integer
   static uint64_t get_mask()
      { uint64_t mask = 0;
        for (int b = 0; b < bytes; ++b)   mask = mask << 8 | 0x00FF;
        return mask;
      }

   /// the value of the item
   T value;
};
//-----------------------------------------------------------------------------
/// a hexadecimal integer signal item of size \b bytes
template<typename T, int bytes>
class Sig_item_xint : public Sig_item_int<T, bytes>
{
public:
   /// construct an item with value \b v
   Sig_item_xint(T v) : Sig_item_int<T, bytes>(v) {}

   /// construct (deserialize) this item from a (received) buffer
   Sig_item_xint(const uint8_t * & buffer) : Sig_item_int<T, bytes>(buffer) {}

   /// print the item
   ostream & print(ostream & out) const
      {
        return out << "0x" << hex << setfill('0') << setw(bytes)
                   << Sig_item_int<T, bytes>::value
                   << setfill(' ') << dec;
      }
};
//-----------------------------------------------------------------------------
typedef Sig_item_int < int16_t, 1> Sig_item_i8;   ///<   8-bit signed integer
typedef Sig_item_int <uint16_t, 1> Sig_item_u8;   ///<   8-bit unsigned integer
typedef Sig_item_xint<uint16_t, 1> Sig_item_x8;   ///<   8-bit hex integer
typedef Sig_item_int < int16_t, 2> Sig_item_i16;   ///< 16-bit signed integer
typedef Sig_item_int <uint16_t, 2> Sig_item_u16;   ///< 16-bit unsigned integer
typedef Sig_item_xint<uint16_t, 2> Sig_item_x16;   ///< 16-bit hex integer
typedef Sig_item_int < int32_t, 3> Sig_item_i24;   ///< 24-bit signed integer
typedef Sig_item_int <uint32_t, 3> Sig_item_u24;   ///< 24-bit unsigned integer
typedef Sig_item_xint<uint32_t, 3> Sig_item_x24;   ///< 24-bit hex integer
typedef Sig_item_int < int32_t, 4> Sig_item_i32;   ///< 32-bit signed integer
typedef Sig_item_int <uint32_t, 4> Sig_item_u32;   ///< 32-bit unsigned integer
typedef Sig_item_xint<uint32_t, 4> Sig_item_x32;   ///< 32-bit hex integer
typedef Sig_item_int < int64_t, 6> Sig_item_i48;   ///< 48-bit signed integer
typedef Sig_item_int <uint64_t, 6> Sig_item_u48;   ///< 48-bit unsigned integer
typedef Sig_item_xint<uint64_t, 6> Sig_item_x48;   ///< 48-bit hex integer
typedef Sig_item_int < int64_t, 8> Sig_item_i64;   ///< 64-bit signed integer
typedef Sig_item_int <uint64_t, 8> Sig_item_u64;   ///< 64-bit unsigned integer
typedef Sig_item_xint<uint64_t, 8> Sig_item_x64;   ///< 64-bit hex integer

/// a string signal item of size \b bytes
class Sig_item_string
{
public:
   /// construct an item with value \b v
   Sig_item_string(const string & str)
   : value(str)
   {}

   /// construct (deserialize) this item from a (received) buffer
   Sig_item_string(const uint8_t * & buffer)
      {
        Sig_item_u16 len (buffer);
        value.reserve(len.get_value() + 2);
        for (int b = 0; b < len.get_value(); ++b)   value += *buffer++;
      }

   /// return the value of the item
   const string get_value() const   { return value; }

   /// store (aka. serialize) this item into a buffer
   void store(string & buffer) const
      {
        const Sig_item_u16 len (value.size());
        len.store(buffer);
        for (int b = 0; b < value.size(); ++b)   buffer += value[b];
      }

   /// print the item
   ostream & print(ostream & out) const
      {
        bool printable = true;
        for (int b = 0; b < value.size(); ++b)
            {
              const int v = value[b] & 0xFF;
              if (v < ' ' || v > '~')   { printable = false;   break; }
            }

        if (printable)   return out << "\"" << value << "\"";

        out << hex << setfill('0');
        for (int b = 0; b < value.size(); ++b)
            {
              if (b > 16)   { out << "...";   break; }
              out << " " << setw(2) << (value[b] & 0xFF);
            }
        return out << dec << setfill(' ');
      }

protected:
   /// the value of the item
   string value;
};
//-----------------------------------------------------------------------------
divert(`-1')

define(`shift2', `shift(shift($@))')
define(`shift4', `shift2(shift2($@))')

# expa(macro, separator, class, typ1, name1, typ2, name2, ...
define(`expa', `ifelse(`$#', `3', `',
               `ifelse(`$#', `5', `$1($3, $4, $5)',
               `$1($3, $4, $5)$2`'expa(`$1', `$2', `$3',shift(shift4($@)))')')')
define(`typtrans', `ifelse(`$1', `x64',    `uint64_t',
                    ifelse(`$1', `u64',    `uint64_t',
                    ifelse(`$1', `i64',     `int64_t',
                    ifelse(`$1', `x48',    `uint64_t',
                    ifelse(`$1', `u48',    `uint64_t',
                    ifelse(`$1', `i48',     `int64_t',
                    ifelse(`$1', `x32',    `uint32_t',
                    ifelse(`$1', `u32',    `uint32_t',
                    ifelse(`$1', `i32',     `int32_t',
                    ifelse(`$1', `x24',    `uint32_t',
                    ifelse(`$1', `u24',    `uint32_t',
                    ifelse(`$1', `i24',     `int32_t',
                    ifelse(`$1', `x16',    `uint16_t',
                    ifelse(`$1', `u16',    `uint16_t',
                    ifelse(`$1', `i16',     `int16_t',
                    ifelse(`$1', `x8',      `uint8_t',
                    ifelse(`$1', `u8',      `uint8_t',
                    ifelse(`$1', `i8',       `int8_t',
                                             `string'))))))))))))))))))')
define(`sig_args', `,
                Sig_item_`'$2 _`'$3')
define(`sig_init', `$3(`_'$3)')
define(`sig_get', `   /// get $3
   const typtrans($2) get_$3() const { return $3.get_value(); }

')
define(`sig_memb', `   `Sig_item_'$2 $3;   ///< $3
')
define(`sig_load', `$3(buffer)')
define(`sig_bad', `   virtual typtrans($2) get__$1__$3() const   ///< dito
      { bad_get("$1", "$3"); return 0; }
')
define(`sig_good', `  /// return item $3 of this signal 
   virtual typtrans($2) get__$1__$3() const { return $3.get_value(); }

')
define(`sig_store', `        $3.store(buffer);
')
define(`sig_print', `        $3.print(out);')

divert`'dnl
//----------------------------------------------------------------------------
define(`m4_signal', ``   sid_'$1,')dnl
/// a number identifying the signal
enum Signal_id
{
include(protocol.def)dnl
   sid_MAX,
};
//----------------------------------------------------------------------------
/// the base class for all signal classes
class Signal_base
{
public:
   /// store (encode) the signal into buffer
   virtual void store(string & buffer) const = 0;

   /// print the signal
   virtual ostream & print(ostream & out) const = 0;

   /// return the ID of the signal
   virtual Signal_id get_sigID() const = 0;

   /// return the name of the ID of the signal
   virtual const char * get_sigName() const = 0;

   /// get function for an item that is not defined for the signal
   void bad_get(const char * signal, const char * member) const
      {
        cerr << endl << "*** called function get_" << signal << "__" << member
             << "() with wrong signal " << get_sigName() << endl;
        assert(0 && "bad_get()");
      }
define(`m4_signal', `   /// access functions for signal $1...
expa(`sig_bad', `', $@)')
include(protocol.def)dnl

   /// receive a signal (TCP)
   inline static Signal_base * recv_TCP(int tcp_sock, char * buffer,
                                        int bufsize, char * & del,
                                        ostream * debug);

protected:

   /// send this signal on TCP (or AF_UNIX) socket tcp_sock
   int send_TCP(int tcp_sock) const
       {
         string buffer;
         store(buffer);

         char ll[sizeof(uint32_t)];
         *(uint32_t *)ll = htonl(buffer.size());
         send(tcp_sock, ll, 4, 0);
         ssize_t sent = send(tcp_sock, buffer.data(), buffer.size(), 0);
         return sent;
       }
};
define(`m4_signal',
`//----------------------------------------------------------------------------
/// a class for $1
class $1_c : public Signal_base
{
public:

   /// contructor that creates the signal and sends it on TCP socket s
   $1_c(int s`'expa(`sig_args', `', $@))ifelse(`$#', `1', `', `
   : expa(`sig_init', `,
     ', $@)')
   { send_TCP(s); }

   /// construct (deserialize) this item from a (received) buffer
   /// id has already been load()ed.
   $1_c(const uint8_t * & buffer)ifelse(`$#', `1', `', `
   : expa(`sig_load', `,
     ', $@)')
   {}

   /// store (aka. serialize) this signal into a buffer
   virtual void store(string & buffer) const
       {
         const Sig_item_u16 signal_id(sid_`'$1);
         signal_id.store(buffer);
expa(`sig_store', `', $@)dnl
       }

   /// print this signal on out.
   virtual ostream & print(ostream & out) const
      {
        out << "$1(";
expa(`sig_print', `   out << ", ";
', $@)
        return out << ")" << endl;
      }

   /// a unique number for this signal
   virtual Signal_id get_sigID() const   { return sid_`'$1; }

   /// the name of this signal
   virtual const char * get_sigName() const   { return "$1"; }

expa(`sig_good', `', $@)dnl

protected:
expa(`sig_memb', `', $@)dnl
};')
include(protocol.def)dnl
//----------------------------------------------------------------------------

/// a union big enough for all signal classes
struct _all_signal_classes_
{
define(`m4_signal', `        char u_$1[sizeof($1_c)];')
include(protocol.def)dnl
};

enum { MAX_SIGNAL_CLASS_SIZE = sizeof(_all_signal_classes_) };

//----------------------------------------------------------------------------
Signal_base *
Signal_base::recv_TCP(int tcp_sock, char * buffer, int bufsize,
                                 char * & del, ostream * debug)
{
   if (bufsize < 2*MAX_SIGNAL_CLASS_SIZE)
      {
         // a too small bufsize happens easily but is hard to debug!
         //
         cerr << "\n\n*** bufsize is " << bufsize
              << " but MUST be at least " << 2*MAX_SIGNAL_CLASS_SIZE
              << " in recv_TCP() !!!" << endl;

         return 0;
      }
uint32_t siglen = 0;
   for (;;)
       {
         errno = 0;
         const ssize_t rx_bytes = ::recv(tcp_sock, buffer,
                                     sizeof(uint32_t), MSG_WAITALL);

         if (errno == EINTR)   continue;

         if (rx_bytes != sizeof(uint32_t))
            {
                 // connection was closed by the peer
                 return 0;
            }

         break;   // got  sizeof(uint32_t) length bytes
       }
//    debug && *debug << "rx_bytes is " << rx_bytes
//                    << " when reading siglen in in recv_TCP()" << endl;

   siglen = ntohl(*(uint32_t *)buffer);
   if (siglen == 0)   return 0;   // close

// debug && *debug << "signal length is " << siglen << " in recv_TCP()" << endl;

   // skip MAX_SIGNAL_CLASS_SIZE bytes at the beginning of buffer
   //
char * rx_buf = buffer + MAX_SIGNAL_CLASS_SIZE;
   bufsize -= MAX_SIGNAL_CLASS_SIZE;

   if (siglen > bufsize)
      {
        // the buffer provided is too small: allocate a bigger one
        //
        del = new char[siglen];
        if (del == 0)
           {
             cerr << "*** new(" << siglen <<") failed in recv_TCP()" << endl;
             return 0;
           }
        rx_buf = del;
      }

   for (;;)
       {
          errno = 0;
          const ssize_t rx_bytes = ::recv(tcp_sock, rx_buf,
                                            siglen, MSG_WAITALL);

          if (errno == EINTR)   continue;

          if (rx_bytes != siglen)
             {
               cerr << "*** got " << rx_bytes
                    << " when expecting " << siglen << endl;
               return 0;
             }


         break;   // got siglen bytes
       }

// debug && *debug << "rx_bytes is " << rx_bytes << " in recv_TCP()" << endl;

const uint8_t * b = (const uint8_t *)rx_buf;
Sig_item_u16 signal_id(b);

Signal_base * ret = 0;
   switch(signal_id.get_value())
      {
define(`m4_signal',
       `        case sid_`'$1: ret = new $1`'_c(b);   break;')
include(protocol.def)dnl
        default: cerr << "Signal_base::recv_TCP() failed: unknown signal id "
                      << signal_id.get_value() << endl;
                 errno = EINVAL;
                 return 0;
      }

   debug && ret->print(*debug << "<-- ");

   return ret;   // invalid id
}
//----------------------------------------------------------------------------
