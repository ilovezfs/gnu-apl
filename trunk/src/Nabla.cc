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


#include "Avec.hh"
#include "Command.hh"
#include "InputFile.hh"
#include "LineInput.hh"
#include "Logging.hh"
#include "main.hh"
#include "Nabla.hh"
#include "Output.hh"
#include "PrintOperator.hh"
#include "Symbol.hh"
#include "UserFunction.hh"
#include "UserPreferences.hh"
#include "Workspace.hh"

#define NABLA_ERROR throw_edit_error(LOC)

//-----------------------------------------------------------------------------
void
Nabla::edit_function(const UCS_string & cmd)
{
Nabla nabla(cmd);
   nabla.edit();
}
//-----------------------------------------------------------------------------
Nabla::Nabla(const UCS_string & cmd)
   : defn_line_no(InputFile::current_line_no()),
     fun_symbol(0),
     ecmd(ECMD_NOP),
     edit_from(-1),
     edit_to(-1),
     function_existed(false),
     modified(false),
     do_close(false),
     locked(false),
     current_line(1),
     first_command(cmd)
{
   Workspace::more_error().clear();
}
//-----------------------------------------------------------------------------
void
Nabla::throw_edit_error(const char * why)
{
   COUT << "DEFN ERROR+" << endl
        << "      " << first_command << endl
        << "      " << UCS_string(first_command.size() - 1, UNI_ASCII_SPACE)
        << "^" << endl;

   if (Workspace::more_error().size() == 0)
      {
        Workspace::more_error() = UCS_string(why);
      }

   throw_define_error(fun_header, first_command, why);
}
//-----------------------------------------------------------------------------
void
Nabla::edit()
{
const char * error = start();
   if (error)
      {
        Log(LOG_verbose_error)   if (Workspace::more_error().size() == 0)
           {
             UERR << "Bad ∇-open '" << first_command
                  << "' : '" << error << "'" << endl;
           }
        throw_edit_error(error);
      }

   // editor loop
   //
int control_D_count = 0;
   Log(LOG_nabla)   UERR << "Nabla(" << fun_header << ")..." << endl;
   while (!do_close)
       {
         const UCS_string prompt = current_line.print_prompt(0);
         bool eof = false;
         UCS_string line;
         if (uprefs.raw_cin)
            {
              LineHistory lh(10);
              InputMux::get_line(LIM_Nabla, prompt, line, eof, lh);
            }
         else
            {
              LineHistory lh(*this);
              InputMux::get_line(LIM_Nabla, prompt, line, eof, lh);
            }

         if (eof)   // end-of-input (^D) pressed
            {
              ++control_D_count;
              if (control_D_count < 5)
                 {
                    COUT << "^D" << endl;
                    continue;
                 }
               COUT << endl << "      *** end of input" << endl;
               Command::cmd_OFF(2);
            }

         if (const char * loc = parse_oper(line, false))
            {
              UERR << "??? " << loc << endl;
              continue;
            }

         if (const char * loc = execute_oper())
            {
              UERR << "∇-command failed: " << loc << endl;
              Output::set_color_mode(Output::COLM_INPUT);
            }
       }

   Log(LOG_nabla)
      UERR << "done '" << fun_header << "'" << endl;

UCS_string fun_text;
   loop(l, lines.size())
      {
        fun_text.append(lines[l].text);
        fun_text.append(UNI_ASCII_LF);
      }

   // maybe copy function into the history
   //
   if (!uprefs.raw_cin && !InputFile::running_script() &&
       ((uprefs.nabla_to_history ==  /* always     */ 2) ||
        ((uprefs.nabla_to_history == /* if changed */ 1) && modified)))
      {
        // create a history entry that can be re-entered and replace
        // the last history line (which contained some ∇foo ...)
        //
        {
          UCS_string line_0("    ∇");
          line_0.append(lines[0].text);
          LineInput::replace_history_line(line_0);

        }

        for (int l = 1; l < lines.size(); ++l)
            {
              UCS_string line_l("[");
              line_l.append_number(l);
              line_l.append_utf8("]  ");
              while (line_l.size() < 6)   line_l.append(UNI_ASCII_SPACE);
              line_l.append(lines[l].text);

             LineInput::add_history_line(line_l);
           }

        LineInput::add_history_line(UCS_string("    ∇"));
      }

int error_line = 0;
UCS_string creator(InputFile::current_filename());
   creator.append(UNI_ASCII_COLON);
   creator.append_number(defn_line_no);
UTF8_string creator_utf8(creator);

UserFunction * ufun = UserFunction::fix(fun_text, error_line, false,
                                        LOC, creator_utf8);
   if (ufun == 0)   NABLA_ERROR;

   if (locked)
      {
         const int exec_properties[4] = { 1, 1, 1, 1 };
         ufun->set_exec_properties(exec_properties);
      }

   // set stop and trace vectors
   //
DynArray(Function_Line, stop_vec,  lines.size());
DynArray(Function_Line, trace_vec, lines.size());
int stop_count = 0;
int trace_count = 0;
   loop(l, lines.size())
       {
         if (lines[l].stop_flag)    stop_vec [stop_count++]  = Function_Line(l);
         if (lines[l].trace_flag)   trace_vec[trace_count++] = Function_Line(l);
       }

   ufun->set_trace_stop(stop_vec,  stop_count,  true);
   ufun->set_trace_stop(trace_vec, trace_count, false);
}
//-----------------------------------------------------------------------------
const char *
Nabla::start()
{
   // cmd should be something like:
   //
   // ∇FUN
   // ∇FUN[⎕]
   // ∇FUN[⎕]∇
   // etc.
   //
UCS_string::iterator c(first_command.begin());

   // skip leading spaces
   //
   while (c.more() && c.get() <= UNI_ASCII_SPACE)   c.next();

   // skip leading nabla.
   //
   if (c.next() != UNI_NABLA)   return "Bad ∇-command (no ∇)";

   // skip leading spaces
   //
   while (c.more() && c.get() <= UNI_ASCII_SPACE)   c.next();

   // function header.
   //
   while (c.more() && c.get() != UNI_ASCII_L_BRACK)
         fun_header.append(c.next());

   // at this point there could be an axis specification [X] that
   // could be confused with an operation like [⎕]. We take the first char
   // after the [ to decide if there is an axis or an operation.
   //
   if (c.get() == UNI_ASCII_L_BRACK)   // [
      {
        for (int off = 1; ; ++off)
            {
              if (c.get(off) <= UNI_ASCII_SPACE)   continue; 
              if (Avec::is_first_symbol_char(c.get(off)))   // axis
                 {
                   fun_header.append(c.next());   //  copy the [
                   while (c.more() && c.get() != UNI_ASCII_L_BRACK)
                         fun_header.append(c.next());
                 }
              break;
            }
      }

UserFunction_header hdr(fun_header);
   if (hdr.get_error())   return "Bad function header";
   fun_symbol = hdr.FUN();
   Assert(fun_symbol);

   if (fun_header.size() == 0)   return "no function name";

   // optional operation
   //
   if (c.get() == UNI_ASCII_L_BRACK)
      {
        UCS_string oper;
        Unicode cc;
        do 
          {
            if (!c.more())   return "no ] in ∇-command";
            oper.append(cc = c.next());
          } while (cc != UNI_ASCII_R_BRACK);

        const char * loc = parse_oper(oper, true);
        if (loc)   return loc;   // error
      }

   switch(fun_symbol->get_nc())
      {
        case NC_UNUSED_USER_NAME:   // open a new function
             function_existed = false;
             modified = true;
             {
               // a new function must not have a command
               //
               if (ecmd != ECMD_NOP)   return "∇-command in new function";

               const char * open_loc = open_new_function();
               if (open_loc)   return open_loc;
             }
             break;

        case NC_FUNCTION:
        case NC_OPERATOR:   // open an existing function
             function_existed = true;
             if (InputFile::running_script())   // script
                {
                  const char * open_loc = open_new_function();
                  if (open_loc)   return open_loc;
                  break;   // continue below
                }

             // interactive
             {
               if (hdr.has_vars())
                  {
                       // an existing function was opened with a header that
                       // contains more than the function name.
                       //
                       return "attempt to ∇-open existing function with "
                              "new function header";
                  }

               const char * open_loc = open_existing_function();
               if (open_loc)   return open_loc;
             }
             break;

        default:
             return "attempt to ∇-open a variable at " LOC;
      }

   // at this point the (new or existing) function was successfully opened.
   // That means that at least the header is present (lines.size() > 0)

   // immediate close (only show command is allowed here),
   // e.g. ∇fun[⎕]∇
   //
   if (c.get() == UNI_NABLA)
      {
        if (ecmd != ECMD_SHOW)   return "illegal command between ∇ ... ∇";
        if (const char * loc = execute_oper())
           UERR << "execute_oper() failed at " << loc << endl;
        do_close = true;
        return 0;
      }

   if (const char * loc = execute_oper())
      UERR << "execute_oper() failed at " << loc << endl;

   return 0;   // no error
}
//-----------------------------------------------------------------------------
const char *
Nabla::parse_oper(UCS_string & oper, bool initial)
{
   Log(LOG_nabla)
      UERR << "parsing oper '" << oper << "'" << endl;

   // skip trailing spaces
   //
   while (oper.size() > 0 && oper.last() < ' ')   oper.pop();
   if (oper.size() > 0 && oper.last() == UNI_NABLA)
      {
        do_close = true;
        oper.pop();
        while (oper.size() > 0 && oper.last() < ' ')   oper.pop();
      }
   else if (oper.size() > 0 && oper.last() == UNI_DEL_TILDE)
      {
        do_close = true;
        locked = true;
        oper.pop();
        while (oper.size() > 0 && oper.last() < ' ')   oper.pop();
      }

   current_text.clear();
   ecmd = ECMD_NOP;

   if (oper.size() == 0 && do_close)   return 0;

UCS_string::iterator c(oper.begin());
Unicode cc = c.next();
   while (cc == ' ')   cc = c.next();

   // we expect one of the following:
   //
   // [⎕] [n⎕] [⎕m] [n⎕m] [⎕n-m]                    (show)
   //     [n∆] [∆m] [n∆m] [∆n-m] [∆n1 n2 ...]       (delete)
   // [→]                                           (escape)
   // [n]                                           (goto)
   // text                                          (override text)

   if (cc != UNI_ASCII_L_BRACK)
      {
        ecmd = ECMD_EDIT;
        edit_from = current_line;
        for (; cc != Invalid_Unicode; cc = c.next())   current_text.append(cc);
        return 0;
      }

   // a loop over multiple commands, like
   // [2⎕4] [∆5]
   //
   // only the last command is executed; the previous commands are discarded/
   //
command_loop:

   // at this point, [ was seen and skipped

   ecmd = ECMD_NOP;
   edit_from.clear();
   edit_to.clear();
   got_minus = false;

   // optional edit_from
   //
   if (Avec::is_digit(c.get()) || c.get() == UNI_ASCII_FULLSTOP)
      edit_from = parse_lineno(c);

   // operation, which is one of:
   //
   // [⎕   show
   // []   edit
   // [∆   delete
   // [→   abandon
   switch (c.get())
      {
        case UNI_Quad_Quad:
        case UNI_Quad_Quad1:    ecmd = ECMD_SHOW;     c.next();   break;
        case UNI_ASCII_R_BRACK: ecmd = ECMD_EDIT;                 break;
        case UNI_DELTA:         ecmd = ECMD_DELETE;   c.next();   break;
        case UNI_RIGHT_ARROW:   ecmd = ECMD_ESCAPE;   c.next();   break;
        case Invalid_Unicode:   return "Bad ∇-command";

        default: UERR << "Bad edit op '" << c.get() << "'" << endl;
                 return "Bad ∇-command";
      }

   // don't allow delete or escape in the first command
   if (initial)
      {
        if (ecmd == ECMD_DELETE)   return "Bad initial ∇-command ∆";
        if (ecmd == ECMD_ESCAPE)   return "Bad initial ∇-command →";
      }

again:
   // optional edit_to
   //
   if (Avec::is_digit(c.get()))   edit_to = parse_lineno(c);

   if (c.get() == UNI_ASCII_MINUS)   // range
      {
        if (got_minus)   return "error: second -  in ∇-range";
        got_minus = true;
        edit_from = edit_to;
        c.next();   // eat the -
        goto again;
      }

   if (c.next() != UNI_ASCII_R_BRACK)   return "missing ] in ∇-range";

   // at this point we have parsed an editor command, like:
   //
   // [from ⎕ to]
   // [from ∆ to]
   // [from]

   while (c.get() <= UNI_ASCII_SPACE)   c.next();

   if (c.get() == UNI_ASCII_L_BRACK)   // another command: ignore previous
      {
         c.next();   // eat the [
         goto command_loop;
      }

   // copy the rest to current_text. Set do_close if ∇ or ⍫ is seen
   // unless inside strings.
   //
   for (;;)
      {
        switch(cc = c.next())
           {
             case Invalid_Unicode:     // regular end of input
                  return 0;

             case UNI_NABLA:           // ∇
                  do_close = true;
                  return 0;

             case UNI_DEL_TILDE:       // ⍫
                  locked = true;
                  do_close = true;
                  return 0;

             case UNI_ASCII_DOUBLE_QUOTE:  // "
                  current_text.append(cc);
                  for (;;)
                      {
                        cc = c.next();
                        if (cc == Invalid_Unicode)   // premature end of input
                           {
                             current_text.append(UNI_ASCII_DOUBLE_QUOTE);
                             return 0;
                           }

                        current_text.append(cc);
                        if (cc == UNI_ASCII_DOUBLE_QUOTE)   break; // string end
                        if (cc == UNI_ASCII_BACKSLASH)      // \x
                           {
                             cc = c.next();
                             if (cc == Invalid_Unicode)   // premature input end
                                {
                                  current_text.append(UNI_ASCII_BACKSLASH);
                                  current_text.append(UNI_ASCII_DOUBLE_QUOTE);
                                  return 0;
                                }
                             current_text.append(cc);
                           }
                      }
                  break;

             case UNI_SINGLE_QUOTE:    // '
                  current_text.append(cc);
                  for (;;)
                      {
                        // no need to care for ''. Since we only copy, we can
                        // handle ' ... '' ... ' like two adjacent strings
                        // instead of a string containing a (doubled) quote.
                        //
                        cc = c.next();
                        if (cc == Invalid_Unicode)   // premature end of input
                           {
                             current_text.append(UNI_SINGLE_QUOTE);
                             return 0;
                           }

                        current_text.append(cc);
                        if (cc == UNI_SINGLE_QUOTE)      break;   // string end
                      }
                  break;

             default:
                current_text.append(cc);
           }
      }

   return 0;
}
//-----------------------------------------------------------------------------
LineLabel
Nabla::parse_lineno(UCS_string::iterator & c)
{
LineLabel ret(0);

   while (Avec::is_digit(c.get()))
      {
        ret.ln_major *= 10;
        ret.ln_major += c.next() - UNI_ASCII_0;
      }

   if (c.get() == UNI_ASCII_FULLSTOP)
      {
        c.next();   // eat the .
        while (Avec::is_digit(c.get()))   ret.ln_minor.append(c.next());
      }

   return ret;
}
//-----------------------------------------------------------------------------
const char *
Nabla::open_new_function()
{
   Log(LOG_nabla)
      UERR << "creating new function '" << fun_symbol->get_name() 
           << "' with header '" << fun_header << "'" << endl;

   lines.push_back(FunLine(0, fun_header));
   return 0;
}
//-----------------------------------------------------------------------------
const char *
Nabla::open_existing_function()
{
   Log(LOG_nabla)
      UERR << "opening existing function '" << fun_symbol->get_name()
           << "'" << endl;

   // this function must only be called when editing functions interactively
   //
   Assert(!InputFile::running_script());

Function * function = fun_symbol->get_function();
   Assert(function);

   if (function->get_exec_properties()[0])
      return "function is locked";

   if (function->is_native())
      return "function is native";

   if (Workspace::is_called(fun_symbol->get_name()))
      return "function is pendent or suspended";

const UserFunction * ufun = function->get_ufun1();
   if (ufun == 0)
      return "function is not editable at " LOC;

const UCS_string ftxt = function->canonical(false);
   Log(LOG_nabla)   UERR << "existing function is:\n" << ftxt << endl;

vector<UCS_string> tlines;
   ftxt.to_vector(tlines);

   Assert(tlines.size());
   fun_header = tlines[0];
   loop(t, tlines.size())
       {
         FunLine fl(t, tlines[t]);

         // set stop and trace flags
         //
         loop(st, ufun->get_stop_lines().size())
             {
               if (t == ufun->get_stop_lines()[st])   // stop set
                  {
                    fl.stop_flag = true;
                    break;
                  }
             }

         loop(tr, ufun->get_trace_lines().size())
             {
               if (t == ufun->get_trace_lines()[tr])   // trace set
                  {
                    fl.trace_flag = true;
                    break;
                  }
             }

         lines.push_back(fl);
       }

   current_line = LineLabel(tlines.size());

   return 0;
}
//-----------------------------------------------------------------------------
const char *
Nabla::execute_oper()
{
   if (ecmd == ECMD_NOP)
      {
        Log(LOG_nabla)
           UERR << "Nabla::execute_oper(NOP)" << endl;
        return 0;
      }

const bool have_from = edit_from.ln_major != -1;
const bool have_to = edit_to.ln_major != -1;

   if (lines.size())
      {
        if (!have_from && ecmd != ECMD_DELETE)   edit_from = lines[0].label;
        if (!have_to)     edit_to = lines[lines.size() - 1].label;
      }

   if (ecmd == ECMD_SHOW)     return execute_show();
   if (ecmd == ECMD_DELETE)   return execute_delete();
   if (ecmd == ECMD_EDIT)     return execute_edit();
   if (ecmd == ECMD_ESCAPE)   return execute_escape();

   UERR << "edit command " << ecmd
        << " from " << edit_from << " to " << edit_to << endl;
   FIXME;

   return LOC;
}
//-----------------------------------------------------------------------------
const char *
Nabla::execute_show()
{
   Log(LOG_nabla)
      UERR << "Nabla::execute_oper(SHOW) from " << edit_from
           << " to " << edit_to << " line-count " << lines.size() << endl;

int idx_from = find_line(edit_from);
int idx_to   = find_line(edit_to);

const LineLabel user_edit_to = edit_to;

   if (idx_from == -1)   edit_from = lines[idx_from = 0].label;
   if (idx_to == -1)     edit_to   = lines[idx_to = lines.size() - 1].label;

   Log(LOG_nabla)
      UERR << "Nabla::execute_oper(SHOW) from "
           << edit_from << " to " << edit_to << endl;

   if (idx_from == 0)   COUT << "    ∇" << endl;               // if header line
   for (int e = idx_from; e <= idx_to; ++e)   lines[e].print(COUT);
   if (idx_to == lines.size() - 1)   COUT << "    ∇" << endl;  // if last line

   if (user_edit_to.valid())   // eg. [⎕42] or [2⎕42]
      {
        current_line = user_edit_to;
        if (line_exists(current_line))   current_line.next();
        if (line_exists(current_line))
           {
             // user_edit_to and its next line exist. Increase minor length
             // That is, if both [8] and [9] exist then use [8.1]
             //
             current_line = user_edit_to;
             current_line.insert();
           }
      }
   else                      // eg. [42⎕] or [⎕]
      {
        current_line = lines.back().label;
        current_line.next();
      }

   Log(LOG_nabla)
      UERR << "Nabla::execute_oper(SHOW) done with current line "
           << current_line << endl;

   return 0;
}
//-----------------------------------------------------------------------------
const char *
Nabla::execute_delete()
{
   // for delete we want exact numbers.
   //
const int idx_to = find_line(LineLabel(edit_to));
   if (idx_to == -1)   return "Bad line number N in [M∆N] ";

   modified = true;

   if (edit_from == -1)   // delete single line
      {
        lines.erase(lines.begin() + idx_to);
        return 0;
      }

   // delete multiple lines
   //
const int idx_from = find_line(LineLabel(edit_from));
   if (idx_from == -1)       return "Bad line number M in [M∆N] ";
   if (idx_from >= idx_to)   return "M ≥ N in [M∆N] ";

   lines.erase(lines.begin() + idx_from, lines.begin() + idx_to);
   current_line = lines.back().label;
   return 0;
}
//-----------------------------------------------------------------------------
const char *
Nabla::execute_edit()
{
   // if the user has not specified a line then edit at current_line
   //
   if (edit_from.ln_major == -1)   edit_from = current_line;

   current_line = edit_from;

   // if the user has specified a line label without a text then we are done
   //
   if (current_text.size() == 0)   return 0;

   // check that current_text is valid
   //
   if (current_line.is_header_line_number())   return edit_header_line();
   else                                        return edit_body_line();
}
//-----------------------------------------------------------------------------
const char *
Nabla::edit_header_line()
{
   // parse the header and check that it is valid
   //
UserFunction_header header(current_text);
   if (header.get_error() != E_NO_ERROR)
      {
        CERR << "BAD FUNCTION HEADER";
        COUT << endl;
        return 0;
      }

   // check if the function name has changed
   //
   Assert(fun_symbol);
   Assert(header.FUN());
const UCS_string & old_name = fun_symbol->get_name();
const UCS_string & new_name = header.FUN()->get_name();
   if (old_name != new_name)
      {
        // the name has changed. This is OK if the new name can be edited.
        //
        // the old symbol shall not be ⎕FXed when closing the editor, so we
        // can simply forget it and continue with the new function
        //
        // UserFunction_header::UserFunction_header() should have triggered
        // the creation of header.FUN() so we can assume that it exists. We
        // need to check that it is not a variable or an existing function.
        //
        if (header.FUN()->get_nc() != NC_UNUSED_USER_NAME)
           {
             CERR << "BAD FUNCTION HEADER";
             COUT << endl;
             return 0;
           }
      }

   modified = true;

   lines[0].text = current_text;
   current_line.next();
   return 0;
}
//-----------------------------------------------------------------------------
const char *
Nabla::edit_body_line()
{
const Parser parser(PM_FUNCTION, LOC);
Token_string in;
ErrorCode ec = parser.parse(current_text, in);
   if (ec)
      {
        CERR << "SYNTAX ERROR";
        COUT << endl;
        return 0;
      }

   modified = true;

const int idx_from = find_line(edit_from);

   Assert(lines.size() > 0);
   if (idx_from == -1)   // new line
      {
        // find the largest label before edit_from (if any)
        //
        int before_idx = -1;
        for (int i = 0; i < lines.size(); ++i)
            {
              if (lines[i].label < edit_from)   before_idx = i;
              else                                 break;
            }

        FunLine fl(edit_from, current_text);
        lines.insert(lines.begin() + before_idx + 1, fl);
      }
   else
      {
        // replace line
        //
        lines[idx_from].text = current_text;
      }

   current_line.next();
   return 0;
}
//-----------------------------------------------------------------------------
const char *
Nabla::execute_escape()
{
   lines.clear();
   lines.push_back(FunLine(0, fun_header));
   return 0;
}
//-----------------------------------------------------------------------------
int
Nabla::find_line(const LineLabel & lab) const
{
   if (lab.ln_major == -1)   return -1;

   loop(l, lines.size())   if (lab == lines[l].label)   return l;

   return -1;   // not found.
}
//-----------------------------------------------------------------------------
void
Nabla::FunLine::print(ostream & out) const
{
   label.print(out);

   // print a space unless text is a label or a comment
   //
   if (!text.is_comment_or_label())   out << " ";
   out << text << endl;
}
//-----------------------------------------------------------------------------
void
LineLabel::print(ostream & out) const
{
UCS_string ucs("[");
   ucs.append_number(ln_major);
   if (ln_minor.size())
      {
        ucs.append_utf8(".");
        ucs.append(ln_minor);
      }
   ucs.append_utf8("]");

   while (ucs.size() < 5)   ucs.append_utf8(" ");
   out << ucs;
}
//-----------------------------------------------------------------------------
UCS_string
LineLabel::print_prompt(int min_size) const
{
UCS_string ret("[");
   ret.append_number(ln_major);

   if (ln_minor.size())
      {
        ret.append(Unicode('.'));
        loop(s, ln_minor.size())   ret.append(Unicode(char(ln_minor[s])));
      }

   ret.append_utf8("] ");
   while (ret.size() < min_size)   ret.append(UNI_ASCII_SPACE);
   return ret;
}
//-----------------------------------------------------------------------------
void
LineLabel::next()
{
   if (ln_minor.size() == 0)   // full number: add 1
      {
        ++ln_major;
        return;
      }

   // fract number: increment last fract digit
   //
const Unicode cc = ln_minor[ln_minor.size() - 1];
   if (cc != UNI_ASCII_9)   ln_minor[ln_minor.size() - 1] = Unicode(cc + 1);
   else                     ln_minor.append(UNI_ASCII_1);
}
//-----------------------------------------------------------------------------
void
LineLabel::insert()
{
   ln_minor.append(UNI_ASCII_1);
}
//-----------------------------------------------------------------------------
bool
LineLabel::operator ==(const LineLabel & other) const
{
   return (ln_major == other.ln_major) && (ln_minor == other.ln_minor);
}
//-----------------------------------------------------------------------------
bool
LineLabel::operator <(const LineLabel & other) const
{
   if (ln_major != other.ln_major)   return ln_major < other.ln_major;
 return  ln_minor.compare(other.ln_minor) < 0;
}
//-----------------------------------------------------------------------------
ostream &
operator <<(ostream & out, const LineLabel & lab)
{
   lab.print(out);
   return out;
}
//-----------------------------------------------------------------------------
UCS_string
Nabla::FunLine::get_label_and_text() const
{
UCS_string ret = label.print_prompt(6);
   ret.append(text);

   return ret;
}
//-----------------------------------------------------------------------------
UCS_string
Nabla::get_label_and_text(int line, bool & is_current) const
{
const FunLine & fl = lines[line];
   is_current = fl.label == current_line;
   return fl.get_label_and_text();
}
//-----------------------------------------------------------------------------

