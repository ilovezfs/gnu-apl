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

#include <errno.h>
#include <signal.h>

#include "Assert.hh"
#include "Command.hh"
#include "Error.hh"
#include "Input.hh"
#include "InputFile.hh"
#include "IO_Files.hh"
#include "LineInput.hh"
#include "main.hh"
#include "SystemVariable.hh"
#include "UserPreferences.hh"
#include "Workspace.hh"

// hooks for external editors (emacs)
extern void (*start_input)();
extern void (*end_input)();

LineInput * LineInput::the_line_input = 0;

const char ESCmap::ESC_CursorUp[]    = { 0x1B, 0x5B, 0x41, 0 };
const char ESCmap::ESC_CursorDown[]  = { 0x1B, 0x5B, 0x42, 0 };
const char ESCmap::ESC_CursorRight[] = { 0x1B, 0x5B, 0x43, 0 };
const char ESCmap::ESC_CursorLeft[]  = { 0x1B, 0x5B, 0x44, 0 };
const char ESCmap::ESC_CursorEnd[]   = { 0x1B, 0x5B, 0x46, 0 };
const char ESCmap::ESC_CursorHome[]  = { 0x1B, 0x5B, 0x48, 0 };
const char ESCmap::ESC_InsertMode[]  = { 0x1B, 0x5B, 0x32, 0x7E, 0 };
const char ESCmap::ESC_Delete[]      = { 0x1B, 0x5B, 0x33, 0x7E, 0 };

ESCmap ESCmap::the_ESCmap[] =
{
  { 3, ESC_CursorUp,    UNI_CursorUp     },
  { 3, ESC_CursorDown,  UNI_CursorDown   },
  { 3, ESC_CursorRight, UNI_CursorRight  },
  { 3, ESC_CursorLeft,  UNI_CursorLeft   },
  { 3, ESC_CursorEnd,   UNI_CursorEnd    },
  { 3, ESC_CursorHome,  UNI_CursorHome   },
  { 4, ESC_InsertMode,  UNI_InsertMode   },
  { 4, ESC_Delete,      UNI_ASCII_DELETE },
};

enum { ESCmap_entry_count = sizeof(ESCmap::the_ESCmap) / sizeof(ESCmap) };

