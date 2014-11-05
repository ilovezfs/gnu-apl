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

#ifndef __PARSER_HH_DEFINED__
#define __PARSER_HH_DEFINED__

#include "Token.hh"

/*!
     Functions related to parsing of input lines and user defined functions.
 */
class Parser
{
public:
   /// Constructor
   Parser(ParseMode pm, const char * loc)
   : pmode(pm),
     create_loc(loc)
   {}

   /// Parse UCS_string \b input into token string \b tos.
   ErrorCode parse(const UCS_string & input, Token_string & tos) const;

   /// Parse token string \b input into token string \b tos.
   ErrorCode parse(const Token_string & input, Token_string & tos) const;

   /// remove VOID token from \b tos (compacting \b tos)
   static void remove_void_token(Token_string & tos);

   /// compute distances between matching (), [], and {}
   static ErrorCode match_par_bra(Token_string & tos, bool backwards);

protected:
   /// Parse token string \b tos (a statement without diamonds).
   static ErrorCode parse_statement(Token_string & tos);

   /// find opening bracket; throw error if not found
   static int find_opening_bracket(const Token_string & tos, int pos);

   /// find closing bracket; throw error if not found
   static int find_closing_bracket(const Token_string & tos, int pos);

   /// find opening paranthesis; throw error if not found.
   static int find_opening_parent(const Token_string & tos, int pos);

   /// find closing paranthesis; throw error if not found.
   static int find_closing_parent(const Token_string & tos, int pos);

   /// Collect consecutive smaller APL values or value token into vectors
   static void collect_constants(Token_string & tos);

   /// mark next symbol left of ← on the same level as LSYMB
   static void mark_lsymb(Token_string & tos);

   /// Collect groups
   static bool collect_groups(Token_string & tos);

   /// Replace (X) by X and ((..) by (..) in \b string. (X is a single token)
   static void remove_nongrouping_parantheses(Token_string & tos);

   /// degrade / ⌿ \ and ⍀ from OPER1 to FUN2
   static void degrade_scan_reduce(Token_string & tos);

   /// check if tos[pos] is the end of a value or of a function
   static bool check_if_value(const Token_string & tos, int pos);

   /// Create one APL value from \b len values.
   static void create_value(Token_string & tos, int pos, int len);

   /// Create one scalar APL value from a token.
   static void create_scalar_value(Token & tok);

   /// Create one vector APL value from \b value_count single values.
   static void create_vector_value(Token_string & tos, int pos, int len);

   /// the parsing mode of this parser
   const ParseMode pmode;

   /// where this parser was constructed
   const char * create_loc;
};

#endif // __PARSER_HH_DEFINED__
