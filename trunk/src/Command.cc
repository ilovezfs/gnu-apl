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

#include <dirent.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <string.h>

#include "CharCell.hh"
#include "ComplexCell.hh"
#include "Command.hh"
#include "Executable.hh"
#include "FloatCell.hh"
#include "IndexExpr.hh"
#include "IntCell.hh"
#include "IO_Files.hh"
#include "LineInput.hh"
#include "Nabla.hh"
#include "Output.hh"
#include "Parser.hh"
#include "Prefix.hh"
#include "Quad_TF.hh"
#include "StateIndicator.hh"
#include "Svar_DB.hh"
#include "Symbol.hh"
#include "Tokenizer.hh"
#include "UserFunction.hh"
#include "UserPreferences.hh"
#include "Value.icc"
#include "Workspace.hh"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

int Command::boxing_format = -1;

vector<Command::user_command> Command::user_commands;

//-----------------------------------------------------------------------------
void
Command::process_line()
{
UCS_string line;
bool eof = false;
   InputMux::get_line(LIM_ImmediateExecution, Workspace::get_prompt(),
                      line, eof, LineInput::get_history());
   if (eof)
      {
        CERR << "EOF at " << LOC << endl;
        return;
      }

   process_line(line);
}
//-----------------------------------------------------------------------------
void
Command::process_line(UCS_string & line)
{
   line.remove_leading_whitespaces();
   if (line.size() == 0)   return;   // empty input line

   switch(line[0])
      {
         case UNI_ASCII_R_PARENT:      // regular command, e.g. )SI
              do_APL_command(COUT, line);
              if (line.size())   break;
              return;

         case UNI_ASCII_R_BRACK:       // debug command, e.g. ]LOG
              do_APL_command(CERR, line);
              if (line.size())   break;
              return;

         case UNI_NABLA:               // e.g. ∇FUN
              Nabla::edit_function(line);
              return;

         case UNI_ASCII_NUMBER_SIGN:   // e.g. # comment
         case UNI_COMMENT:             // e.g. ⍝ comment
              return;

        default: ;
      }

   do_APL_expression(line);
}
//-----------------------------------------------------------------------------
bool
Command::do_APL_command(ostream & out, UCS_string & line)
{
const UCS_string line1(line);   // the original line

   // split line into command and arguments
   //
UCS_string cmd;   // the command without arguments
int len = 0;
   line.copy_black(cmd, len);

UCS_string arg(line, len, line.size() - len);
vector<UCS_string> args;
   arg.copy_black_list(args);
   line.clear();
   if (!cmd.starts_iwith(")MORE")) 
      {
        // clear )MORE info unless command is )MORE
        //
        Workspace::more_error().clear();
      }

#define cmd_def(cmd_str, code, _arg, _hint) \
   if (cmd.starts_iwith(cmd_str)) { code; return true; }
#include "Command.def"

   // check for user defined commands...
   //
   loop(u, user_commands.size())
       {
         if (cmd.starts_iwith(user_commands[u].prefix))
            {
              do_USERCMD(out, line, line1, cmd, args, u);
              return true;
            }
       }

     out << "BAD COMMAND" << endl;
     return false;
}
//-----------------------------------------------------------------------------
void
Command::do_APL_expression(UCS_string & line)
{
   Workspace::more_error().clear();

Executable * statements = 0;
   try
      {
        statements = StatementList::fix(line, LOC);
      }
   catch (Error err)
      {
        COUT << Error::error_name(err.error_code) << endl;
        if (err.get_error_line_2().size())
           {
             COUT << "      " << err.get_error_line_2() << endl
                  << "      " << err.get_error_line_3() << endl;
           }

        err.print(CERR);
        return;
      }
   catch (...)
      {
        CERR << "*** Command::process_line() caught other exception ***"
             << endl;
        return;
      }

   if (statements == 0)
      {
        COUT << "main: Parse error." << endl;
        return;
      }

   // At this point, the user command was parsed correctly.
   // check for Escape (→)
   //
   {
     const Token_string & body = statements->get_body();
     if (body.size() == 3                &&
         body[0].get_tag() == TOK_ESCAPE &&
         body[1].get_Class() == TC_END   &&
         body[2].get_tag() == TOK_RETURN_STATS)
        {
          delete statements;

          // remove all SI entries up to (including) the next immediate
          // execution context
          //
          for (bool goon = true; goon;)
              {
                StateIndicator * si = Workspace::SI_top();
                if (si == 0)   break;   // SI empty

                const Executable * exec = si->get_executable();
                Assert(exec);
                goon = exec->get_parse_mode() != PM_STATEMENT_LIST;
                si->escape();   // pop local vars of user defined functions
                Workspace::pop_SI(LOC);
              }
          return;
        }
   }

// statements->print(CERR);

   // push a new context for the statements.
   //
   Workspace::push_SI(statements, LOC);

   for (;;)
       {
         //
         // NOTE that the entire SI may change while executing this loop.
         // We should therefore avoid references to SI entries.
         //
         Token token = Workspace::SI_top()->get_executable()->execute_body();

// Q(token)

         // start over if execution has pushed a new SI entry
         //
         if (token.get_tag() == TOK_SI_PUSHED)   continue;

         // maybe call EOC handler and repeat if true returned
         //
       check_EOC:
         if (Workspace::SI_top()->call_eoc_handler(token))   continue;

         // the far most frequent cases are TC_VALUE and TOK_VOID
         // so we handle them first.
         //
         if (token.get_Class() == TC_VALUE || token.get_tag() == TOK_VOID )
            {
              if (Workspace::SI_top()->get_executable()
                             ->get_parse_mode() == PM_STATEMENT_LIST)
                 {
                   if (attention_raised)
                      {
                        attention_raised = false;
                        interrupt_raised = false;
                        ATTENTION;
                      }

                   break;   // will return to calling context
                 }

              Workspace::pop_SI(LOC);

              // we are back in the calling SI. There should be a TOK_SI_PUSHED
              // token at the top of stack. Replace it with the result from
              //  the called (just poped) SI.
              //
              {
                Prefix & prefix =
                         Workspace::SI_top()->get_prefix();
                Assert(prefix.at0().get_tag() == TOK_SI_PUSHED);

                copy_1(prefix.tos().tok, token, LOC);
              }
              if (attention_raised)
                 {
                   attention_raised = false;
                   interrupt_raised = false;
                   ATTENTION;
                 }

              continue;
            }

         if (token.get_tag() == TOK_BRANCH)
            {
              StateIndicator * si = Workspace::SI_top_fun();

              if (si == 0)
                 {
                    Workspace::more_error() = UCS_string(
                          "branch back into function (→N) without "
                            "suspended function");
                    SYNTAX_ERROR;   // →N without function,
                 }

              // pop contexts above defined function
              //
              while (si != Workspace::SI_top())   Workspace::pop_SI(LOC);

              const Function_Line line = Function_Line(token.get_int_val());

              if (line == Function_Retry)   si->retry(LOC);
              else                          si->goon(line, LOC);
              continue;
            }

         if (token.get_tag() == TOK_ESCAPE)
            {
              // remove all SI entries up to (including) the next immediate
              // execution context
              //
              for (bool goon = true; goon;)
                  {
                    StateIndicator * si = Workspace::SI_top();
                    if (si == 0)   break;   // SI empty

                    const Executable * exec = si->get_executable();
                    Assert(exec);
                    goon = exec->get_parse_mode() != PM_STATEMENT_LIST;
                    si->escape();   // pop local vars of user defined functions
                    Workspace::pop_SI(LOC);
              }
              return;

              Assert(0 && "not reached");
            }

         if (token.get_tag() == TOK_ERROR)
            {
              if (token.get_int_val() == E_COMMAND_PUSHED)
                 {
                   Workspace::pop_SI(LOC);
                   UCS_string pushed_command = Workspace::get_pushed_Command();
                   process_line(pushed_command);
                   pushed_command.clear();
                   Workspace::push_Command(pushed_command);   // clear in
                   return;
                 }

              // clear attention and interrupt flags
              //
              attention_raised = false;
              interrupt_raised = false;

              // check for safe execution mode. Entries in safe execution mode
              // can be far above the current SI entry. The EOC handler will
              // unroll the SI stack.
              //
              for (StateIndicator * si = Workspace::SI_top();
                   si; si = si->get_parent())
                  {
                    if (si->get_safe_execution())
                       {
                         // pop SI entries above the entry with safe_execution
                         //
                         while (Workspace::SI_top() != si)
                            {
                              Workspace::pop_SI(LOC);
                            }

                         goto check_EOC;
                       }
                  }

              // if suspend is not allowed then pop all SI entries that
              // don't allow suspend
              //
              if (Workspace::SI_top()->get_executable()->cannot_suspend())
                 {
                    Error error = Workspace::SI_top()->get_error();
                    while (Workspace::SI_top()->get_executable()
                                              ->cannot_suspend())
                       {
                         Workspace::pop_SI(LOC);
                       }

                   if (Workspace::SI_top())
                      {
                        Workspace::SI_top()->get_error() = error;
                      }
                 }

              Workspace::get_error()->print(CERR);
              if (Workspace::SI_top()->get_level() == 0)
                 {
                   Value::erase_stale(LOC);
                   IndexExpr::erase_stale(LOC);
                 }
              return;
            }

         // we should not come here.
         //
         Q1(token)  Q1(token.get_Class())  Q1(token.get_tag())  FIXME;
       }

   // pop the context for the statements
   //
   Workspace::pop_SI(LOC);
}
//-----------------------------------------------------------------------------
void 
Command::cmd_XTERM(ostream & out, const UCS_string & arg)
{
   const char * term = getenv("TERM");
   if (!strncmp(term, "dumb", 4) && arg.starts_iwith("ON"))
      {
        out << "impossible on dumb terminal" << endl;
      }
   else if (arg.starts_iwith("OFF") || arg.starts_iwith("ON"))
      {
        Output::toggle_color(arg);
      }
   else if (arg.size() == 0)
      {
        out << "]COLOR/XTERM ";
        if (Output::color_enabled()) out << "ON"; else out << "OFF";
        out << endl;
      }
   else
      {
        out << "BAD COMMAND" << endl;
        return;
      }
}
//-----------------------------------------------------------------------------
void 
Command::cmd_BOXING(ostream & out, const UCS_string & arg)
{
int format = arg.atoi();

   if (arg.size() == 0)
      {
        
        out << "]BOXING ";
        if (boxing_format == -1) out << "OFF"; else out << boxing_format;
        out << endl;
        return;
      }

   if (arg.starts_iwith("OFF"))   format = -1;

   switch (format)
      {
        case -1:
        case  2:
        case  3:
        case  4:
        case  7:
        case  8:
        case  9: boxing_format = format;
                 return;

        default:   out << "Bad ]BOXING parameter " << arg
                       << " (valid values are: OFF, 2, 3, 4, 7, 8, and 9)"
                       << endl;
      }
}
//-----------------------------------------------------------------------------
void 
Command::cmd_CHECK(ostream & out)
{
   // erase stale functions from failed ⎕EX
   //
   {
     bool erased = false;
     int stale = Workspace::cleanup_expunged(CERR, erased);
     if (stale)
        {
          CERR << "WARNING - " << stale << " stale functions ("
               << (erased ? "" : "not ") << "erased)" << endl;
        }
     else CERR << "OK      - no stale functions" << endl;
   }

   {
     const int stale = Value::print_stale(CERR);
     if (stale)
        {
          char cc[200];
          snprintf(cc, sizeof(cc), "ERROR   - %d stale values", stale);
          out << cc << endl;
          IO_Files::apl_error(LOC);
        }
     else out << "OK      - no stale values" << endl;
   }
   {
     const int stale = IndexExpr::print_stale(CERR);
     if (stale)
        {
          char cc[200];
          snprintf(cc, sizeof(cc), "ERROR   - %d stale indices", stale);
          out << cc << endl;
          IO_Files::apl_error(LOC);
        }
     else out << "OK      - no stale indices" << endl;
   }
}
//-----------------------------------------------------------------------------
void 
Command::cmd_CONTINUE(ostream & out)
{
   Workspace::wsid(out, UCS_string("CONTINUE"));

vector<UCS_string> vcont;
   Workspace::save_WS(out, vcont);
   cmd_OFF(0);
}
//-----------------------------------------------------------------------------
void 
Command::cmd_DROP(ostream & out, const vector<UCS_string> & lib_ws)
{
   // )DROP wsname
   // )DROP libnum wsname

   // check number of arguments (1 or 2)
   //
   if (lib_ws.size() == 0)   // missing argument
      {
        out << "BAD COMMAND+" << endl;
        Workspace::more_error() = "missing workspace name in command )DROP";
        return;
      }

   if (lib_ws.size() > 2)   // too many arguments
      {
        out << "BAD COMMAND+" << endl;
        Workspace::more_error() = "too many parameters in command )DROP";
        return;
      }

   // at this point, lib_ws.size() is 1 or 2. If 2 then the first
   // is the lib number
   //
LibRef libref = LIB_NONE;
UCS_string wname = lib_ws.back();
   if (lib_ws.size() == 2)   libref = (LibRef)(lib_ws.front().atoi());

UTF8_string filename = LibPaths::get_lib_filename(libref, wname, true,
                                                  ".xml", ".apl");

int result = unlink(filename.c_str());
   if (result)
      {
        out << wname << " NOT DROPPED: " << strerror(errno) << endl;
        UTF8_ostream more;
        more << "could not unlink file " << filename;
        Workspace::more_error() = UCS_string(more.get_data());
      }
}
//-----------------------------------------------------------------------------
void
Command::cmd_ERASE(ostream & out, vector<UCS_string> & args)
{
   Workspace::erase_symbols(out, args);
}
//-----------------------------------------------------------------------------
void 
Command::cmd_KEYB(ostream & out)
{
   // maybe print user-supplied keyboard layout file
   //
   if (uprefs.keyboard_layout_file.size())
      {
        FILE * layout = fopen(uprefs.keyboard_layout_file.c_str(), "r");
        if (layout == 0)
           {
             out << "Could not open " << uprefs.keyboard_layout_file
                 << ": " << strerror(errno) << endl
                 << "Showing default layout instead" << endl;
           }
        else
           {
             out << "User-defined Keyboard Layout:\n";
             for (;;)
                 {
                    const int cc = fgetc(layout);
                    if (cc == EOF)   break;
                    out << (char)cc;
                 }
             out << endl;
             return;
           }
      }

   out << "US Keyboard Layout:\n"
                              "\n"
"╔════╦════╦════╦════╦════╦════╦════╦════╦════╦════╦════╦════╦════╦═════════╗\n"
"║ ~  ║ !⌶ ║ @⍫ ║ #⍒ ║ $⍋ ║ %⌽ ║ ^⍉ ║ &⊖ ║ *⍟ ║ (⍱ ║ )⍲ ║ _! ║ +⌹ ║         ║\n"
"║ `◊ ║ 1¨ ║ 2¯ ║ 3< ║ 4≤ ║ 5= ║ 6≥ ║ 7> ║ 8≠ ║ 9∨ ║ 0∧ ║ -× ║ =÷ ║ BACKSP  ║\n"
"╠════╩══╦═╩══╦═╩══╦═╩══╦═╩══╦═╩══╦═╩══╦═╩══╦═╩══╦═╩══╦═╩══╦═╩══╦═╩══╦══════╣\n"
"║       ║ Q  ║ W⍹ ║ E⋸ ║ R  ║ T⍨ ║ Y¥ ║ U  ║ I⍸ ║ O⍥ ║ P⍣ ║ {⍞ ║ }⍬ ║  |⊣  ║\n"
"║  TAB  ║ q? ║ w⍵ ║ e∈ ║ r⍴ ║ t∼ ║ y↑ ║ u↓ ║ i⍳ ║ o○ ║ p⋆ ║ [← ║ ]→ ║  \\⊢  ║\n"
"╠═══════╩═╦══╩═╦══╩═╦══╩═╦══╩═╦══╩═╦══╩═╦══╩═╦══╩═╦══╩═╦══╩═╦══╩═╦══╩══════╣\n"
"║ (CAPS   ║ A⍶ ║ S  ║ D  ║ F  ║ G  ║ H⍙ ║ J⍤ ║ K  ║ L⌷ ║ :≡ ║ \"≢ ║         ║\n"
"║  LOCK)  ║ a⍺ ║ s⌈ ║ d⌊ ║ f_ ║ g∇ ║ h∆ ║ j∘ ║ k' ║ l⎕ ║ ;⍎ ║ '⍕ ║ RETURN  ║\n"
"╠═════════╩═══╦╩═══╦╩═══╦╩═══╦╩═══╦╩═══╦╩═══╦╩═══╦╩═══╦╩═══╦╩═══╦╩═════════╣\n"
"║             ║ Z  ║ Xχ ║ C¢ ║ V  ║ B£ ║ N  ║ M  ║ <⍪ ║ >⍙ ║ ?⍠ ║          ║\n"
"║  SHIFT      ║ z⊂ ║ x⊃ ║ c∩ ║ v∪ ║ b⊥ ║ n⊤ ║ m| ║ ,⍝ ║ .⍀ ║ /⌿ ║  SHIFT   ║\n"
"╚═════════════╩════╩════╩════╩════╩════╩════╩════╩════╩════╩════╩══════════╝\n"
   << endl;
}
//-----------------------------------------------------------------------------
void 
Command::cmd_PSTAT(ostream & out, const UCS_string & arg)
{
#ifndef PERFORMANCE_COUNTERS_WANTED
   out << "\n"
<< "Command ]PSTAT is not available, since performance counters were not\n"
"configured for this APL interpreter. To enable performance counters (which\n"
"will slightly decrease performance), recompile the interpreter as follows:"

<< "\n\n"
"   ./configure PERFORMANCE_COUNTERS_WANTED=yes (... "
<< "other configure options"
<< ")\n"
"   make\n"
"   make install (or try: src/apl)\n"
"\n"

<< "above the src directory."
<< "\n";

   return;
#endif

   if (arg.starts_iwith("CLEAR"))
      {
        out << "Performance counters cleared" << endl;
        Performance::reset_all();
        return;
      }

   if (arg.starts_iwith("SAVE"))
      {
        const char * filename = "./PerformanceData.def";
        ofstream outf(filename, ofstream::out);
        if (!outf.is_open())
           {
             out << "opening " << filename
                 << " failed: " << strerror(errno) << endl;
             return;
           }

        out << "Writing performance data to file " << filename << endl;
        Performance::save_data(out, outf);
        return;
      }

Pfstat_ID iarg = PFS_ALL;
   if (arg.size() > 0)   iarg = (Pfstat_ID)(arg.atoi());

   Performance::print(iarg, out);
}
//-----------------------------------------------------------------------------
void 
Command::cmd_HELP(ostream & out)
{
   out << "APL Commands:" << endl;
#define cmd_def(cmd_str, _cod, arg, _hint) \
   out << "      " cmd_str " " arg << endl;
#include "Command.def"

  if (user_commands.size())
     {
       out << endl << "User defined commands:" << endl;
       loop(u, user_commands.size())
           {
             out << "      " << user_commands[u].prefix << " [args]  calls:  ";
             if (user_commands[u].mode)   out << "tokenized-args ";
 
             out << user_commands[u].apl_function << " quoted-args" << endl;
           }
     }

   out << endl << "System variables:" << endl;
#define ro_sv_def(x, txt)                                           \
   { const UCS_string & ucs = Workspace::get_v_ ## x().get_name();  \
        out << "      " << setw(8) << ucs << #txt << endl; }
#define rw_sv_def(x, txt)                                           \
   { const UCS_string & ucs = Workspace::get_v_ ## x().get_name();  \
        out << "      " << setw(8) << ucs << #txt << endl; }
#include "SystemVariable.def"

   out << endl << "System functions:" << endl;
#define ro_sv_def(x, txt)
#define rw_sv_def(x, txt)
#define sf_def(q, txt) { const char * qu = #q;                      \
   if (!strncmp(qu, "Quad_", 5))                                    \
        out << "      ⎕" << setw(8) << UCS_string(qu + 5) << #txt << endl; }
#include "SystemVariable.def"
}
//-----------------------------------------------------------------------------
void 
Command::cmd_HISTORY(ostream & out, const UCS_string & arg)
{
   if (arg.size() == 0)                  LineInput::print_history(out);
   else if (arg.starts_iwith("CLEAR"))   LineInput::clear_history(out);
   else                                  out << "BAD COMMAND" << endl;
}
//-----------------------------------------------------------------------------
void
Command::cmd_HOST(ostream & out, const UCS_string & arg)
{
   if (uprefs.safe_mode)
      {
        out << ")HOST command not allowed in safe mode." << endl;
        return;
      }

UTF8_string host_cmd(arg);
FILE * pipe = popen(host_cmd.c_str(), "r");
   if (pipe == 0)   // popen failed
      {
        out << ")HOST command failed: " << strerror(errno) << endl;
        return;
      }

   for (;;)
       {
         const int cc = fgetc(pipe);
         if (cc == EOF)   break;
         out << (char)cc;
       }

int result = pclose(pipe);
   out << endl << IntCell(result) << endl;
}
//-----------------------------------------------------------------------------
void
Command::cmd_IN(ostream & out, vector<UCS_string> & args, bool protection)
{
   if (args.size() == 0)
      {
        out << "BAD COMMAND" << endl;
        Workspace::more_error() =
                   UCS_string("missing filename in command )IN");
        return;
      }

UCS_string fname = args.front();
   args.erase(args.begin());

UTF8_string filename = LibPaths::get_lib_filename(LIB_NONE, fname, true,
                                                  ".atf", 0);

FILE * in = fopen(filename.c_str(), "r");
   if (in == 0)   // open failed: try filename.atf unless already .atf
      {
        UTF8_string fname_utf8(fname);
        CERR << ")IN " << fname_utf8.c_str()
             << " failed: " << strerror(errno) << endl;

        char cc[200];
        snprintf(cc, sizeof(cc),
                 "command )IN: could not open file %s for reading: %s",
                 fname_utf8.c_str(), strerror(errno));
        Workspace::more_error() = UCS_string(cc);
        return;
      }

UTF8 buffer[80];
int idx = 0;

transfer_context tctx(protection);

   for (;;)
      {
        const int cc = fgetc(in);
        if (cc == EOF)   break;
        if (idx == 0 && cc == 0x0A)   // optional LF
           {
             // CERR << "CRLF" << endl;
             continue;
           }

        if (idx < 80)
           {
              if (idx < 72)   buffer[idx++] = cc;
              else            buffer[idx++] = 0;
             continue;
           }

        if (cc == 0x0D || cc == 0x15)   // ASCII or EBCDIC
           {
             tctx.is_ebcdic = (cc == 0x15);
             tctx.process_record(buffer, args);

             idx = 0;
             ++tctx.recnum;
             continue;
           }

        CERR << "BAD record charset (neither ASCII nor EBCDIC)" << endl;
        break;
      }

   fclose(in);
}
//-----------------------------------------------------------------------------
void 
Command::cmd_LIBS(ostream & out, const vector<UCS_string> & args)
{
   // Command is:
   //
   // )LIB N path         (set libdir N to path)
   // )LIB path           (set libroot to path)
   // )LIB                (display root and path states)
   //
   if (args.size() >= 2)   // set individual dir
      {
        const UCS_string & libref_ucs = args[0];
        const int libref = libref_ucs.atoi();
        if (libref_ucs.size() != 1 || libref < 0 || libref > 9)
           {
             CERR << "Invalid library referenc " << libref_ucs << "'" << endl;
             return;
           }

        UTF8_string path(args[1]);
        LibPaths::set_lib_dir((LibRef)libref, path.c_str(),
                               LibPaths::LibDir::CSRC_CMD);
        out << "LIBRARY REFERENCE " << libref << " SET TO " << path << endl;
        return;
      }

   if (args.size() == 1)   // set root
      {
        UTF8_string utf(args[0]);
        LibPaths::set_APL_lib_root(utf.c_str());
        out << "LIBRARY ROOT SET TO " << args[0] << endl;
        return;
      }

   out << "Library root: " << LibPaths::get_APL_lib_root() << 
"\n"
"\n"
"Library reference number mapping:\n"
"\n"
"---------------------------------------------------------------------------\n"
"Ref Conf  Path                                                State   Err\n"
"---------------------------------------------------------------------------\n";
         

   loop(d, 10)
       {
          UTF8_string path = LibPaths::get_lib_dir((LibRef)d);
          out << " " << d << " ";
          switch(LibPaths::get_cfg_src((LibRef)d))
             {
                case LibPaths::LibDir::CSRC_NONE:      out << "NONE" << endl;
                                                     continue;
                case LibPaths::LibDir::CSRC_ENV:       out << "ENV   ";   break;
                case LibPaths::LibDir::CSRC_ARGV0:     out << "BIN   ";   break;
                case LibPaths::LibDir::CSRC_PREF_SYS:  out << "PSYS  ";   break;
                case LibPaths::LibDir::CSRC_PREF_HOME: out << "PUSER ";   break;
                case LibPaths::LibDir::CSRC_CMD:       out << "CMD   ";   break;
             }

        out << left << setw(52) << path.c_str();
        DIR * dir = opendir(path.c_str());
        if (dir)   { out << " present" << endl;   closedir(dir); }
        else       { out << " missing (" << errno << ")" << endl; }
      }

   out <<
"===========================================================================\n" << endl;
}
//-----------------------------------------------------------------------------
DIR *
Command::open_LIB_dir(UTF8_string & path, ostream & out, const UCS_string & arg)
{
   // arg is '' or '0'-'9' or path from:
   //
   // 1.  )LIB          meaning )LIB 0
   // 2.  )LIB 1
   // 3.  )LIB directory-name
   //
   if (arg.size() == 0)   // case 1.
      {
        path = LibPaths::get_lib_dir(LIB0);
      }
   else if (arg.size() == 1 && Avec::is_digit((Unicode)arg[0]))   // case 2.
      {
        path = LibPaths::get_lib_dir((LibRef)(arg.atoi()));
      }
   else  // case 3.
      {
        path = UTF8_string(arg);
      }

   // follow symbolic links...
   //
   loop(depth, 20)
       {
         char buffer[FILENAME_MAX + 1];
         const ssize_t len = readlink(path.c_str(), buffer, FILENAME_MAX);
         if (len <= 0)   break;   // not a symlink

         buffer[len] = 0;
         if (buffer[0] == '/')   // absolute path
            {
              path = UTF8_string(buffer);
            }
          else                   // relative path
            {
              path.append((UTF8)'/');
              path.append(UTF8_string(buffer));
            }
       }

DIR * dir = opendir(path.c_str());

   if (dir == 0)
      {
        out << "IMPROPER LIBRARY REFERENCE '" << arg << "': "
            << strerror(errno) << endl;

        UTF8_ostream more;
        more << "path '" << path << "' could not be openend as directory: "
           << strerror(errno);
        Workspace::more_error() = UCS_string(more.get_data());
        return 0;
      }

   return dir;
}
//-----------------------------------------------------------------------------
bool
Command::is_directory(dirent * entry, const UTF8_string & path)
{
#ifdef _DIRENT_HAVE_D_TYPE
   return entry->d_type == DT_DIR;
#endif

UTF8_string filename = path;
UTF8_string entry_name(entry->d_name);
   filename.append('/');
   filename.append(entry_name);

DIR * dir = opendir(filename.c_str());
   if (dir) closedir(dir);
   return dir != 0;
}
//-----------------------------------------------------------------------------
void 
Command::lib_common(ostream & out, const UCS_string & arg, int variant)
{
   // 1. open directory
   //
UTF8_string path;
DIR * dir = open_LIB_dir(path, out, arg);
   if (dir == 0)   return;

   // 2. collect files and directories
   //
vector<UCS_string> apl_files;
vector<UCS_string> xml_files;
vector<UCS_string> directories;

   for (;;)
       {
         dirent * entry = readdir(dir);
         if (entry == 0)   break;   // directory done

         UTF8_string filename_utf8(entry->d_name);
         UCS_string filename(filename_utf8);
         if (is_directory(entry, path))
            {
              if (filename_utf8[0] == '.')   continue;
              filename.append(UNI_ASCII_SLASH);
              directories.push_back(filename);
              continue;
            }

         const int dlen = strlen(entry->d_name);
         if (variant == 1)
            {
              if (filename_utf8.ends_with(".apl"))
                 {
                   filename.shrink(filename.size() - 4);   // skip extension
                   apl_files.push_back(filename);
                 }
              else if (filename_utf8.ends_with(".xml"))
                 {
                   filename.shrink(filename.size() - 4);   // skip extension
                   xml_files.push_back(filename);
                 }
            }
         else
            {
              if (filename[0] == '.')   continue;         // skip dot files ...
              if (filename[dlen - 1] == '~')   continue;  // and editor backups
              apl_files.push_back(filename);
            }
       }
   closedir(dir);

   // 3. sort directories and filenames alphabetically
   //
DynArray(const UCS_string *, directory_names, directories.size());
   loop(a, directories.size())   directory_names[a] = &directories[a];
   UCS_string::sort_names(directory_names, directories.size());

DynArray(const UCS_string *, apl_filenames, apl_files.size());
   loop(a, apl_files.size())   apl_filenames[a] = &apl_files[a];
   UCS_string::sort_names(apl_filenames, apl_files.size());

DynArray(const UCS_string *, xml_filenames, xml_files.size());
   loop(x, xml_files.size())   xml_filenames[x] = &xml_files[x];
   UCS_string::sort_names(xml_filenames, xml_files.size());

   // 4. list directories first, then files
   //
DynArray(const UCS_string *, filenames, apl_files.size() + xml_files.size()
                                        + directories.size()
         );

int count = 0;
   loop(dd, directories.size())
       filenames[count++] = directory_names[dd];

   for (int a = 0, x = 0;;)
       {
         if (a >= apl_files.size())   // end of apl_files reached
            {
              loop(xx, xml_files.size() - x)
                  filenames[count++] = xml_filenames[x + xx];
              break;
            }

         if (x >= xml_files.size())   // end of xml_files reached
            {
              loop(aa, apl_files.size() - a)
                  filenames[count++] = apl_filenames[a + aa];
              break;
            }

         // both APL and XML files. Compare them
         //
         const Comp_result comp = apl_filenames[a]->compare(*xml_filenames[x]);
         if (comp == COMP_LT)        // APL filename smaller
            {
              filenames[count++] = apl_filenames[a++];
            }
         else if (comp == COMP_GT)   // XML filename smaller
            {
              filenames[count++] = xml_filenames[x++];
            }
         else                        // same
            {
              ((UCS_string *)(apl_filenames[a]))->append(UNI_ASCII_PLUS);
              filenames[count++] = apl_filenames[a++];   ++x;
            }
       }
        
   // figure column widths
   //
   enum { tabsize = 4 };
vector<int> col_width;
   UCS_string::compute_column_width(col_width, filenames, count, tabsize,
                                    Workspace::get_PrintContext().get_PW());

   loop(c, count)
      {
        const int col = c % col_width.size();
        out << *filenames[c];
        if (col == (col_width.size() - 1) || c == (count - 1))
           {
             // last column or last item: print newline
             //
             out << endl;
           }
        else
           {
             // intermediate column: print spaces
             //
             const int len = tabsize*col_width[col] - filenames[c]->size();
             Assert(len > 0);
             loop(l, len)   out << " ";
           }
      }
}
//-----------------------------------------------------------------------------
void 
Command::cmd_LIB1(ostream & out, const UCS_string & arg)
{
   Command::lib_common(out, arg, 1);
}
//-----------------------------------------------------------------------------
void 
Command::cmd_LIB2(ostream & out, const UCS_string & arg)
{
   Command::lib_common(out, arg, 2);
}
//-----------------------------------------------------------------------------
void 
Command::cmd_LOG(ostream & out, const UCS_string & arg)
{
#ifdef DYNAMIC_LOG_WANTED

   log_control(arg);

#else

   out << "\n"
<< "Command ]LOG is not available, since dynamic logging was not\n"
"configured for this APL interpreter. To enable dynamic logging (which\n"
"will decrease performance), recompile the interpreter as follows:"

<< "\n\n"
"   ./configure DYNAMIC_LOG_WANTED=yes (... "
<< "other configure options"
<< ")\n"
"   make\n"
"   make install (or try: src/apl)\n"
"\n"

<< "above the src directory."
<< "\n";

#endif
}
//-----------------------------------------------------------------------------
void 
Command::cmd_MORE(ostream & out)
{
   if (Workspace::more_error().size() == 0)
      {
        out << "NO MORE ERROR INFO" << endl;
        return;
      }

   out << Workspace::more_error() << endl;
   return;
}
//-----------------------------------------------------------------------------
void 
Command::cmd_OFF(int exit_val)
{
   cleanup(true);
   COUT << endl;
   if (!uprefs.silent)   COUT << "Goodbye." << endl;
   exit(exit_val);
}
//-----------------------------------------------------------------------------
void
Command::cmd_OUT(ostream & out, vector<UCS_string> & args)
{
   if (args.size() == 0)
      {
        out << "BAD COMMAND" << endl;
        Workspace::more_error() =
                   UCS_string("missing filename in command )OUT");
        return;
      }

UCS_string fname = args.front();
   args.erase(args.begin());

UTF8_string filename = LibPaths::get_lib_filename(LIB_NONE, fname, false,
                                                  ".atf", 0);
   
FILE * atf = fopen(filename.c_str(), "w");
   if (atf == 0)
      {
        UTF8_string fname_utf8(fname);
        out << ")OUT " << fname << " failed: " << strerror(errno) << endl;
        char cc[200];
        snprintf(cc, sizeof(cc),
                 "command )OUT: could not open file %s for writing: %s",
                 fname_utf8.c_str(), strerror(errno));
        Workspace::more_error() = UCS_string(cc);
        return;
      }

uint64_t seq = 1;   // sequence number for records written
   Workspace::write_OUT(atf, seq, args);

   fclose(atf);
}
//-----------------------------------------------------------------------------
bool
Command::check_name_conflict(ostream & out, const UCS_string & cnew,
                             const UCS_string cold)
{
int len = cnew.size();
        if (len > cold.size())   len = cold.size();

   loop(l, len)
      {
        int c1 = cnew[l];
        int c2 = cold[l];
        if (c1 >= 'a' && c1 <= 'z')   c1 -= 0x20;   // uppercase
        if (c2 >= 'a' && c2 <= 'z')   c2 -= 0x20;   // uppercase
        if (l && (c1 != c2))   return false;   // OK: different
     }

   out << "BAD COMMAND" << endl;
   Workspace::more_error() = UCS_string(
          "conflict with existing command name in command ]USERCMD");

   return true;
}
//-----------------------------------------------------------------------------
bool
Command::check_redefinition(ostream & out, const UCS_string & cnew,
                            const UCS_string fnew, const int mnew)
{
   loop(u, user_commands.size())
     {
       const UCS_string cold = user_commands[u].prefix;
       const UCS_string fold = user_commands[u].apl_function;
       const int mold = user_commands[u].mode;

       if (cnew != cold)   continue;

       // user command name matches; so must mode and function
       if (mnew != mold || fnew != fold)
         {
           out << "BAD COMMAND" << endl;
           Workspace::more_error() 
             = UCS_string(
                          "conflict with existing user command definition"
                          " in command ]USERCMD");
         }
       return true;
     }

   return false;
}
//-----------------------------------------------------------------------------
void
Command::cmd_USERCMD(ostream & out, const UCS_string & cmd,
                     vector<UCS_string> & args)
{
   // ]USERCMD
   // ]USERCMD REMOVE-ALL
   // ]USERCMD REMOVE        ]existing-command
   // ]USERCMD ]new-command  APL-fun
   // ]USERCMD ]new-command  APL-fun  mode
   //
   if (args.size() == 0)
      {
        if (user_commands.size())
           {
             loop(u, user_commands.size())
                {
                  out << user_commands[u].prefix << " → ";
                  if (user_commands[u].mode)   out << "A ";
                  out << user_commands[u].apl_function << " B"
                      << " (mode " << user_commands[u].mode << ")" << endl;
                }
           }
        return;
      }

  if (args.size() == 1 && args[0].starts_iwith("REMOVE-ALL"))
     {
       user_commands.clear();
       out << "    All user-defined commands removed." << endl;
       return;
     }

  if (args.size() == 2 && args[0].starts_iwith("REMOVE"))
     {
       loop(u, user_commands.size())
           {
             if (user_commands[u].prefix.starts_iwith(args[1]) &&
                 args[1].starts_iwith(user_commands[u].prefix))   // same
                {
                  // print first and remove then!
                  //
                  out << "    User-defined command "
                      << user_commands[u].prefix << " removed." << endl;
                  user_commands.erase(user_commands.begin() + u);
                  return;
                }
           }

       out << "BAD COMMAND+" << endl;
       Workspace::more_error() = UCS_string(
                "user command in command ]USERCMD REMOVE does not exist");
       return;
     }

   if (args.size() > 3)
      {
        out << "BAD COMMAND+" << endl;
        Workspace::more_error() =
                   UCS_string("too many parameters in command ]USERCMD");
        return;
      }

const int mode = (args.size() == 3) ? args[2].atoi() : 0;
   if (mode < 0 || mode > 1)
      {
        out << "BAD COMMAND+" << endl;
        Workspace::more_error() =
           UCS_string("unsupported mode in command ]USERCMD (0 or 1 expected)");
        return;
      }

   // check command name
   //
   loop(c, args[0].size())
      {
        bool error = false;
        if (c == 0)   error = error || args[0][c] != ']';
        else          error = error || !Avec::is_symbol_char(args[0][c]);
        if (error)
           {
             out << "BAD COMMAND+" << endl;
             Workspace::more_error() =
                   UCS_string("bad user command name in command ]USERCMD");
             return;
           }
      }

   // check conflicts with existing commands
   //
#define cmd_def(cmd_str, _cod, _arg, _hint) \
   if (check_name_conflict(out, cmd_str, args[0]))   return;
#include "Command.def"
   if (check_redefinition(out, args[0], args[1], mode))
     {
       out << "    User-defined command "
           << args[0] << " installed." << endl;
       return;
     }

   // check APL function name
   //
   loop(c, args[1].size())
      {
        if (!Avec::is_symbol_char(args[1][c]))
           {
             out << "BAD COMMAND+" << endl;
             Workspace::more_error() =
                   UCS_string("bad APL function name in command ]USERCMD");
             return;
           }
      }

user_command new_user_command = { args[0], args[1], mode };
   user_commands.push_back(new_user_command);

   out << "    User-defined command "
       << new_user_command.prefix << " installed." << endl;
}
//-----------------------------------------------------------------------------
void
Command::do_USERCMD(ostream & out, UCS_string & apl_cmd,
                    const UCS_string & line, const UCS_string & cmd,
                    vector<UCS_string> & args, int uidx)
{
  if (user_commands[uidx].mode > 0)   // dyadic
     {
        apl_cmd.append_quoted(cmd);
        apl_cmd.append(UNI_ASCII_SPACE);
        loop(a, args.size())
           {
             apl_cmd.append_quoted(args[a]);
             apl_cmd.append(UNI_ASCII_SPACE);
           }
     }

   apl_cmd.append(user_commands[uidx].apl_function);
   apl_cmd.append(UNI_ASCII_SPACE);
   apl_cmd.append_quoted(line);
}
//-----------------------------------------------------------------------------
#ifdef DYNAMIC_LOG_WANTED
void
Command::log_control(const UCS_string & arg)
{
   if (arg.size() == 0 || arg[0] == UNI_ASCII_QUESTION)  // no arg or '?'
      {
        for (LogId l = LID_MIN; l < LID_MAX; l = LogId(l + 1))
            {
              const char * info = Log_info(l);
              Assert(info);

              const bool val = Log_status(l);
              CERR << "    " << setw(2) << right << l << ": " 
                   << (val ? "(ON)  " : "(OFF) ") << left << info << endl;
            }

        return;
      }

LogId val = LogId(0);
bool skip_leading_ws = true;

   loop(a, arg.size())
      {
        if ((arg[a] <= ' ') && skip_leading_ws)   continue;
        skip_leading_ws = false;
        if (arg[a] < UNI_ASCII_0)   break;
        if (arg[a] > UNI_ASCII_9)   break;
        val = LogId(10*val + arg[a] - UNI_ASCII_0);
      }

   if (val >= LID_MIN && val <= LID_MAX)
      {
        const char * info = Log_info(val);
        Assert(info);
        const bool new_status = !Log_status(val);
        Log_control(val, new_status);
        CERR << "    Log facility '" << info << "' is now "
             << (new_status ? "ON " : "OFF") << endl;
      }
}
#endif
//-----------------------------------------------------------------------------
void
Command::transfer_context::process_record(const UTF8 * record, const
                                          vector<UCS_string> & objects)
{
const char rec_type = record[0];   // '*', ' ', or 'X'
const char sub_type = record[1];

   if (rec_type == '*')   // comment or similar
      {
        Log(LOG_command_IN)
           {
             const char * stype = " *** bad sub-record of *";
             switch(sub_type)
                {
                  case ' ': stype = " comment";     break;
                  case '(': {
                              stype = " timestamp";
                              YMDhmsu t(now());   // fallback if sscanf() != 7
                              if (7 == sscanf((const char *)(record + 1),
                                              "(%d %d %d %d %d %d %d)",
                                              &t.year, &t.month, &t.day,
                                              &t.hour, &t.minute, &t.second,
                                              &t.micro))
                                  {
                                    timestamp = t.get();
                                  }
                            }
                            break;
                  case 'I': stype = " imbed";       break;
                }

             CERR << "record #" << setw(3) << recnum << ": '" << rec_type
                  << "'" << stype << endl;
           }
      }
   else if (rec_type == ' ' || rec_type == 'X')   // object
      {
        if (new_record)
           {
             Log(LOG_command_IN)
                {
                  const char * stype = " *** bad sub-record of X";

//                          " -------------------------------------";
                  switch(sub_type)
                     {
                       case 'A': stype = " 2 ⎕TF array ";           break;
                       case 'C': stype = " 1 ⎕TF char array ";      break;
                       case 'F': stype = " 2 ⎕TF function ";        break;
                       case 'N': stype = " 1 ⎕TF numeric array ";   break;
                     }

                  CERR << "record #" << setw(3) << recnum
                       << ": " << stype << endl;
                }

             item_type = sub_type;
           }

        add(record + 1, 71);

        new_record = (rec_type == 'X');   // 'X' marks the final record
        if (new_record)
           {
             if      (item_type == 'A')   array_2TF(objects);
             else if (item_type == 'C')   chars_1TF(objects);
             else if (item_type == 'N')   numeric_1TF(objects);
             else if (item_type == 'F')   function_2TF(objects);
             else                         CERR << "????: " << data << endl;
             data.clear();
           }
      }
   else
      {
        CERR << "record #" << setw(3) << recnum << ": '" << rec_type << "'"
             << "*** bad record type '" << rec_type << endl;
      }
}
//-----------------------------------------------------------------------------
uint32_t
Command::transfer_context::get_nrs(UCS_string & name, Shape & shape) const
{
int idx = 1;

   // data + 1 is: NAME RK SHAPE RAVEL...
   //
   while (idx < data.size() && data[idx] != UNI_ASCII_SPACE)
         name.append(data[idx++]);
   ++idx;   // skip space after the name

int rank = 0;
   while (idx < data.size() &&
          data[idx] >= UNI_ASCII_0 &&
          data[idx] <= UNI_ASCII_9)
      {
        rank *= 10;
        rank += data[idx++] - UNI_ASCII_0;
      }
   ++idx;   // skip space after the rank

   loop (r, rank)
      {
        ShapeItem s = 0;
        while (idx < data.size() &&
               data[idx] >= UNI_ASCII_0 &&
               data[idx] <= UNI_ASCII_9)
           {
             s *= 10;
             s += data[idx++] - UNI_ASCII_0;
           }
        shape.add_shape_item(s);
        ++idx;   // skip space after shape[r]
      }
  
   return idx;
}
//-----------------------------------------------------------------------------
void
Command::transfer_context::numeric_1TF(const vector<UCS_string> & objects) const
{
UCS_string var_name;
Shape shape;
int idx = get_nrs(var_name, shape);

   if (objects.size() && !var_name.contained_in(objects))   return;

Symbol * sym = 0;
   if (Avec::is_quad(var_name[0]))   // system variable.
      {
        int len = 0;
        const Token t = Workspace::get_quad(var_name, len);
        if (t.get_ValueType() == TV_SYM)   sym = t.get_sym_ptr();
        else                               Assert(0 && "Bad system variable");
      }
   else                            // user defined variable
      {
        sym = Workspace::lookup_symbol(var_name);
        Assert(sym);
      }
   
   Log(LOG_command_IN)
      {
        CERR << endl << var_name << " rank " << shape.get_rank() << " IS '";
        loop(j, data.size() - idx)   CERR << data[idx + j];
        CERR << "'" << endl;
      }

Token_string tos;
   {
     UCS_string data1(data, idx, data.size() - idx);
     Tokenizer tokenizer(PM_EXECUTE, LOC);
     if (tokenizer.tokenize(data1, tos) != E_NO_ERROR)   return;
   }
 
   if (tos.size() != shape.get_volume())   return;

Value_P val(shape, LOC);
   new (&val->get_ravel(0)) IntCell(0);   // prototype

const ShapeItem ec = val->element_count();
   loop(e, ec)
      {
        const TokenTag tag = tos[e].get_tag();
        Cell * C = &val->get_ravel(e);
        if      (tag == TOK_INTEGER)  new (C) IntCell(tos[e].get_int_val());
        else if (tag == TOK_REAL)     new (C) FloatCell(tos[e].get_flt_val());
        else if (tag == TOK_COMPLEX)  new (C)
                                          ComplexCell(tos[e].get_cpx_real(),
                                                      tos[e].get_cpx_imag());
        else FIXME;
      }

   Assert(sym);
   sym->assign(val, LOC);
}
//-----------------------------------------------------------------------------
void
Command::transfer_context::chars_1TF(const vector<UCS_string> & objects) const
{
UCS_string var_name;
Shape shape;
int idx = get_nrs(var_name, shape);

   if (objects.size() && !var_name.contained_in(objects))   return;

Symbol * sym = 0;
   if (Avec::is_quad(var_name[0]))   // system variable.
      {
        int len = 0;
        const Token t = Workspace::get_quad(var_name, len);
        if (t.get_ValueType() == TV_SYM)   sym = t.get_sym_ptr();
        else                               Assert(0 && "Bad system variable");
      }
   else                            // user defined variable
      {
        sym = Workspace::lookup_symbol(var_name);
        Assert(sym);
      }
   
   Log(LOG_command_IN)
      {
        CERR << endl << var_name << " rank " << shape.get_rank() << " IS '";
        loop(j, data.size() - idx)   CERR << data[idx + j];
        CERR << "'" << endl;
      }

Value_P val(shape, LOC);
   new (&val->get_ravel(0)) CharCell(UNI_ASCII_SPACE);   // prototype

const ShapeItem ec = val->element_count();
   loop(e, ec)   new (&val->get_ravel(e)) CharCell(data[idx + e]);

   Assert(sym);
   sym->assign(val, LOC);
}
//-----------------------------------------------------------------------------
void
Command::transfer_context::array_2TF(const vector<UCS_string> & objects) const
{
   // an Array in 2 ⎕TF format
   //
UCS_string data1(data.get_items() + 1, data.size() - 1);
UCS_string var_or_fun;

   // data1 is: VARNAME←data...
   //
   if (objects.size())
      {
        UCS_string var_name;
        loop(d, data1.size())
           {
             const Unicode uni = data1[d];
             if (uni == UNI_LEFT_ARROW)   break;
             var_name.append(uni);
           }

        if (!var_name.contained_in(objects))   return;
      }

   var_or_fun = Quad_TF::tf2_inv(data1);

   Assert(var_or_fun.size());
}
//-----------------------------------------------------------------------------
void
Command::transfer_context::function_2TF(const vector<UCS_string> & objects)const
{
int idx = 1;
UCS_string fun_name;

   /// chars 1...' ' are the function name
   while ((idx < data.size()) && (data[idx] != UNI_ASCII_SPACE))
        fun_name.append(data[idx++]);
   ++idx;

   if (objects.size() && !fun_name.contained_in(objects))   return;

UCS_string statement;
   while (idx < data.size())   statement.append(data[idx++]);
   statement.append(UNI_ASCII_LF);

UCS_string fun_name1 = Quad_TF::tf2_inv(statement);
   if (fun_name1.size() == 0)   // tf2_inv() failed
      {
        CERR << "inverse 2 ⎕TF failed for the following APL statement: "
             << endl << "    " << statement << endl;
        return;
      }

Symbol * sym1 = Workspace::lookup_existing_symbol(fun_name1);
   Assert(sym1);
Function * fun1 = sym1->get_function();
   Assert(fun1);
   fun1->set_creation_time(timestamp);

   Log(LOG_command_IN)
      {
       const YMDhmsu ymdhmsu(timestamp);
       CERR << "FUNCTION '" << fun_name1 <<  "'" << endl
            << "   created: " << ymdhmsu.day << "." << ymdhmsu.month
            << "." << ymdhmsu.year << "  " << ymdhmsu.hour
            << ":" << ymdhmsu.minute << ":" << ymdhmsu.second
            << "." << ymdhmsu.micro << " (" << timestamp << ")" << endl;
      }
}
//-----------------------------------------------------------------------------
void
Command::transfer_context::add(const UTF8 * str, int len)
{

#if 0
   // helper to print the uni_to_cp_map when given a cp_to_uni_map.
   // The print-out can be triggered by )IN file.atf and then sorted
   // with the GNU/Linux command 'sort'.
   //
   loop(q, 128)
       fprintf(stderr, "  { 0x%4.4X, %u },\n", cp_to_uni_map[q], q + 128);
   exit(0);
#endif

const Unicode * cp_to_uni_map = Avec::IBM_quad_AV();
   loop(l, len)
      {
        const UTF8 utf = str[l];
        switch(utf)
           {
             case '^': data.append(UNI_AND);              break;   // ~ → ∼
             case '*': data.append(UNI_STAR_OPERATOR);    break;   // * → ⋆
             case '~': data.append(UNI_TILDE_OPERATOR);   break;   // ~ → ∼
             
             default:  data.append(Unicode(cp_to_uni_map[utf]));
           }
      }
}
//-----------------------------------------------------------------------------
bool
Command::parse_from_to(UCS_string & from, UCS_string & to,
                       const UCS_string & user_arg)
{
   // parse user_arg which is one of the following:
   //
   // 1.   (empty)
   // 2.   FROMTO
   // 3a.  FROM -
   // 3b.       - TO
   // 3c.  FROM - TO
   //
   from.clear();
   to.clear();

int s = 0;
bool got_minus = false;

   // skip spaces before from
   //
   while (s < user_arg.size() && user_arg[s] <= ' ') ++s;

   if (s == user_arg.size())   return false;   // case 1.: OK

   // copy left of - to from
   //
   while (s < user_arg.size()   &&
              user_arg[s] > ' ' &&
              user_arg[s] != '-')  from.append(user_arg[s++]);

   // skip spaces after from
   //
   while (s < user_arg.size() && user_arg[s] <= ' ') ++s;

   if (s < user_arg.size() && user_arg[s] == '-') { ++s;   got_minus = true; }

   // skip spaces before to
   //
   while (s < user_arg.size() && user_arg[s] <= ' ') ++s;

   // copy right of - to from
   //
   while (s < user_arg.size() && user_arg[s] > ' ')  to.append(user_arg[s++]);

   // skip spaces after to
   //
   while (s < user_arg.size() && user_arg[s] <= ' ') ++s;

   if (s < user_arg.size())   return true;   // error: non-blank after to

   if (!got_minus)   to = from;   // case 2.

   if (from.size() == 0 && to.size() == 0) return true;   // error: single -

   // "increment" TO so that we can compare ITEM < TO
   //
   if (to.size())   to.last() = (Unicode)(to.last() + 1);
   
   return false;   // OK
}
//-----------------------------------------------------------------------------
bool
Command::is_lib_ref(const UCS_string & lib)
{
   if (lib.size() == 1)   // single char: lib number
      {
        if (Avec::is_digit(lib[0]))   return true;
      }

   if (lib[0] == UNI_ASCII_FULLSTOP)   return true;

   loop(l, lib.size())
      {
        const Unicode uni = lib[l];
        if (uni == UNI_ASCII_SLASH)       return true;
        if (uni == UNI_ASCII_BACKSLASH)   return true;
      }

   return false;
}
//-----------------------------------------------------------------------------
ExpandResult
Command::expand_tab(UCS_string & user, int & replace_count)
{
   replace_count = user.size();

const bool have_trailing_blank = replace_count && user.last() == ' ';

   // skip leading and trailing blanks
   //
   user.remove_leading_and_trailing_whitespaces();
      
   if (user.size() == 0 || Avec::is_first_symbol_char(user[0]))
      {
        // nothing or user-defined name: expand names
        //
        const int symbol_count = Workspace::symbols_allocated();
        DynArray(Symbol *, all_symbols, symbol_count);
        Workspace::get_all_symbols(&all_symbols[0], symbol_count);

        vector<UCS_string>matches;
        loop(s, symbol_count)
            {
             const Symbol * sym = all_symbols[s];
             if (sym->is_erased())   continue;

              const UCS_string & sym_name = sym->get_name();
              if (!sym_name.starts_with(user))   continue;
              matches.push_back(sym_name);
            }

        if (matches.size() == 0)   return ER_IGNORE;   // no match
        if (matches.size() > 1)   // multiple names match user input
           {
             const int user_len = user.size();
             user.clear();
             return show_alternatives(user, user_len, matches);
           }

        // unique match
        //
        if (user.size() < matches[0].size())
           {
             // the name is longer than user, so we expand it.
             //
             user = matches[0];
             return ER_REPLACE;
           }
        return ER_IGNORE;
      }

   if (user[0] == ')' || user[0] == ']')   // APL command
      {
        ExpandHint ehint = EH_NO_PARAM;
        const char * shint = 0;
        vector<UCS_string>matches;
        UCS_string cmd = user;
        UCS_string arg;
        cmd.split_ws(arg);

#define cmd_def(cmd_str, code, arg, hint)  \
   { UCS_string ustr(cmd_str);             \
     if (ustr.starts_iwith(cmd))           \
        { matches.push_back(ustr); ehint = hint; shint = arg; } }
#include "Command.def"

        // if no match was found then ignore the TAB
        //
        if (matches.size() == 0)   return ER_IGNORE;

        // if we have multiple matches but the user has provided a command
        // argument then some matches were wrong. For example:
        //
        // LIB 3 matches LIB and LIBS
        //
        // remove wrong matches
        //
        if (matches.size() > 1 && arg.size() > 0)
           {
             again:
             if (matches.size() > 1)
                {
                  loop(m, matches.size())
                     {
                       if (matches[m].size() != cmd.size())   // wrong match
                          {
                             matches[m].swap(matches.back());
                             matches.pop_back();
                             goto again;
                          }
                     }
                }
           }

        // if multiple matches were found then either expand the common part
        // or list all matches
        //
        if (matches.size() > 1)   // multiple commands match cmd
           {
             user.clear();
             return show_alternatives(user, cmd.size(), matches);
           }

        // unique match
        //
        if (cmd.size() < matches[0].size())
           {
             // the command is longer than user, so we expand it.
             //
             user = matches[0];
             if (ehint != EH_NO_PARAM)   user.append(UNI_ASCII_SPACE);
             return ER_REPLACE;
           }

        if (cmd.size() == matches[0].size() &&
            ehint != EH_NO_PARAM            &&
            !arg.size()                     &&
            !have_trailing_blank)   // no blank entered
           {
             // the entire command was entered but without a blank. If the
             // command has arguments then append a space to indicate that.
             // Otherwiese fall throught to expand_command_arg();
             //
             user = matches[0];
             user.append(UNI_ASCII_SPACE);
             return ER_REPLACE;
           }
        return expand_command_arg(user, have_trailing_blank,
                                  ehint, shint, matches[0], arg);
      }

   // not an APL command.
   //
   user.remove_leading_and_trailing_whitespaces();
int qpos = -1;
   loop(e, 5)
      {
        if (e >= user.size())   break;
        if (user[user.size() - e - 1] == UNI_Quad_Quad)
           {
             qpos = user.size() - e;
             break;
           }
      }

   if (qpos != -1)   // ⎕xxx at end
      {
        UCS_string qxx(user, qpos, user.size() - qpos);
        vector<UCS_string>matches;

#define ro_sv_def(q, txt) { const char * qu = #q;                  \
   if (!strncmp(qu, "Quad_", 5)) { UCS_string ustr(qu + 5);   \
        if (ustr.starts_iwith(qxx)) matches.push_back(ustr); } }

#define rw_sv_def(q, txt) { const char * qu = #q;                  \
   if (!strncmp(qu, "Quad_", 5)) { UCS_string ustr(qu + 5);   \
        if (ustr.starts_iwith(qxx)) matches.push_back(ustr); } }

#define sf_def(q, txt) { const char * qu = #q;                  \
   if (!strncmp(qu, "Quad_", 5)) { UCS_string ustr(qu + 5);   \
        if (ustr.starts_iwith(qxx)) matches.push_back(ustr); } }

#include "SystemVariable.def"

        if (matches.size() == 0)   return ER_IGNORE;
        if (matches.size() > 1)
           {
            UCS_string::sort_names(matches);

            const int common_len = compute_common_length(qxx.size(), matches);
            if (common_len == qxx.size())
               {
                 // qxx is already the common part of all matching ⎕xx
                 // display matching ⎕xx
                 //
                 CIN << endl;
                 loop(m, matches.size())
                     {
                        CERR << "⎕" << matches[m] << " ";
                     }
                 CERR << endl;
                 return ER_AGAIN;
               }
            else
               {
                 // qxx is a prefix of the common part of all matching ⎕xx.
                 // expand to common part.
                 //
                 user = matches[0];
                 user.shrink(common_len);
                 return ER_REPLACE;
               }
           }

        // unique match
        //
        user = UCS_string(UNI_Quad_Quad);
        user.append(matches[0]);
        return ER_REPLACE;
      }

   return ER_IGNORE;
}
//-----------------------------------------------------------------------------
ExpandResult
Command::expand_command_arg(UCS_string & user, bool have_trailing_blank,
                            ExpandHint ehint, const char * shint,
                            const UCS_string cmd, const UCS_string arg)
{
   switch(ehint)
      {
        case EH_NO_PARAM:  return ER_IGNORE;

        case EH_oWSNAME:
        case EH_oLIB_WSNAME:
        case EH_oLIB_oPATH:
        case EH_FILENAME:
        case EH_DIR_OR_LIB:
        case EH_WSNAME:    return expand_filename(user, have_trailing_blank,
                                                  ehint, shint, cmd, arg);

        default:
             CIN << endl;
             CERR << cmd << " " << shint << endl;
             return ER_AGAIN;
      }
}
//-----------------------------------------------------------------------------
ExpandResult
Command::expand_filename(UCS_string & user, bool have_trailing_blank,
                         ExpandHint ehint, const char * shint,
                         const UCS_string cmd, UCS_string arg)
{
   if (arg.size() == 0)
      {
        // the user has entered a command but nothing else.
        // If the command accepts a library number then we propose one of
        // the existing libraries.
        //
        if (ehint == EH_oLIB_WSNAME || ehint == EH_DIR_OR_LIB)
           {
             // the command accepts a library reference number
             //
             UCS_string libs_present;
             loop(lib, 10)
                 {
                    if (!LibPaths::is_present((LibRef)lib))   continue;
                       
                    libs_present.append(UNI_ASCII_SPACE);
                    libs_present.append((Unicode)(UNI_ASCII_0 + lib));
                 }
             if (libs_present.size() == 0)   goto nothing;

             CIN << endl;
             CERR << cmd << libs_present << " <workspace-name>" << endl;
             return ER_AGAIN;
           }
        
        goto nothing;
      }

   if (arg[0] >= '0' && arg[0] <= '9')
      {
        // library number 0-9. EH_DIR_OR_LIB is complete already so
        // EH_oLIB_WSNAME is the only expansion case left
        //
        if (ehint != EH_oLIB_WSNAME)   goto nothing;

        const LibRef lib = (LibRef)(arg[0] - '0');

        if (arg.size() == 1 && !have_trailing_blank)   // no space yet
           {
             user = cmd;
             user.append(UNI_ASCII_SPACE);
             user.append((Unicode)(arg[0]));
             user.append(UNI_ASCII_SPACE);
             return ER_REPLACE;
           }

         // discard library reference number
         //
         if (arg.size() == 1)   arg.drop_leading(1);
         else                   arg.drop_leading(2);
         return expand_wsname(user, cmd, lib, arg);
      }

   {
     UCS_string dir_ucs;

     if (arg[0] == '/')
        {
          dir_ucs = arg;
        }
     else if (arg[0] == '.')
        {
          const char * pwd = getenv("PWD");
          if (pwd == 0)   goto nothing;
          dir_ucs = UCS_string(pwd);
          dir_ucs.append(UNI_ASCII_SLASH);
          dir_ucs.append(arg);
        }
     else goto nothing;

     UTF8_string dir_utf = UTF8_string(dir_ucs);

     const char * dir_dirname = dir_utf.c_str();
     const char * dir_basename = strrchr(dir_dirname, '/');
     if (dir_basename == 0)   goto nothing;

     UTF8_string base_utf = UTF8_string(++dir_basename);
     dir_utf.shrink(dir_basename - dir_dirname);
     dir_ucs = UCS_string(dir_utf);
 
     DIR * dir = opendir(dir_utf.c_str());
     if (dir == 0)   goto nothing;

     vector<UCS_string>matches;
     read_matching_filenames(dir, dir_utf, base_utf, ehint, matches);
     closedir(dir);

     if (matches.size() == 0)
        {
          CIN << endl;
          CERR << "  no matching filesnames" << endl;
          return ER_AGAIN;
        }

     if (matches.size() > 1)
        {
          UCS_string prefix(base_utf);
          user = cmd;
          user.append(UNI_ASCII_SPACE);
          user.append(dir_ucs);
          user.append(prefix);
          return show_alternatives(user, prefix.size(), matches);
        }

     // unique match
     //
     user = cmd;
     user.append(UNI_ASCII_SPACE);
     user.append(dir_ucs);
     user.append(matches[0]);
     return ER_REPLACE;
   }

nothing:
   CIN << endl;
   CERR << cmd << " " << shint << endl;
   return ER_AGAIN;
}
//-----------------------------------------------------------------------------
ExpandResult
Command::expand_wsname(UCS_string & user, const UCS_string cmd,
                       LibRef lib, const UCS_string filename)
{
UTF8_string path = LibPaths::get_lib_dir(lib);
   if (path.size() == 0)
      {
        CIN << endl;
        CERR << "Invalib library reference " << lib << endl;
        return ER_AGAIN;
      }

DIR * dir = opendir(path.c_str());
   if (dir == 0)
      {
        CIN << endl;
        CERR
<< "  library reference " << lib
<< " is a valid number, but the corresponding directory " << endl
<< "  " << path << " does not exist" << endl
<< "  or is not readable. " << endl
<< "  The relation between library reference numbers and filenames" << endl
<< "  (aka. paths) can be configured file 'preferences'." << endl << endl
<< "  At this point, you can use a path instead of the optional" << endl
<< "  library reference number and the workspace name." << endl;

        user = cmd;
        user.append(UNI_ASCII_SPACE);
        return ER_REPLACE;
      }

vector<UCS_string>matches;
UTF8_string arg_utf(filename);
   read_matching_filenames(dir, path, arg_utf, EH_oLIB_WSNAME, matches);
   closedir(dir);

   if (matches.size() == 0)   goto nothing;
   if (matches.size() > 1)
      {
        user = cmd;
        user.append(UNI_ASCII_SPACE);
        user.append_number(lib);
        user.append(UNI_ASCII_SPACE);
        return show_alternatives(user, filename.size(), matches);
      }

   // unique match
   //
   user = cmd;
   user.append(UNI_ASCII_SPACE);
   user.append_utf8(path.c_str());
   user.append(UNI_ASCII_SLASH);
   user.append(matches[0]);
   return ER_REPLACE;

nothing:
   CIN << endl;
   CERR << cmd << " " << lib << " '" << filename << "'" << endl;
   return ER_AGAIN;
}
//-----------------------------------------------------------------------------
int
Command::compute_common_length(int len, const vector<UCS_string> & matches)
{
   // we assume that all matches have the same case

   for (;; ++len)
       {
         loop(m, matches.size())
            {
              if (len >= matches[m].size())   return matches[m].size();
              if (matches[0][len] != matches[m][len])    return len;
            }
       }
}
//-----------------------------------------------------------------------------
void
Command::read_matching_filenames(DIR * dir, UTF8_string dirname,
                                 UTF8_string prefix, ExpandHint ehint,
                                 vector<UCS_string> & matches)
{
const bool only_workspaces = (ehint == EH_oLIB_WSNAME) ||
                             (ehint == EH_WSNAME     ) ||
                             (ehint == EH_oWSNAME    );

   for (;;)
       {
          struct dirent * dent = readdir(dir);
          if (dent == 0)   break;

          const size_t dlen = strlen(dent->d_name);
          if (dlen == 1 && dent->d_name[0] == '.')   continue;
          if (dlen == 2 && dent->d_name[0] == '.'
                        && dent->d_name[1] == '.')   continue;

          if (strncmp(dent->d_name, prefix.c_str(), prefix.size()))   continue;

          UCS_string name(dent->d_name);

          const bool is_dir = is_directory(dent, dirname);
          if (is_dir)   name.append(UNI_ASCII_SLASH);
          else if (only_workspaces)
             {
               const UTF8_string filename(dent->d_name);
               bool is_wsname = false;
               if (filename.ends_with(".apl"))   is_wsname = true;
               if (filename.ends_with(".xml"))   is_wsname = true;
               if (!is_wsname)   continue;
             }

          matches.push_back(name);
       }
}
//-----------------------------------------------------------------------------
ExpandResult
Command::show_alternatives(UCS_string & user, int prefix_len,
                           vector<UCS_string>matches)
{
const int common_len = compute_common_length(prefix_len, matches);

   if (common_len == prefix_len)
      {
        // prefix is already the common part of all matching files
        // display matching items
        //
        CIN << endl;
        loop(m, matches.size())
            {
              CERR << matches[m] << " ";
            }
        CERR << endl;
        return ER_AGAIN;
      }
   else
      {
        // prefix is a prefix of the common part of all matching files.
        // expand to common part.
        //
        const int usize = user.size();
        user.append(matches[0]);
        user.shrink(usize + common_len);
        return ER_REPLACE;
      }
}
//-----------------------------------------------------------------------------
