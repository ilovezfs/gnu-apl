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


#include "Executable.hh"
#include "Output.hh"
#include "PrintOperator.hh"
#include "UCS_string.hh"
#include "UserFunction.hh"
#include "Value.icc"
#include "Workspace.hh"

//-----------------------------------------------------------------------------
Executable::Executable(const UCS_string & ucs,  bool multi_line,
                       ParseMode pm, const char * loc)
   : alloc_loc(loc),
     pmode(pm),
     refcount(0)
{
// { cerr << "Executable " << (void *)this << " created at " << loc << endl; }

   // initialize text, stripping carriage returns and line feeds.
   //
   if (multi_line)
      {
        UCS_string line;

        loop(t, ucs.size())
           {
             const Unicode uni = ucs[t];
             switch (uni)
                {
                  case UNI_ASCII_CR:                         break;
                  case UNI_ASCII_LF: text.push_back(line);
                                     line.clear();           break;
                  default:           line.append(uni);
               }
           }

        if (line.size())   text.push_back(line);
      }
   else
      {
        text.push_back(ucs);
      }
}
//-----------------------------------------------------------------------------
Executable::Executable(Fun_signature sig, int lambda_num,
                       const UCS_string & lambda_text, const char * loc)
   : alloc_loc(loc),
     pmode(PM_FUNCTION),
     refcount(0)
{
   text.push_back(UserFunction_header::lambda_header(sig, lambda_num));
   text.push_back(lambda_text);
}
//-----------------------------------------------------------------------------
Executable::~Executable()
{
   Log(LOG_UserFunction__fix)
      {
        CERR << "deleting Executable " << (const void *)this
             << " (body size=" <<  body.size() << ")" << endl;
      }

   clear_body();
}
//-----------------------------------------------------------------------------
void
Executable::clear_body()
{
   loop(b, body.size())
      {
        body[b].extract_apl_val(LOC);

        if (body[b].is_function())
           {
              Function * fun = body[b].get_function();
              UserFunction * ufun = fun->get_ufun1();
              if (ufun && ufun->is_lambda())   ufun->decrement_refcount(LOC);
           }
      }

   body.clear();   // resize to 0
}
//-----------------------------------------------------------------------------
ErrorCode
Executable::parse_body_line(Function_Line line, const UCS_string & ucs_line,
                            bool trace, bool tolerant, const char * loc,
                            bool macro)
{
   Log(LOG_UserFunction__set_line)
      CERR << "[" << line << "]" << ucs_line << endl;

Token_string in;
const Parser parser(get_parse_mode(), loc, macro);
ErrorCode ec = parser.parse(ucs_line, in);
   if (ec)
      {
        if (tolerant)   return ec;

        if (ec == E_NO_TOKEN)
           {
             Error error(ec, LOC);
             throw error;
           }

        if (ec != E_NO_ERROR)   throw_parse_error(ec, LOC, LOC);
      }

   return parse_body_line(line, in, trace, tolerant, loc);
} 
//-----------------------------------------------------------------------------
ErrorCode
Executable::parse_body_line(Function_Line line, const Token_string & in,
                            bool trace,  bool tolerant, const char * loc)
{
Source<Token> src(in);

   // handle labels (if any)
   //
   if (get_parse_mode() == PM_FUNCTION &&
       src.rest() >= 2                 &&
       src[0].get_tag() == TOK_SYMBOL  &&
       src[1].get_tag() == TOK_COLON)
      {
        Token tok = src.get();   // get the label symbol
        ++src;                   // skip the :

        UserFunction * ufun = get_ufun();
        Assert(ufun);
        ufun->add_label(tok.get_sym_ptr(), line);
      }

Token_string out;
   while (src.rest())
      {
        // find next diamond. If there is one, copy statement in reverse order,
        // append TOK_ENDL, and continue.
        
        // 1. determine statement length
        //
        ShapeItem stat_len = 0;   // statement length, not counting ◊
        int diamond = 0;
        loop(t, src.rest())
            {
              if (src[t].get_tag() == TOK_DIAMOND)
                 {
                   diamond = 1;
                   break;
                 }
              else
                 {
                   ++stat_len;
                 }
            }

        // 2. check for bad token and append statement to out
        //
        loop(s, stat_len)
            {
              const Token & tok = src[stat_len - s - 1];
              if (tok.get_Class() >= TC_MAX_PERM &&
                  tok.get_tag() != TOK_L_CURLY &&
                  tok.get_tag() != TOK_R_CURLY)
                 {
                   if (tolerant)   return E_SYNTAX_ERROR;

                   CERR << "Line " << line << endl
                        << "Offending token: (tag > TC_MAX_PERM) "
                        << tok.get_tag() << " " << tok << endl
                        << "Statement: ";
                   Token::print_token_list(CERR, in);
                   SYNTAX_ERROR;
                 }
              out.append(tok);
            }
        src.skip(stat_len + diamond);   // skip statement and maybe ◊

        // 3. maybe add a TOK_END or TOK_ENDL
        //
        if (stat_len && (diamond || get_parse_mode() != PM_EXECUTE))
           {
             const int64_t tr = trace ? 1 : 0;
             out.append(Token(TOK_END, tr), LOC);
           }
      }

   // replace the trailing TOK_END (if any) into TOK_ENDL
   //
   if (out.size() && out.last().get_tag() == TOK_END)
      out.last().ChangeTag(TOK_ENDL);

   Log(LOG_UserFunction__set_line)
      {
        CERR << "[final line " << line << "] ";
        out.print(CERR);
      } 

   loop(t, out.size())   body.append(out[t], LOC);
   return E_NO_ERROR;
}
//-----------------------------------------------------------------------------
Token
Executable::execute_body() const
{
StateIndicator & si = *Workspace::SI_top();

   try                 { return si.run();                         }
   catch (Error err)   { return Token(TOK_ERROR, err.error_code); }
   catch (Token tok)   { return tok;                              }
   catch (...)         { return Token(TOK_ERROR, E_SYSTEM_ERROR); }

   // not reached
   FIXME;
}
//-----------------------------------------------------------------------------
void
Executable::print_token(ostream & out) const
{
   out << endl
       <<  "Function body [" << body.size() << " token]:" << endl;

   body.print(out);
}
//-----------------------------------------------------------------------------
void
Executable::print_text(ostream & out) const
{
   loop(l, text.size())   out << text[l] << endl;
}
//-----------------------------------------------------------------------------
UCS_string
Executable::statement_text(Function_PC pc) const
{
   if (pc >= body.size())
      {
        Assert(pc == body.size());
        pc = Function_PC(body.size() - 1);
      }

   if (pc > 0 && body[pc].get_Class() == TC_RETURN)   pc = Function_PC(pc - 1);
   if (pc > 0 && body[pc].get_Class() == TC_END)      pc = Function_PC(pc - 1);

const Function_Line line = get_line(pc);
const UCS_string & line_txt = text[line];

   // count statement ends before pc
   //
int stats_before = 0;
   for (Function_PC p = line_start(line); p < pc; ++p)
       {
         // a TOK_STOP_LINE is immediately followd by a TC_END, but the
         // TC_END has no ◊ in line_txt. We therefore discount stats_before.
         //
         if (body[p].get_tag() == TOK_STOP_LINE)          --stats_before;

         if (body[p].get_Class() == TC_END)               ++stats_before;
         else if (body[p].get_tag() == TOK_RETURN_EXEC)   ++stats_before;
       }

int tidx = 0;
   while (stats_before)
      {
        Assert(tidx < line_txt.size());
        if (line_txt[tidx++] == UNI_DIAMOND)   --stats_before;
      }

   // skip leading spaces
   //
   while (line_txt[tidx] == ' ')   ++tidx;

UCS_string ret;
   while (tidx < line_txt.size())
      {
        if (line_txt[tidx] == UNI_DIAMOND)    break;
        ret.append(line_txt[tidx++], LOC);
      }

   // skip trailing spaces
   //
   while (ret.size() && ret.back() <= ' ')   ret.pop_back();

   return ret;
}
//-----------------------------------------------------------------------------
void
Executable::set_error_info(Error & error, Function_PC2 pc_from_to) const
{
const ErrorCode ec = error.error_code;
UCS_string & message_2 = error.error_message_2;

   // for value errors we point to the failed symbol itself.
   //
   if (ec == E_VALUE_ERROR)   pc_from_to.low = pc_from_to.high;

   Log(LOG_prefix__location_info)
      {
        Q1(pc_from_to.low)
        Q1(pc_from_to.high)
      }

   if (body[pc_from_to.high].get_Class() == TC_RETURN &&
       pc_from_to.high > pc_from_to.low)
       pc_from_to.high = Function_PC(pc_from_to.high - 1);

/*
    The statement is stored in reverse order in the body:

    Body:  
          start ... pc_low      ...       pc_high ... end
                    ^                     ^
                    |                     |
                    +--- failed action ---+

    Display:
          end ... pc_high      ...      pc_low ... start ◊ next statement
                  ^                     ^
                  |                     |
                  +--- failed action ---+
 */

const Function_PC start = get_statement_start(pc_from_to.low);
Function_PC end = start;
   while (body[end].get_Class() != TC_END &&
          body[end].get_tag() != TOK_RETURN_EXEC)   ++end;

   Assert(start   <= pc_from_to.low);
   Assert(pc_from_to.low  <= pc_from_to.high);
   if (pc_from_to.high > end)   pc_from_to.high = end;

   // if the error was caused by some function being executed then
   // the right arg of the function was OK and we skip it
   //
   if (body[pc_from_to.low].get_Class() == TC_VALUE)
      {
         ++pc_from_to.low;
      }

   Log(LOG_prefix__location_info)
      {
        Q1(start)
        Q1(pc_from_to.low)
        Q1(pc_from_to.high)
        Q1(end)
      }

   // Line 2: statement
   //
int len_left = 0;      // characters before the first caret
int len_between = 0;   // distance between the carets
   for (int q = end - 1; q >= start; --q)
       {
         // avoid duplicate ∘ in ∘.f
         //
         if (body[q].get_tag() == TOK_JOT && q > 0 &&
             body[q - 1].get_tag() == TOK_OPER2_OUTER)   continue;

         // Note: Token::error_info returns -len if it inserts a space.
         // Such an inserted space counts for the previous token.
         // The previous token is q + 1 since we count down.
         //
         int len = body[q].error_info(message_2);

         if (len < 0)   // space inserted, len is negative
            {
              if ((q + 1) > pc_from_to.high)       len_left++;
              else if ((q + 1) > pc_from_to.low)   len_between++;
              len = -len;
            }

         if (q > pc_from_to.high)         len_left += len;
         else if (q > pc_from_to.low)   len_between += len;
       }

   // Line 3: carets
   //
   error.left_caret += len_left;

   if (pc_from_to.high != pc_from_to.low)   // two carets
      {
        error.right_caret = error.left_caret + len_between;
      }
}
//-----------------------------------------------------------------------------
Function_PC
Executable::get_statement_start(int pc) const
{
   // this function is used in error reporting so it should
   // not Assert() and the like to avoid infinite recursion.

   if (pc >= body.size())   pc = body.size() - 1;

   // if we are at the end of the statement, move back.
   //
   if (pc < 2)   return Function_PC_0;
   if (body[pc].get_Class() == TC_RETURN)   --pc;
   if (body[pc].get_Class() == TC_END)      --pc;

   for (; pc > 0; --pc)
      {
        if (body[pc].get_Class() == TC_END)          return Function_PC(pc + 1);
        if (body[pc].get_tag() == TOK_RETURN_EXEC)   return Function_PC(pc + 1);
      }

   return Function_PC_0;
}
//-----------------------------------------------------------------------------
void
Executable::unmark_all_values() const
{
   loop (b, body.size())
      {
        const Token & tok = body[b];
        if (tok.get_ValueType() == TV_VAL)
           {
             Value_P value = tok.get_apl_val();
             if (!!value)   value->unmark();
           }

        if (tok.get_ValueType() == TV_FUN)
           {
             Function * fun = tok.get_function();
             UserFunction * ufun = fun->get_ufun1();
             if (ufun && ufun->is_lambda())
                {
                  ufun->unmark_all_values();
                }
           }
      }
}
//-----------------------------------------------------------------------------
int
Executable::show_owners(const char * prefix, ostream & out, const Value & value) const
{
int count = 0;

   loop (b, body.size())
      {
        const Token & tok = body[b];
        if (tok.get_ValueType() != TV_VAL)      continue;

        if (Value::is_or_contains(tok.get_apl_val().get(), value))
           {
             out << prefix << get_name() << "[" << b << "]" << endl;
             ++count;
           }
      }

   return count;
}
//-----------------------------------------------------------------------------
void
Executable::setup_lambdas()
{
   // quick check if this body contains any lambdas. Parser::match_par_bra()
   // should have checked for unbalanced { } so we simply look for } (which
   // is the first to occur since the body is reversed.
   //
bool have_curly = false;
   loop(b, body.size())
       {
         if (body[b].get_tag() == TOK_R_CURLY)
            {
              have_curly = true;
              break;
            }
       }

   if (!have_curly)   return;   // no { ... } in this body

   // undo the token reversion of the body so that the body token and the
   // text run in the same direction.
   //
   reverse_statement_token(body);

int lambda_num = 0;
   loop(b, body.size())
       {
         if (body[b].get_tag() != TOK_L_CURLY)   continue;   // not {

         b = setup_one_lambda(b, ++lambda_num) - 1;   // -1 due to ++b in loop(b) above
       }

   // redo the token reversion of the body so that the body token and the
   // text run in the same direction.
   //
   reverse_statement_token(body);

   Parser::match_par_bra(body, true);
   adjust_line_starts();
   Parser::remove_void_token(body);   // do this AFTER adjust_line_starts() !!!
}
//-----------------------------------------------------------------------------
ShapeItem
Executable::setup_one_lambda(ShapeItem b, int lambda_num)
{
const ShapeItem bend = b + body[b].get_int_val2();   // the corresponding }
   Assert(bend < body.size());
   Assert(body[bend].get_tag() == TOK_R_CURLY);

   body[b++].clear(LOC);    // invalidate {
   body[bend].clear(LOC);   // invalidate }

Token_string lambda_body;

const Fun_signature signature =
      compute_lambda_body(lambda_body, b, bend);

   // at this point { ... } was copied from body to lambda_body and
   // bend is at the (now invalidated) { token.
   //
const UCS_string lambda_text = extract_lambda_text(signature, lambda_num - 1);

   reverse_statement_token(lambda_body);
   reverse_all_token(lambda_body);

#if 0  // not yet working

UCS_string ufun_text = UserFunction_header::lambda_header(signature,lambda_num);
   ufun_text.append(UNI_ASCII_LF);
   ufun_text.append(lambda_text);
   ufun_text.append(UNI_ASCII_LF);

Q(ufun_text)

int error_line = -1;
const char * error_loc = 0;
const UTF8_string creator("Executable::setup_one_lambda()");

UserFunction * ufun = new UserFunction(ufun_text, error_line, error_loc,
                                       false, LOC, creator, false);

   ufun->increment_refcount(LOC);

#else

UserFunction * ufun = new UserFunction(signature, lambda_num,
                                       lambda_text, lambda_body);
   ufun->increment_refcount(LOC);

#endif
   // put a token for the lambda at the place where the { was.
   // That replaces, for example, (in forward notation):
   //
   // A←{ ... }   by:
   // A←UFUN      UFUN being a user-defined function with body { ... }
   //
   move_2(body[bend], ufun->get_token(), LOC);

   return bend + 1;
}
//-----------------------------------------------------------------------------
Fun_signature
Executable::compute_lambda_body(Token_string & lambda_body,
                                ShapeItem b, ShapeItem bend)
{
int signature = SIG_FUN;
int level = 0;

   for (; b < bend; ++b)
       {
         Token t;
         move_1(t, body[b], LOC);
         body[b].clear(LOC);   // invalidate in main body

         // figure the signature by looking for ⍺, ⍶, ⍵, ⍹, and χ
         // and complain about ◊ and →
         switch(t.get_tag())
            {
              case TOK_ALPHA:   if (!level)   signature |= SIG_A;   // no break
              case TOK_OMEGA:   if (!level)   signature |= SIG_B;         break;
              case TOK_CHI:     if (!level)   signature |= SIG_X;         break;
              case TOK_OMEGA_U: if (!level)   signature |= SIG_RO;  // no break
              case TOK_ALPHA_U: if (!level)   signature |= SIG_LO;        break;

              case TOK_DIAMOND: DEFN_ERROR;
              case TOK_BRANCH:  DEFN_ERROR;
              case TOK_ESCAPE:  DEFN_ERROR;

              case TOK_L_CURLY: ++level;   break;
              case TOK_R_CURLY: --level;   break;
              default: break;
            }

         if (lambda_body.size() == 0)
            {
              // first token: prepend λ ← tokens
              //
              Symbol * sym_Z = &Workspace::get_v_LAMBDA();
              lambda_body.append(Token(TOK_LAMBDA, sym_Z), LOC);
              lambda_body.append(Token(TOK_ASSIGN), LOC);

              signature |= SIG_Z;
            }

         lambda_body.append(t, LOC);
       }

   // if the lambda has at least one token, then it (is supposed to) return λ.
   // Otherwise the lambda is empty (and result-less)
   //
   if (signature & SIG_Z)
      {
        const int64_t trace = 0;
        lambda_body.append(Token(TOK_ENDL, trace));

        Token ret_lambda(TOK_RETURN_SYMBOL, &Workspace::get_v_LAMBDA());
        lambda_body.append(ret_lambda, LOC);
      }
   else
      {
        Token ret_void(TOK_RETURN_VOID);
        lambda_body.append(ret_void, LOC);
      }

   return (Fun_signature)signature;
}
//-----------------------------------------------------------------------------
UCS_string
Executable::extract_lambda_text(Fun_signature signature, int skip) const
{
UCS_string lambda_text;
   if (signature & SIG_Z)   lambda_text.append_utf8("λ←");

int level = 0;   // {/} nesting level
bool copying = false;

size_t tidx = 0;
int tcol = 0;

   // skip over the first skip lambdas and copy the next one to lambda_text
   //
   for (;;)
       {
         if (tidx >= text.size())
            {
              Q1(copying)   Q1(tidx)   FIXME;
            }

         const UCS_string & line = text[tidx];

         if (tcol >= line.size())   // end of line: wrap to next line
            {
              ++tidx;
              tcol = 0;
              continue;
            }

         const Unicode uni = line[tcol++];
         if (copying)   lambda_text.append(uni);

         if (uni == UNI_ASCII_L_CURLY)
            {
              if (level++ == 0)   // top-level {
                 {
                   if (skip == 0)   copying = true;
                 }
            }
         else if (uni == UNI_ASCII_R_CURLY)
            {
              if (--level == 0)   // top-level }
                 {
                   if (copying)   break;   // done
                   --skip;
                 }
            }
       }

   lambda_text.pop();   // the last }
   return lambda_text;
}
//-----------------------------------------------------------------------------
void
Executable::reverse_statement_token(Token_string & tos)
{
ShapeItem from = 0;

   for (ShapeItem to = from + 1; to < tos.size(); ++to)
       {
         if (tos[to].get_Class() == TC_END)
            {
              tos.reverse_from_to(from, to - 1);   // except TC_END
              from = to + 1;
            }
         else if (to == (tos.size() - 1))
            {
              tos.reverse_from_to(from, to);
              from = to + 1;
            }
       }
}
//-----------------------------------------------------------------------------
void
Executable::reverse_all_token(Token_string & tos)
{
Token * t1 = &tos[0];
Token * t2 = &tos[tos.size() - 1];

   for (;t1 < t2; ++t1, --t2)
       {
         char tt[sizeof(Token)];
         memcpy(tt, t1, sizeof(Token));
         memcpy(t1, t2,   sizeof(Token));
         memcpy(t2, tt, sizeof(Token));
       }
}
//-----------------------------------------------------------------------------
void
Executable::increment_refcount(const char * loc)
{
   Assert1(get_ufun());
   Assert(get_ufun()->is_lambda());

   ++refcount;

// CERR << "*** increment_refcount() of " << get_name()
//      << " to " << refcount << " at " << loc << endl;A
}
//-----------------------------------------------------------------------------
void
Executable::decrement_refcount(const char * loc)
{
   Assert1(get_ufun());
   Assert(get_ufun()->is_lambda());

   if (refcount <= 0)
      {
        CERR << "*** Warning: refcount of " << get_name() << " is " << refcount
             << ":" << endl;
        print_text(CERR);
//     FIXME;
      }

   --refcount;

// CERR << "*** decrement_refcount() of " << get_name()
//      << " to " << refcount << " at " << loc << endl;


   if (refcount <= 0)
      {
//      CERR << "*** lambda died" << endl;
        clear_body();
      }
}
//=============================================================================
ExecuteList *
ExecuteList::fix(const UCS_string & data, const char * loc)
{
   // clear errors that may have occured before
   {
     Error * err = Workspace::get_error();
     if (err) err->error_code = E_NO_ERROR;
   }

ExecuteList * fun = new ExecuteList(data, loc);

   Log(LOG_UserFunction__fix)
      {
        CERR << "fix pmode=execute list:" << endl << data
             << " addr " << (const void *)fun << endl
             << "------------------- ExecuteList::fix() --" << endl;
      }

   {
     Error * err = Workspace::get_error();
     if (err && err->error_code)
        {
          Log(LOG_UserFunction__fix)
             {
                CERR << "fix pmode=execute list failed with error "
                     << Error::error_name(err->error_code) << endl;
             }

          err->parser_loc = 0;
          delete fun;
          return 0;
        }
   }

   try
      {
        fun->parse_body_line(Function_Line_0, data, false, false, loc, false);
      }
   catch (Error err)
      {
        Error * werr = Workspace::get_error();
        if (werr)   *werr = err;
        delete fun;
        return 0;
      }

   fun->setup_lambdas();

   Log(LOG_UserFunction__fix)
      {
        CERR << "fun->body.size() is " << fun->body.size() << endl;
      }

   // for ⍎ we don't append TOK_END, but only TOK_RETURN_EXEC.
   fun->body.append(Token(TOK_RETURN_EXEC), LOC);

   Log(LOG_UserFunction__fix)   fun->print(CERR);
   return fun;
}
//=============================================================================
StatementList *
StatementList::fix(const UCS_string & data, const char * loc)
{
StatementList * fun = new StatementList(data, loc);

   Log(LOG_UserFunction__fix)
      {
        CERR << "fix pmode=statement list:" << endl << data << endl
             << " addr " << (const void *)fun << endl
             << "------------------- StatementList::fix() --" << endl;
      }

   {
     Error * err = Workspace::get_error();
     if (err)   err->parser_loc = 0;
   }

   fun->parse_body_line(Function_Line_0, data, false, false, loc, false);
   fun->setup_lambdas();

   Log(LOG_UserFunction__fix)
      {
        CERR << "fun->body.size() is " << fun->body.size() << endl;
      }

   fun->body.append(Token(TOK_RETURN_STATS), LOC);

   Log(LOG_UserFunction__fix)   fun->print(CERR);
   return fun;
}
//=============================================================================
