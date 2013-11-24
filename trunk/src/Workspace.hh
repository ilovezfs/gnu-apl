/*
    This file is part of GNU APL, a free implementation of the
    ISO/IEC Standard 13751, "Programming Language APL, Extended"

    Copyright (C) 2008-2013  Dr. Jürgen Sauermann

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

#ifndef __WORKSPACE_HH_DEFINED__
#define __WORKSPACE_HH_DEFINED__

#include <vector>

#include "PrimitiveOperator.hh"
#include "QuadFunction.hh"
#include "Quad_RL.hh"
#include "Quad_SVx.hh"
#include "SkalarFunction.hh"
#include "Symbol.hh"
#include "SymbolTable.hh"
#include "SystemVariable.hh"
#include "UTF8_string.hh"

class Executable;
class StateIndicator;

//-----------------------------------------------------------------------------
/**
    An APL workspace. This structure contains everyting (variables, functions,
    SI stack, etc.) belonging to a single APL workspace.
 */
class Workspace
{
   friend class XML_Loading_Archive;

public:
   /// Construct an empty workspace.
   Workspace();

   /// Destructor
   ~Workspace();

   /// Create a new SI-entry on the SI stack.
   void push_SI(const Executable * fun, const char * loc);

   /// Remove the current SI-entry from the SI stack.
   void pop_SI(const char * loc);

   /// create and execute one immediate execution context
   // (leave with TOK_ESCAPE)
   Token immediate_execution(bool exit_on_error);

   /// clear the workspace
   void clear_WS(ostream & out, bool silent);

   /// clear the SI
   void clear_SI(ostream & out);

   /// print the SI on \b out
   void list_SI(ostream & out, SI_mode mode) const;

   /// return the current QUAD-CT
   static APL_Float get_CT()
      { Assert1(the_workspace);   return the_workspace->v_quad_CT.current(); }

   /// set the current QUAD-CT
   static void set_CT(APL_Float new_CT)
      { Assert(the_workspace);   the_workspace->v_quad_CT.set_current(new_CT); }

   /// Return the current QUAD-FC
   static const APL_Char * get_FC()
      { Assert(the_workspace);   return the_workspace->v_quad_FC.current(); }

   /// Return element \b pos of the current QUAD-FC (pos should be 0..5)
   static APL_Char get_FC(int p)
      { Assert(the_workspace);   return the_workspace->v_quad_FC.current()[p]; }

   /// Return the current QUAD-IO
   static APL_Integer get_IO()
      { Assert(the_workspace);   return the_workspace->v_quad_IO.current(); }

   /// Return the current QUAD-LX
   static UCS_string get_LX()
      { Assert(the_workspace);
        return UCS_string(*the_workspace->v_quad_LX.get_apl_value()); }

   /// Return the current QUAD-PP
   static APL_Integer get_PP()
      { Assert(the_workspace);   return the_workspace->v_quad_PP.current(); }

   /// Return the current QUAD-PR
   static const UCS_string & get_PR()
      { Assert(the_workspace);   return the_workspace->v_quad_PR.current(); }

   /// Return the current QUAD-PS
   static PrintStyle get_PS()
      { Assert(the_workspace);   return the_workspace->v_quad_PS.current(); }

   /// Return the current QUAD-PW.
   static APL_Integer get_PW()
      { Assert1(the_workspace);   return the_workspace->v_quad_PW.current(); }

   /// Return the QUAD-RL.
   APL_Integer get_RL()
      { return v_quad_RL.get_random(); }

   /// the number of SI entries
   int SI_entry_count() const
      { return top_SI ? (top_SI->get_level() + 1) : 0; }

   /// the top of the SI stack (the SI pushed last)
   StateIndicator * SI_top()
      { return top_SI; }

   /// the top of the SI stack (the SI pushed last)
   const StateIndicator * SI_top() const
      { return top_SI; }

   /// the topmost SI with parse mode PM_FUNCTION
   StateIndicator * SI_top_fun();

   /// the topmost SI with an error
   const StateIndicator * SI_top_error() const;

   /// the number of symbols in the symbol table
   uint32_t symbols_allocated() const
      { return symbol_table.symbols_allocated(); }

   /// copy all allocated symbols into \b table of size \b table_size
   void get_all_symbols(Symbol ** table, uint32_t table_size) const
      { return symbol_table.get_all_symbols(table, table_size); }

