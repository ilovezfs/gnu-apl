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

   /// add \b sym to the symbol table. The caller has checked
   /// that new_name does not yet exist in the symbol table
   void add_symbol(T * sym)
       {
         const uint32_t hash = compute_hash(sym->get_name());
         if (symbol_table[hash] == 0)   // unused slot
            {
              symbol_table[hash] = sym;
              sym->next = 0;
              return;
            }

         for (T * t = symbol_table[hash]; ; t = t->next)
             {
               if (t->is_erased())   // override an erased symbol.
                  {
                    t->replace_name(sym->get_name());
                    t->set_erased(false);
                    return;
                  }

               if (t->next == 0)   // append new_symbol at the end
                  {
                    t->next = sym;
                    sym->next = 0;
                    return;
                  }
             }
       }

   /// compute a 16-bit hash for \b name
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

        hash = (((hash >> 16) ^ hash) & 0x0000FFFF) % SYMBOL_COUNT;

        Log(LOG_SYMBOL_lookup_symbol)
           {
              CERR << "name[len=" << name.size() << "] " << name
                   << " has hash " << HEX(hash)
                   << " in table of size " << SYMBOL_COUNT << endl;
           }

        return  hash;
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
   /// constructor: system variable or function
   SystemName(const UCS_string & var_name, ID::Id var_id,
              QuadFunction * fun,  SystemVariable * var)
   : name(var_name),
     id(var_id),
     function(fun),
      sysvar(var)
   {}

   /// return the distinguished name
   const UCS_string & get_name() const
      { return name; }

   /// never called
   void replace_name(const UCS_string &) const { FIXME; }

   /// never called
   void set_erased(bool) const { FIXME; }

   /// system names are never erased
   bool is_erased()   { return false; }

   /// Compare the name of \b this \b Symbol with \b ucs
   bool equal(const UCS_string & ucs) const
      { return (name.compare(ucs) == COMP_EQ); }

   SystemVariable * get_variable() const
      { return sysvar; }

   QuadFunction * get_function() const
      { return function; }

   ID::Id get_id() const
      { return id; }

   /// The next name with the same hash value as \b this \b SystemName
   SystemName * next;

protected:
   /// the name (including ⎕). Eg. ⎕IO
   const UCS_string name;

   /// the Id of the variable or function
   const ID::Id id;

   /// the function if the name refers to a system function, or 0 if not
   QuadFunction * function;

   /// the variable if the name refers to a system variable, or 0 if not
   SystemVariable * sysvar;
};

/**
     A table containing system defined symbols (aka. distinguished names)
 */

class SystemSymTab : public SymbolTableBase<SystemName, 256 - 1>
{
public:
   SystemSymTab()
   : max_name_len(0)
   {}

   void add_function(const UCS_string & name, ID::Id id,
                     QuadFunction * function)
      { add_fun_or_var(name, id, function, 0); }

   void add_variable(const UCS_string & name, ID::Id id,
                     SystemVariable * variable)
      { add_fun_or_var(name, id, 0, variable); }

   /// don't add ⍺ and friends
   void add_variable(const UCS_string & name, ID::Id id, Symbol * variable)
      { }

protected:
   void add_fun_or_var(const UCS_string & name, ID::Id id,
                       QuadFunction * function, SystemVariable * variable);

   // the length of the longest name
   int max_name_len;
};
//-----------------------------------------------------------------------------

#endif // __SYMBOLTABLE_HH_DEFINED__
