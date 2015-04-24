/*
    This file is part of GNU APL, a free implementation of the
    ISO/IEC Standard 13751, "Programming Language APL, Extended"

    Copyright (C) 2008-2015  Dr. Jürgen Sauermann

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

#ifndef __SYMBOLTABLE_HH_DEFINED__
#define __SYMBOLTABLE_HH_DEFINED__

#include <stdint.h>

#include "UCS_string.hh"

//-----------------------------------------------------------------------------
/// common part of user-defined names and sistinguished names
template <typename T, int SYMBOL_COUNT>
class SymbolTableBase
{
public:
   /// Construct an empty \b SymbolTable.
   SymbolTableBase()
     { memset(symbol_table, 0, sizeof(symbol_table)); }

   /// Return a \b Symbol with name \b name in \b this \b SymbolTable.
   T * lookup_existing_symbol(const UCS_string & name)
      {
        const uint32_t hash = compute_hash(name);
        for (T * sp = symbol_table[hash]; sp; sp = sp->next)
            {
              if (!sp->is_erased() && sp->equal(name))   return sp;
            }

        return 0;
      }

   /// add \b new_name to the symbol table. The caller has checked
   /// that new_name does not yet exist in the symbol table
   T * add_symbol(const UCS_string & new_name, ID::Id id)
       {
         const uint32_t hash = compute_hash(new_name);
         Log(LOG_SYMBOL_lookup_symbol)
            {
              CERR << "Symbol " << new_name
                   << " has hash " << HEX(hash) << endl;
            }

         if (symbol_table[hash] == 0)   // unused slot
            {
              T * new_symbol = new T(new_name, id);
              symbol_table[hash] = new_symbol;
              new_symbol->next = 0;
              return new_symbol;
            }

         for (T * t = symbol_table[hash]; ; t = t->next)
             {
               if (t->is_erased())   // override an erased symbol.
                  {
                    t->replace_name(new_name);
                    t->set_erased(false);
                    return t;
                  }

               if (t->next == 0)   // append new_symbol at the end
                  {
                    Symbol * new_symbol = new Symbol(new_name, ID::USER_SYMBOL);
                    t->next = new_symbol;
                    new_symbol->next = 0;
                    return new_symbol;
                  }
             }

       }

   /// compute 16-bit hash for \b name
   static uint32_t compute_hash(const UCS_string & name)
      {
        // Parameters for the FNV-1 hash.
        enum {
               FNV_Offset_32 = 0x811C9DC5,
               FNV_Prime_32  = 16777619
             };

        uint32_t hash = FNV_Offset_32;
        for (int s = 0; s < name.size(); ++s)
            hash = (hash * FNV_Prime_32) ^ name[s];
        return  ((hash >> 16) ^ hash) % SYMBOL_COUNT;
     }

protected:
   /// Hash table for all symbols.
   T * symbol_table[SYMBOL_COUNT];
};

class Symbol;

//-----------------------------------------------------------------------------
/**
     A table containing all user defined symbols.
 */
class SymbolTable : public SymbolTableBase<Symbol, MAX_SYMBOL_COUNT>
{
public:
   /// Return or create a \b Symbol with name \b ucs in \b this \b SymbolTable.
   Symbol * lookup_symbol(const UCS_string & ucs);

   /// List all symbols in \b this \b SymbolTable (for )VARS, )FNS etc.)
   void list(ostream & out, ListCategory which, UCS_string from_to) const;

   /// clear the marked flag of all symbols
   void unmark_all_values() const;

   /// print variables owning value
   int show_owners(ostream & out, const Value & value) const;

   /// clear this symbol table (remove all user-defined symbols)
   void clear(ostream & out);

   /// clear one slot (hash) in this symbol table
   void clear_slot(ostream & out, int hash);

   /// erase symbols from \b this SymbolTable
   void erase_symbols(ostream & out, const vector<UCS_string> & symbols);

   /// List details of single symbol in buf.
   ostream & list_symbol(ostream & out, const UCS_string & buf) const;

   /// write all symbols in )OUT format to file \b out
   void write_all_symbols(FILE * out, uint64_t & seq) const;

   /// Number of symbols in \b this \b SymbolTable (including erased symbols)
   int symbols_allocated() const;

   /// return all symbols  (including erased symbols)
   void get_all_symbols(Symbol ** table, int table_size) const;

   /// dump symbols to out
   void dump(ostream & out, int & fcount, int & vcount) const;

protected:
   /// erase one symbol, return \b true on error, \b false on success
   bool erase_one_symbol(const UCS_string & sym);
};
//-----------------------------------------------------------------------------
class QuadFunction;
class SystemVariable;

class SystemName
{
public:
   /// constructor: prefix of a system variable or function
   SystemName(const UCS_string & prefix)
   : name(prefix),
     id(ID::Quad_PREFIX),
     function(0),
      sysvar(0)
   {}

   /// constructor: system variable
   SystemName(const UCS_string & var_name, ID::Id var_id, SystemVariable * var)
   : name(var_name),
     id(var_id),
     function(0),
      sysvar(var)
   {}
     
   /// constructor: system function
   SystemName(const UCS_string & fun_name, ID::Id fun_id, QuadFunction * fun)
   : name(fun_name),
     id(fun_id),
     function(fun),
      sysvar(0)
   {}
     
protected:
   UCS_string name;
   ID::Id id;
   QuadFunction * function;
   SystemVariable * sysvar;
};

/**
     A table containing system defined symbols (aka. distinguished names)
 */

class SystemSymTab : public SymbolTableBase<SystemName, 256 - 1>
{
public:
};
//-----------------------------------------------------------------------------

#endif // __SYMBOLTABLE_HH_DEFINED__