//=============================================================================
bool
ESCmap::has_prefix(const char * seq, int seq_len) const
{
   if (seq_len >= len)   return false;
   loop(s, seq_len)
      {
        if (seq[s] != seqence[s])   return false;
      }

   return true;
}
//-----------------------------------------------------------------------------
bool
ESCmap::is_equal(const char * seq, int seq_len) const
{
   if (len != seq_len)   return false;
   loop(s, seq_len)
      {
        if (seq[s] != seqence[s])   return false;
      }

   return true;
}
//=============================================================================
void
LineHistory::read_history(const char * filename)
{
FILE * hist = fopen(filename, "r");
   if (hist == 0)
      {
        Log(LOG_get_line)
           CERR << "Cannot open history file " << filename
                << ": " << strerror(errno) << endl;
        return;
      }

int count = 0;
   for (;;)
       {
         char buffer[2000];
         const char * s = fgets(buffer, sizeof(buffer) - 1, hist);
         if (s == 0)   break;   // end of file
         buffer[sizeof(buffer) - 1] = 0;

         int slen = strlen(buffer);
         if (buffer[slen - 1] == '\n')   buffer[--slen] = 0;
         if (buffer[slen - 1] == '\r')   buffer[--slen] = 0;
         ++count;
         UTF8_string utf(buffer);
         UCS_string ucs(utf);
         add_line(ucs);
       }

   cerr << count << " history lines read from file " << filename << endl;

   next();
}
//-----------------------------------------------------------------------------
void
LineHistory::save_history(const char * filename)
{
   if (lines.size() == 0)   return;

ofstream outf(filename);
   if (!outf.is_open())
      {
        CERR << "Cannot write history file " << filename
             << ": " << strerror(errno) << endl;
         return;
      }

int count = 0;
   for (int p = put + 1; p < lines.size(); ++p)
      {
        outf << lines[p] << endl;
        ++count;
      }
   for (int p = 0; p < put; ++p)
      {
        outf << lines[p] << endl;
        ++count;
      }

   Log(LOG_get_line)
      cerr << count << " history lines written to " << filename << endl;
}
//-----------------------------------------------------------------------------
void
LineHistory::clear_history(ostream & out)
{
   current_line = 0;
   put = 0;
   lines.resize(0);
UCS_string u("xxx");
   add_line(u);
}
//-----------------------------------------------------------------------------
void
LineHistory::print_history(ostream & out)
{
   for (int p = put + 1; p < lines.size(); ++p)
      {
        out << "      " << lines[p] << endl;
      }
   for (int p = 0; p < put; ++p)
      {
        out << "      " << lines[p] << endl;
      }
}
//-----------------------------------------------------------------------------
void
LineHistory::add_line(const UCS_string & line)
{
   if (!line.has_black())   return;

   if (lines.size() < max_lines)   // append
      {
        lines.push_back(line);
        put = 0;
      }
   else                            // override
      {
        if (put >= lines.size())   put = 0;   // wrap
        lines[put++] = line;
      }

   next();   // update current_line
}
//-----------------------------------------------------------------------------
const UCS_string *
LineHistory::up()
{
   if (lines.size() == 0)   return 0;

int new_current_line = current_line - 1;
    if (new_current_line < 0)   new_current_line += lines.size();   // wrap
    if (new_current_line == put)   return 0;

   return &lines[current_line = new_current_line];
}
//-----------------------------------------------------------------------------
const UCS_string *
LineHistory::down()
{
   if (lines.size() == 0)     return 0;
   if (current_line == put)   return 0;

int new_current_line = current_line + 1;
   if (new_current_line >= lines.size())   new_current_line = 0;   // wrap
   if (new_current_line == put)   return 0;

   return &lines[current_line = new_current_line];
}
//=============================================================================
LineEditContext::LineEditContext(LineInputMode mode, int rows, int cols,
                                 LineHistory & hist, const UCS_string & prmt)
   : screen_rows(rows),
     screen_cols(cols),
     allocated_height(1),
     uidx(0),
     history(hist),
     history_entered(false)
{
   if (mode == LIM_Quote_Quad)
      {
        // the prompt was printed by ⍞ already. Make it the beginning of
        // user_line so that it can be edited.
        //
        user_line = prmt.no_pad();
        uidx = user_line.size();
      }
   else
      {
        prompt = prmt.no_pad();
        refresh_all();
      }
}
//-----------------------------------------------------------------------------
void
LineEditContext::adjust_allocated_height()
{
const int rows = 1 + get_total_length() / screen_cols;

   if (allocated_height >= rows)   return;

   // scroll some lines so that prior text is not overridden.
   //
   CIN.set_cursor(-1, 0);
   loop(a, rows - allocated_height)   CIN << endl;

   allocated_height = rows;

   // redraw screen from -allocated_height:0 onwards
   //
   if (CIN.can_clear_EOS())
      {
        CIN.set_cursor(-allocated_height, 0);
        CIN.clear_EOS();
      }
   else
      {
        loop(a, allocated_height)
           {
             CIN.set_cursor(a - allocated_height, 0);
             CIN.clear_EOL();
           }
      }

   refresh_all();
}
//-----------------------------------------------------------------------------
void
LineEditContext::refresh_from_cursor()
{
const int saved_uidx = uidx;

   adjust_allocated_height();
   set_cursor();
   for (; uidx < user_line.size(); ++uidx)
       {
         refresh_wrapped_cursor();
         CIN << user_line[uidx];
       }

   // clear from end of user_line
   //
   move_idx(user_line.size());
   if (CIN.can_clear_EOS())
      {
        CIN.clear_EOS();
      }
   else
      {
        CIN.clear_EOL();

        // clear subsequent lines
        //
        for (int a = 1; a < allocated_height; ++a)
            {
              CIN.set_cursor(a - allocated_height, 0);
              CIN.clear_EOL();
            }
      }

   move_idx(saved_uidx);
}
//-----------------------------------------------------------------------------
void
LineEditContext::refresh_all()
{
const int saved_uidx = uidx;
   uidx = 0;

   CIN.set_cursor(-allocated_height, 0);
   CIN << prompt << user_line;
   refresh_from_cursor();
   move_idx(saved_uidx);
}
//-----------------------------------------------------------------------------
void
LineEditContext::insert_char(Unicode uni)
{
   if (uidx >= user_line.size())
      {
        user_line.append(uni);
        adjust_allocated_height();
        refresh_wrapped_cursor();
        CIN << uni;
      }
   else
      {
        user_line.insert(uidx, 1, uni);
        adjust_allocated_height();
        refresh_wrapped_cursor();
        refresh_from_cursor();
      }

   move_idx(uidx + 1);
}
//-----------------------------------------------------------------------------
void
LineEditContext::delete_char()
{
   if (uidx == (user_line.size() - 1))   // cursor on last char
      {
        CIN << ' ' << UNI_ASCII_BS;
        user_line.pop();
      }
   else
      {
        user_line.erase(uidx, 1);
        refresh_from_cursor();
      }

   if (get_total_length() >= screen_cols)   set_cursor();
}
//-----------------------------------------------------------------------------
void
LineEditContext::tab(LineInputMode mode)
{
   if (mode != LIM_ImmediateExecution)   return;

int replace_count = 0;
UCS_string line = user_line;

   switch(Command::expand_tab(line, replace_count))
      {
        case ER_IGNORE: return;

        case ER_AGAIN:
             // expand_tab has shown a list of options.
             // Reset the input window and redisplay.
             allocated_height = 1;
             adjust_allocated_height();
             refresh_all();
             return;

        case ER_REPLACE:
             user_line.shrink(user_line.size() - replace_count);
             user_line.append(line);
             uidx = 0;
             refresh_from_cursor();
             move_idx(user_line.size());
             return;

        default: FIXME;
      }
}
//-----------------------------------------------------------------------------
void
LineEditContext::cursor_up()
{
const UCS_string * ucs = history.up();
   if (ucs == 0)   return;

   if (!history_entered)
      {
        user_line_before_history = user_line;
        history_entered = true;
      }

   user_line = *ucs;
   adjust_allocated_height();
   uidx = 0;
   refresh_from_cursor();
   move_idx(user_line.size());
}
//-----------------------------------------------------------------------------
void
LineEditContext::cursor_down()
{
const UCS_string * ucs = history.down();

   if (ucs)                    user_line = *ucs;   // got history line
   else if (history_entered)   user_line = user_line_before_history;
   else                        return;

   // no need for adjust_allocated_height() because we were on this line before
   uidx = 0;
   refresh_from_cursor();
   move_idx(user_line.size());
}
//=============================================================================
LineInput::LineInput(bool do_read_history)
   : history(uprefs.line_history_len)
{
   initial_termios_errno = 0;

   if (UserPreferences::use_readline)
      {
        initial_termios_errno = -1;
        return;
      }

   if (tcgetattr(STDIN_FILENO, &initial_termios))
      initial_termios_errno = errno;

   if (do_read_history)
      history.read_history(uprefs.line_history_path.c_str());

   current_termios = initial_termios;

   // set current_termios to raw mode
   //
   current_termios.c_iflag &= ~( ISTRIP | // don't strip off bit 8
                                 INLCR  | // don't NL → CR
                                 IGNCR  | // don't ignore CR
                                 IXON);   // don't enable XON/XOFF on output
   current_termios.c_iflag |=    IGNBRK | // ignore break
                                 IGNPAR |
                                 ICRNL  ; // CR → NL

   current_termios.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN);
   current_termios.c_lflag |= ISIG;

   tcsetattr(STDIN_FILENO, TCSANOW, &current_termios);
}
//-----------------------------------------------------------------------------
LineInput::~LineInput()
{
   if (initial_termios_errno)   return;

   history.save_history(uprefs.line_history_path.c_str());
   tcsetattr(STDIN_FILENO, TCSANOW, &initial_termios);
}
//=============================================================================
void
InputMux::get_line(LineInputMode mode, const UCS_string * prompt,
                   UCS_string & line, bool & eof)
{
   if (InputFile::is_validating())   Quad_QUOTE::done(true, LOC);

   InputFile::increment_current_line_no();

   // check if we have input from a files...
   {
     UTF8_string file_line;
     bool file_eof = false;
     IO_Files::get_file_line(file_line, file_eof);

     if (!file_eof)
        {
          line = UCS_string(file_line);

          switch(mode)
             {
               case LIM_ImmediateExecution:
               case LIM_Quad_Quad:
               case LIM_Quad_INP:
                    CIN << Workspace::get_prompt() << line << endl;
                    break;

               case LIM_Quote_Quad:
                    Assert(prompt);
                    line = *prompt;

                    // for each leading backspace in line: discard last
                    // prompt character. This is for testing the user
                    // backspacing over the ⍞ prompt
                    //
                    while (line.size()      &&
                           file_line.size() &&
                           file_line[0] == UNI_ASCII_BS)
                          {
                            file_line.drop_leading(1);
                            line.pop();
                          }
                    line.append_utf8(file_line.c_str());
                    break;

               case LIM_Nabla:
                    break;

               default: FIXME;
             }

          return;
        }
   }

   // no input from files: get line from terminal
   //
   if (uprefs.raw_cin)
      {
        Quad_QUOTE::done(true, LOC);
        char buffer[4000];
        const char * s = fgets(buffer, sizeof(buffer) - 1, stdin);
        if (s == 0)
           {
             eof = true;
             return;
           }
         buffer[sizeof(buffer) - 1] = 0;

         int slen = strlen(buffer);
         if (buffer[slen - 1] == '\n')   buffer[--slen] = 0;
         if (buffer[slen - 1] == '\r')   buffer[--slen] = 0;
        UTF8_string line_utf(buffer);
        line = UCS_string(line_utf);
        return;
      }

   if (UserPreferences::use_readline)
      {
        Quad_QUOTE::done(true, LOC);
        Input::get_line(mode, line, eof);
        return;
      }

   Quad_QUOTE::done(mode != LIM_Quote_Quad, LOC);

const APL_time_us from = now();
   if (start_input)   (*start_input)();

   for (int control_D_count = 0; ; ++control_D_count)
       {
         bool _eof = false;
         LineInput::get_terminal_line(mode, prompt, line, _eof);
         if (!_eof)   break;

         // ^D or end of file
         if (control_D_count < 5)
            {
              CIN << endl;
              COUT << "      ^D or end-of-input detected ("
                   << control_D_count << "). Use )OFF to leave APL!"
                   << endl;
           } 

         if (control_D_count > 10 && (now() - from)/control_D_count < 10000)
            {
              // we got 10 or more times EOF (or possibly ^D) at a rate
              // of 10 ms or faster. That looks like end-of-input rather
              // than ^D typed by the user. Abort the interpreter.
              //
              CIN << endl;
              COUT << "      *** end of input" << endl;
              Command::cmd_OFF(2);
            }
      }

   Log(LOG_get_line)   CERR << " '" << line << "'" << endl;

   Workspace::add_wait(now() - from);
   if (end_input)   (*end_input)();
}
//=============================================================================
void
LineInput::get_terminal_line(LineInputMode mode, const UCS_string * prompt,
                            UCS_string & line, bool & eof)
{
   // no file input: get line interactively
   //
   switch(mode)
      {
        case LIM_ImmediateExecution:
        case LIM_Quad_Quad:
        case LIM_Quad_INP:
             {
               Output::set_color_mode(Output::COLM_INPUT);
               no_readline(mode, Workspace::get_prompt(), line, eof);
               return;
             }

        case LIM_Quote_Quad:
             {
               Assert(prompt);
               no_readline(LIM_Quote_Quad, *prompt, line, eof);
               return;
             }

        case LIM_Nabla:
             {
               Assert(prompt);
               no_readline(LIM_Nabla, *prompt, line, eof);
               return;
             }

        default: FIXME;
      }

   Assert(0 && "Bad LineInputMode");
}
//-----------------------------------------------------------------------------
void
LineInput::no_readline(LineInputMode mode, const UCS_string & prompt,
                       UCS_string & user_line, bool & eof)
{
   the_line_input->current_termios.c_lflag &= ~ISIG;
   tcsetattr(STDIN_FILENO, TCSANOW, &the_line_input->current_termios);

   user_line.clear();

LineEditContext lec(mode, 24, Workspace::get_PrintContext().get_PW(),
                    the_line_input->history, prompt);

   for (;;)
       {
         const Unicode uni = get_uni();
         switch(uni)
            {
              case UNI_InsertMode:
                   lec.toggle_insert_mode();
                   continue;

              case UNI_CursorHome:
                   lec.cursor_home();
                   continue;

              case UNI_CursorEnd:
                   lec.cursor_end();
                   continue;

              case UNI_CursorLeft:
                   lec.cursor_left();
                   continue;

              case UNI_CursorRight:
                   lec.cursor_right();
                   continue;

              case UNI_CursorDown:
                   lec.cursor_down();
                   continue;

              case UNI_CursorUp:
                   lec.cursor_up();
                   continue;

              case UNI_EOF:  // end of file
                   eof = user_line.size() == 0;
                   break;

              case UNI_ASCII_ETX:   // ^C
                   lec.clear();
                   control_C(SIGINT);
                   break;

              case UNI_ASCII_EOT:   // ^D
                   CERR << "^D";
                   eof = true;
                   break;

              case UNI_ASCII_BS:
                   lec.backspace();
                   continue;

              case UNI_ASCII_HT:
                   lec.tab(mode);
                   continue;

              case UNI_ASCII_DELETE:
                   lec.delete_char();
                   continue;

              case UNI_ASCII_CR:   // '\r' : ignore
                   continue;

              case UNI_ASCII_LF:   // '\n': done
                   break;

              default:  // regular APL character
                   lec.insert_char(uni);
                   continue;
            }

         break;
       }

   the_line_input->current_termios.c_lflag |= ISIG;
   tcsetattr(STDIN_FILENO, TCSANOW, &the_line_input->current_termios);

   user_line = lec.get_user_line();
   the_line_input->history.add_line(user_line);
   CIN << endl;
}
//-----------------------------------------------------------------------------
Unicode
LineInput::get_uni()
{
const int b0 = fgetc(stdin);
   if (b0 == EOF)   return UNI_EOF;

   if (b0 & 0x80)   // non-ASCII unicode
      {
        int len;
        uint32_t bx = b0;   // the "significant" bits in b0
        if ((b0 & 0xE0) == 0xC0)        { len = 2;   bx &= 0x1F; }
        else if ((b0 & 0xF0) == 0xE0)   { len = 3;   bx &= 0x0F; }
        else if ((b0 & 0xF8) == 0xF0)   { len = 4;   bx &= 0x0E; }
        else if ((b0 & 0xFC) == 0xF8)   { len = 5;   bx &= 0x0E; }
        else if ((b0 & 0xFE) == 0xFC)   { len = 6;   bx &= 0x0E; }
        else
           {
             CERR << "Bad UTF8 sequence start at " << LOC << endl;
             return Invalid_Unicode;
           }

        uint32_t uni = 0;
        loop(l, len - 1)
            {
              const UTF8 subc = fgetc(stdin);
              if ((subc & 0xC0) != 0x80)
                 {
                   CERR << "Bad UTF8 sequence: " << HEX(b0)
                        << "... at " LOC << endl;
                   return Invalid_Unicode;
                 }

              bx  <<= 6;
              uni <<= 6;
              uni |= subc & 0x3F;
            }

        return (Unicode)(bx | uni);

      }
   else if (b0 == UNI_ASCII_ESC)
      {
        char seq[ESCmap::MAX_ESC_LEN];   seq[0] = UNI_ASCII_ESC;
        for (int s = 1; s < ESCmap::MAX_ESC_LEN; ++s)
            {
              const int bs = fgetc(stdin);
              if (bs == EOF)   return UNI_EOF;
              seq[s] = bs;

              // check for exact match
              //
              loop(e, ESCmap_entry_count)
                  {
                  if (ESCmap::the_ESCmap[e].is_equal(seq, s + 1))
                     return ESCmap::the_ESCmap[e].uni;
                  }

              // check for prefix match
              //
              bool is_prefix = false;
              loop(e, ESCmap_entry_count)
                  {
                    if (ESCmap::the_ESCmap[e].has_prefix(seq, s))
                       {
                          is_prefix = true;
                          break;   // loop(e)
                       }
                  }
              
              if (is_prefix)   continue;

              CERR << endl << "Unknown ESC sequence: ESC";
              loop(ss, s)   CERR << " " << HEX2(seq[ss + 1]);
              CERR << endl;

              return Invalid_Unicode;
            }
      }
   else if (b0 == UNI_ASCII_DELETE)   return UNI_ASCII_BS;

   return (Unicode)b0;
}
//-----------------------------------------------------------------------------