   /// lookup an existing user defined symbol. If not found, create one
   /// (unless this would be a quad symbol)
   Symbol * lookup_symbol(const UCS_string & symbol_name)
      { return symbol_table.lookup_symbol(symbol_name);}

   /// clear ⎕EM and ⎕ET
   void clear_error(const char * loc)
      { v_quad_EM.clear(loc);   v_quad_ET.clear(loc); }

   /// update ⎕EM and ⎕ET according to \b error
   void update_EM_ET(const Error & error)
      { v_quad_EM.update(error);   v_quad_ET.update(error); }

   /// lookup an existing name (user defined or ⎕xx, var or function).
   /// return 0 if not found.
   NamedObject * lookup_existing_name(const UCS_string & name);

   /// lookup an existing symbol (user defined or ⎕xx).
   Symbol * lookup_existing_symbol(const UCS_string & symbol_name);

   /// increase the wait time for user input as reported in ⎕AI
   void add_wait(APL_time diff)
      { v_quad_AI.add_wait(diff); }

   /// return information in SI_top()
   Error * get_error()
      { 
        return top_SI ? &top_SI->get_error() : 0;
      }

   /// save this workspace
   void save_WS(ostream & out, vector<UCS_string> & lib_ws);

   /// set or inquire the workspace ID
   void wsid(ostream & out, UCS_string arg);

   /// load new workspace \b lib_ws. Return ⎕LX of the new WS.
   UCS_string load_WS(ostream & out, const vector<UCS_string> & lib_ws);

   /// copy obkects from another workspace
   void copy_WS(ostream & out, vector<UCS_string> & lib_ws, bool protection);

   /// return a token for system function or variable \b ucs
   Token get_quad(UCS_string ucs, int & len);

   /// return oldest SI entry that is running \b exex, or 0 if none
   StateIndicator * oldest_exec(const Executable * exec) const;

   /// return true iff function \b funname is on the current call stack
   bool is_called(const UCS_string & funname) const;

   /// write symbols for )OUT command
   void write_OUT(FILE * out, uint64_t & seq,
                  const vector<UCS_string> & objects);

   /// erase the symbols in \b symbols from the symbol table
   void erase_symbols(ostream & out, const vector<UCS_string> & symbols)
      { symbol_table.erase_symbols(out, symbols); }

   /// list all symbols (of category \b which) with names in \b from_to
   void list_symbols(ostream & out, ListCategory which,
                     UCS_string from_to) const
      { symbol_table.list_symbols(out, which, from_to); }

   /// list all symbols with names in \b buf
   ostream & list_symbol(ostream & out, const UCS_string & buf) const
      { return symbol_table.list_symbol(out, buf); }

   /// clear the marked flag in all values known in this workspace
   void unmark_all_values() const;

   /// print all owners of \b value
   int show_owners(ostream & out, const Value_P value) const;

   /// maybe remove functions for which ⎕EX has failed
   int cleanup_expunged(ostream & out, bool & erased);

   /// replace old_value by new_value in the next higher SI context. Called
   /// when a variable is assigned and the old value has arg flag set.
   void replace_arg(Value_P old_value, Value_P new_value);

   /// the name of this workspace.
   UCS_string WS_name;

   /// The symbol table of this workspace.
   SymbolTable symbol_table;

   // system variables.
   //
#define ro_sv_def(x) Quad_ ## x v_quad_ ## x;
#define rw_sv_def(x) Quad_ ## x v_quad_ ## x;
#include "SystemVariable.def"

   Quad_QUAD         v_quad_QUAD;         ///< System Variable.
   Quad_QUOTE        v_quad_QUOTE;        ///< System Variable.

   /// the APL prompt (6 blankls by default)
   UCS_string prompt;

   /// The current workspace (for objects that need one but don't have one).
   static Workspace * the_workspace;

   /// user defined functions that were ⎕EX'ed while on the SI stack
   vector<const UserFunction *> expunged_functions;

   /// last command error
   UCS_string more_error;
protected:
   /// the SI stack. Initially top_SI is 0 (empty stack)
   StateIndicator * top_SI;
};
//-----------------------------------------------------------------------------

#endif // __WORKSPACE_HH_DEFINED__
