#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <vector>

using namespace std;

#include <apl/libapl.h>

//               H0 H1  H2  H3  H4  H5
int Hx_num[] = { 0, 0,  0,  0,  0,  0 };
enum { Hx_max = sizeof(Hx_num) / sizeof(int) };
int Hx_len = 0;

int in_table = 0;

FILE * fout = 0;   // body text
FILE * ftoc = 0;   // table off content
FILE * ftc = 0;    // testcase file

string stdOut_chars;
string stdErr_chars;
vector<string> quad_responses;

class StdOut : public streambuf
{
   virtual int overflow(int c)
      { stdOut_chars += (char)c; }

} stdOut;

class StdErr : public streambuf
{
   virtual int overflow(int c)
      { stdErr_chars += (char)c; }

} stdErr;

extern ostream COUT;
extern ostream UERR;

//-----------------------------------------------------------------------------
class Out
{
public:
   Out()
   : need_indent(false),
     in_pre(false),
     indent_val(2)
   {}

   void Putc(char cc)
      {
        if (need_indent && !in_pre)
           {
             for (int i = 0; i < indent_val; ++i)   putc(' ', fout);
           }

        putc(cc, fout);
        need_indent = (cc == '\n');
      }

   void print(const char * str)
      {
        while (*str)   Putc(*str++);
      }

   void indent(int diff)
      {
        indent_val += diff;
      }

   void pre(bool on_off)
      { in_pre = on_off; }

protected:
   bool need_indent;
   bool in_pre;
   int  indent_val;
} out;

//-----------------------------------------------------------------------------
class Range
{
public:
   /// a range of length len, starting at cp.
   Range(const char * cp, int len);

   /// a sub-range of this range, from current position to ending at »
   Range(Range & r);

   int getc()
      {
        if (from >= to)   return EOF;

        const int cc = 0xFF & *from++;
        ++col;

        if (cc == '\n')   { ++line;   col = 0; }

        return cc;
      }

   /// maybe skip «, return true if skipped
   bool skip_start()
      {
        if (to - from < 2)      return false;
        if (!is_start(from))    return false;

        from += 2;
        return true;
      }

   /// maybe skip », return true if skipped
   bool skip_end()
      {
        if (to - from < 2)    return false;
        if (!is_end(from))    return false;

        from += 2;
        return true;
      }

   int emit(const char * tag);
   int copy();
   int fcopy(FILE * out);

   /// true if cp is «
   static bool is_start(const char * cp)
      { return (cp[0] & 0xFF) == 0xC2 && (cp[1] & 0xFF) == 0xAB; }

   /// true if cp is »
   static bool is_end(const char * cp)
      { return (cp[0] & 0xFF) == 0xC2 && (cp[1] & 0xFF) == 0xBB; }


   const char * get_from() const   { return from; }
   const char * get_to()  const    { return to;   }

protected:
   Range * parent;

   const char * from;
   const char * to;

   static int line;
   static int col;
};

int Range::line = 1;
int Range::col = 0;

//-----------------------------------------------------------------------------
Range::Range(const char * cp, int len)
   : parent(0),
     from(cp),
     to(cp + len)
{
   // this constructor is called only once (for the top level)

   // check that « and » match.
   //
int level = 0;
int lnum = 1;
   for (const char * p = from; p < to; ++p)
       {
          if (*p == '\n')   ++lnum;
          if (is_start(p))
             {
               ++level;
             }
          else if (is_end(p))
             {
               if (level > 0)
                  {
                    --level;
                  }
               else
                  {
                    fprintf(stderr, "No mathing » for « line %d\n", lnum);
                    _exit(1);
                  }
             }
       }
   
}
//-----------------------------------------------------------------------------

Range::Range(Range & r)
   : parent(&r),
     from(r.from),
     to(r.to)
{
   // this constructor is called after a beginning « was skipped.
   // find the corresponding », and remove the sub-range from r.
   //
int depth = 1;   // the skipped «
   for (const char * cp = from; cp < to; ++cp)
       {
         if (is_start(cp))
            {
              ++depth;
            }
         else if (is_end(cp))
            {
              --depth;
              if (depth == 0)   // corresponding »
                 {
                   this->to = cp;
                   r.from = cp + 2;
                   return;
                 }
            }
       }

   fprintf(stderr, "No mathing » for « at line %d col %d\n", line, col);
   _exit(1);
}
//-----------------------------------------------------------------------------
int
Range::copy()
{
   for (;;)
       {
         if (skip_start())   // another «
            {
              Range sub(*this);
              char tag[20] = { 0 };
              int idx = 0;
              if (sub.from != sub.to)   // unless «»
                 {
                   while (idx < 10)
                       {
                         const int cc = sub.getc();
                         if (cc > ' ')   tag[idx++] = cc;
                         else            break;
                       }
                   tag[idx] = 0;
                 }
              const int error = sub.emit(tag);
              if (error)   return error;
            }
         else
            {
              const int cc = getc();
              if (cc == EOF)   return 1;

              out.Putc(cc);
            }
       }
}
//-----------------------------------------------------------------------------
int
Range::fcopy(FILE * out)
{
   fwrite(from, 1, to - from, out);
   fputc('\n', out);
}
//-----------------------------------------------------------------------------
struct tag_table
{
  const char * tag;
  int (*handler)(const tag_table & tab, Range & r);
  const char * cinfo;
  int          iinfo;
  int          indent;
};
//-----------------------------------------------------------------------------
int
Null_handler(const tag_table & tab, Range & r)
{
   switch(tab.iinfo)
      {
        case 1:  out.print("&nbsp;");   break;
        default: r.copy();
      }
}
//-----------------------------------------------------------------------------
int
T_TE_handler(const tag_table & tab, Range & r)
{
   assert(tab.cinfo);
   out.print("<");
   out.print(tab.cinfo);
   out.print(">");

   r.copy();

   out.print("</");
   out.print(tab.cinfo);
   out.print(">");
   if (tab.iinfo)   out.print("\n");
   return 0;
}
//-----------------------------------------------------------------------------
int
BR_handler(const tag_table & tab, Range & r)
{
   out.print("<BR>\n");
   return 0;
}
//-----------------------------------------------------------------------------
int
Comment(const tag_table & tab, Range & r)
{
   while (r.get_from() < r.get_to())   r.getc();

   return 0;
}
//-----------------------------------------------------------------------------
int
Hx_handler(const tag_table & tab, Range & r)
{
const int Hx = tab.iinfo;
   Hx_len = Hx;

   out.print("<H");
   out.Putc(Hx + '0');
   out.print(">");

   // TOC
   //
   if (Hx > 1)
      {
        ++Hx_num[Hx];

        char chapter[100] = "";
        for (int hh = 2; hh <= Hx; ++hh)
            sprintf(chapter + strlen(chapter), "%d.", Hx_num[hh]);
        chapter[strlen(chapter) - 1] = 0;

        // TOC...
        {
          fprintf(ftoc, "<TR class=\"toc_H%d\"><TD class=\"toc_c1\"> %s",
                  Hx, chapter);

          fprintf(ftoc, "<TD class=\"toc_c2\">\n"
                        "    <a class=\"toc_a2\" href=\"#CH_%s\"> ", chapter);

          const char * cp = r.get_from();
          while (cp < r.get_to())   fprintf(ftoc, "%c", *cp++ & 0xFF);
          fprintf(ftoc, " </a>\n\n");
        }

        // BODY...
        {
          char cc[100];
          sprintf(cc, "<a name=\"CH_%s\"></a>\n", chapter);
          for (const char * s = cc; *s; )   out.Putc(*s++ & 0xFF);
          for (const char * s = chapter; *s; )   out.Putc(*s++ & 0xFF);
          out.Putc(' ');
        }

        // clear deeper sub-chapters
        //
        for (int hh = Hx + 1; hh < sizeof(Hx_num)/sizeof(*Hx_num); ++hh)
            Hx_num[hh] = 0;
      }

   r.copy();

   out.print("</H");
   out.Putc(Hx + '0');
   out.print(">");
   return 0;
}
//-----------------------------------------------------------------------------
void
do_APL(unsigned char * buffer, bool first, bool last, bool & nabla)
{
   // skip leading spaces
   //
   while (*buffer && *buffer <= ' ')   ++buffer;

   // skip trailing spaces
   //
size_t len = strlen((const char *)buffer);
   while (len && buffer[len - 1] <= ' ')   buffer[--len] = 0;

   if (len == 0)   // empty line
      {
          return;   // empty line
      }

const bool comment =   len >= 3          &&
                       buffer[0] == 0xe2 &&
                       buffer[1] == 0x8d &&
                       buffer[2] == 0x9d;

const bool new_nabla = len >= 3          &&           // ∇FUNCTION
                       buffer[0] == 0xE2 &&
                       buffer[1] == 0x88 &&
                       buffer[2] == 0x87;

   if (new_nabla)   nabla = true;

   // print the APL input line to HTML
   //
   if (!nabla)
      {
        out.pre(true);
        if      (in_table)   out.print("<b><PRE class=\"input4\">       ");
        else if (first)      out.print("<b><PRE class=\"input_T\">      ");
        else                 out.print("<b><PRE class=\"input_\">      ");
        out.print((const char *)buffer);
        out.print("</PRE></b>\n");
        out.pre(false);
      }

   if (comment)   return;

   // print chapter number to the testcase file
   //
   if (first)
      {
        char chapter[100] = "";
        for (int hh = 2; hh <= Hx_len; ++hh)
        sprintf(chapter + strlen(chapter), "%d.", Hx_num[hh]);
        chapter[strlen(chapter) - 1] = 0;
        fprintf(ftc, "      ⍝ %s ---------------------------"
                     "----------------------------\n      ⍝\n",
                chapter);
      }

   // print APL input line to the testcase file
   //
   if (nabla)
      {
        if (!new_nabla)   fprintf(ftc, " ");
        fprintf(ftc, "%s\n", buffer);
        if (len == 3)   fprintf(ftc, "\n");
        return;
      }
   fprintf(ftc, "      %s\n", buffer);

   // execute the APL line
   //
   stdOut_chars.clear();
   stdErr_chars.clear();
   if (*buffer == ')' || *buffer == ']')   // APL command
      {
        apl_command((const char *)buffer);
      }
   else
       {
         apl_exec((const char *)buffer);
       }

const bool have_err = stdErr_chars.size() > 0;

   // print the APL result
   //
   if (((stdOut_chars.size() == 1 && stdOut_chars[0] == '\n') ||
         stdOut_chars.size() == 0) && !have_err)
      {
        // empty output
        out.pre(true);
        if      (in_table)   out.print("<PRE class=\"output4\">\n\n</pre>\n");
        else if (last)       out.print("<PRE class=\"output\">\n\n</pre>\n");
        else                 out.print("<PRE class=\"output1\">\n</pre>\n");
        out.pre(false);

        fprintf(ftc, "\n");
        return;
      }

   if (stdOut_chars.size())
      {
        out.pre(true);
        if      (in_table)
           out.print("<PRE class=\"output4\">");
        else if (last && stdErr_chars.size() == 0)
           out.print("<PRE class=\"output\">");
        else
           out.print("<PRE class=\"output1\">");
        out.print(stdOut_chars.c_str());
        out.print("</PRE>\n");
        out.pre(false);

        fprintf(ftc, "%s", stdOut_chars.c_str());
      }

   if (stdErr_chars.size())
      {
        out.pre(true);
        if      (in_table)  out.print("<PRE class=\"errput4\">");
        else if (last)      out.print("<PRE class=\"errput\">");
        else                out.print("<PRE class=\"errput1\">");
        out.print(stdErr_chars.c_str());
        out.print("</PRE>\n");
        out.pre(false);

        fprintf(ftc, "%s", stdErr_chars.c_str());
      }

   fprintf(ftc, "\n");

}
//-----------------------------------------------------------------------------
int
APL_handler(const tag_table & tab, Range & r)
{
bool nabla = false;

unsigned char buf[2000];
char fun[2000];
int idx = 0;
int fidx = 0;
int line = 0;
int nabla_from = -1;
int nabla_to = -1;

   for (;;)
      {
        int cc = r.getc();
        if (cc == EOF)   break;
        fun[fidx++] = cc;
        if (cc == '\n')
           {
             buf[idx] = 0;
             do_APL(buf, line == 0, false, nabla);

             if (nabla)
                {
                  if (nabla_from == -1)   nabla_from = line;
                  nabla_to = line + 1;
                }
             else    fidx = 0;
             idx = 0;
             ++line;
           }
        else
           {
             buf[idx++] = cc;
           }
      }

   buf[idx] = 0;
   do_APL(buf, line == 0, true, nabla);

   fun[fidx] = 0;
   if (nabla)
      {
        const char * ics = "input";   // assume table
        if (!in_table)
           {
             if (nabla_from == 0)
                if (nabla_to == line)   ics = "input_TB";
                else                    ics = "input_T";
             else
                if (nabla_to == line)   ics = "input_B";
                else                    ics = "input_";
           }

        out.pre(true);
        out.print("<b><PRE class=\"");
        out.print(ics);
        out.print("\">");
        out.print(fun);
        out.print("</PRE></b>");
        out.pre(false);
      }
   
   return 0;
}
//-----------------------------------------------------------------------------
int
QUAD_handler(const tag_table & tab, Range & r)
{
string response;
   for (;;)
      {
        int cc = r.getc();
        if (cc == EOF)   break;
        response += cc;
      }
   quad_responses.push_back(response);
   return 0;
}
//-----------------------------------------------------------------------------
int
IN_handler(const tag_table & tab, Range & r)
{
const int iinfo = tab.iinfo;

   if (iinfo == 0 || iinfo == 1 || iinfo == 8)
      {
        char chapter[100] = "";
        for (int hh = 2; hh <= Hx_len; ++hh)
               sprintf(chapter + strlen(chapter), "%d.", Hx_num[hh]);
        chapter[strlen(chapter) - 1] = 0;
        if (iinfo == 0)
           {
             fprintf(ftc, "     ⍝ %s ---------------------------"
                          "----------------------------\n      ⍝\n",
                     chapter);
           }
        r.fcopy(ftc);
      }

   if (iinfo == 0 || iinfo == 1 || iinfo == 2 || iinfo == 7)
      {
        assert(tab.cinfo);
        out.print("<B><PRE class=\"");
        out.print(tab.cinfo);
        out.print("\">");
        out.pre(true);
        r.copy();
        out.pre(false);
        out.print("</PRE></B>");
      }
   else if (iinfo == 3)
      {
        out.print("<B><KBD class=\"");
        out.print(tab.cinfo);
        out.print("\">");
        r.copy();
        out.print("</KBD></B>");
      }

   return 0;
}
//-----------------------------------------------------------------------------
int
OU_handler(const tag_table & tab, Range & r)
{
const int iinfo = tab.iinfo;

   if (iinfo == 0 || iinfo == 1 || iinfo == 8)
      {
        char chapter[100] = "";
        for (int hh = 2; hh <= Hx_len; ++hh)
            sprintf(chapter + strlen(chapter), "%d.", Hx_num[hh]);
        chapter[strlen(chapter) - 1] = 0;
        r.fcopy(ftc);
        if (iinfo == 0)   fprintf(ftc, "\n\n");
      }

   if (iinfo == 0 || iinfo == 1 || iinfo == 2 || iinfo == 7)
      {
        assert(tab.cinfo);
        out.print("<PRE class=\"");
        out.print(tab.cinfo);
        out.print("\">");
        out.pre(true);
        r.copy();
        out.pre(false);
        out.print("</PRE>");
      }

   return 0;
}
//-----------------------------------------------------------------------------
int
LI_handler(const tag_table & tab, Range & r)
{
   out.print("<LI>");
   r.copy();
   out.print("</LI>");
   return 0;
}
//-----------------------------------------------------------------------------
int
TAB_handler(const tag_table & tab, Range & r)
{
   ++in_table;

const char * cls = tab.cinfo;
   out.print("<TABLE");
   if (cls)
      {
        out.print(" class=\"");
        out.print(cls);
        out.print("\"");
      }
   out.print(" cellspacing=\"0\" cellpadding=\"0\">");

   r.copy();
   out.print("</TABLE>\n");

   --in_table;
   return 0;
}
//-----------------------------------------------------------------------------
int
TD_handler(const tag_table & tab, Range & r)
{
const int columns = tab.iinfo;
const char * cls = tab.cinfo;

   out.print("<TD");
   if (cls)
      {
        out.print(" class=\"");
        out.print(cls);
        out.Putc('"');
      }

   if (columns != 1)
      {
        out.print(" colspan=\"");
        out.Putc('0' + columns);
        out.print("\"");
      }
   out.print(">");
   r.copy();
   return 0;
}
//-----------------------------------------------------------------------------
int
TO_handler(const tag_table & tab, Range & r)
{
   out.print("<LABEL class=\"token\">");
   r.copy();
   out.print("</LABEL>");
   return 0;
}
//-----------------------------------------------------------------------------
int
TH_handler(const tag_table & tab, Range & r)
{
const int columns = tab.iinfo;
const char * cls = tab.cinfo;

   out.print("<TH");
   if (cls)
      {
        out.print(" class=\"");
        out.print(cls);
        out.Putc('"');
      }

   if (columns != 1)
      {
        out.print(" colspan=\"");
        out.Putc('0' + columns);
        out.print("\"");
      }
   out.print(">");
   r.copy();
   return 0;
}
//-----------------------------------------------------------------------------
int
TR_handler(const tag_table & tab, Range & r)
{
   out.print("<TR>");
   r.copy();
   return 0;
}
//-----------------------------------------------------------------------------
int
OL_handler(const tag_table & tab, Range & r)
{
   out.print("<OL>\n");
   r.copy();
   out.print("</OL>\n");
   return 0;
}
//-----------------------------------------------------------------------------
int
UL_handler(const tag_table & tab, Range & r)
{
   out.print("<UL>\n");
   r.copy();
   out.print("</UL>\n");
   return 0;
}
//-----------------------------------------------------------------------------
tag_table open_tags[] =
{
 // tag     handler        cinfo       iinfo indent
 { "#",     Comment      , ""        , 0   , 0  },
 { "BO"   , T_TE_handler , "B"       , 0   , 0  },
 { "H1"   , Hx_handler   , 0         , 1   , 0  },
 { "H2"   , Hx_handler   , 0         , 2   , 0  },
 { "H3"   , Hx_handler   , 0         , 3   , 0  },
 { "H4"   , Hx_handler   , 0         , 4   , 0  },
 { "H5"   , Hx_handler   , 0         , 5   , 0  },    
 { "H6"   , Hx_handler   , 0         , 6   , 0  },

                           // in table       ? ------------------------+ 
                           // in .tc output  ? -------------------+    |
                           // in HTML output ? --------------+    |    |
                           // continuation   ? ---------+    |    |    |
/*                                                      |    |    |    |
   APL: APL normal APL input and output (executed)      |    |    |    |
                                                        |    |    |    | */
 { "APL"  , APL_handler  , 0         , 0   , 0  },
 { "QUAD" , QUAD_handler , 0         , 0   , 0  },
/* IN: normal APL input at the beginning of example     |    |    |    |
                                                        |    |    |    | */
 { "IN"   , IN_handler   , "input_T"  , 0   , 0  }, // no   yes  yes  no
 { "IN1"  , IN_handler   , "input_"   , 1   , 0  }, // yes  yes  yes  no
 { "IN2"  , IN_handler   , "input_"   , 2   , 0  }, // yes  yes  no   no
 { "IN3"  , IN_handler   , "input"    , 3   , 0  }, // inl  yes  no   no
 { "IN7"  , IN_handler   , "input_T"  , 7   , 0  }, // no   yes  no   no
 { "IN8"  , IN_handler   , "input_T"  , 8   , 0  }, // no   no   yes  no
/* OU: normal APL output with end of example            |    |    |    |
                                                        |    |    |    | */
 { "OU"   , OU_handler   , "output"   , 0   , 0  }, // no   yes  yes  no
 { "OU1"  , OU_handler   , "output1"  , 1   , 0  }, // yes  yes  yes  no
 { "OU2"  , OU_handler   , "output1"  , 2   , 0  }, // yes  yes  no   no
 { "OU7"  , OU_handler   , "output"   , 7   , 0  }, // no   yes  no   no
 { "OU8"  , OU_handler   , "output"   , 8   , 0  }, // no   no   yes  no

 { "LI"   , LI_handler   , 0          , 0   , 0  },
 { "OL"   , OL_handler   , 0          , 0   , 0  },
 { "-"    , Null_handler , 0         , 0   , 1  },
 { "EMPTY", Null_handler , 0         , 1   , 1  },
 { "SU"   , T_TE_handler , "SUP"     , 0   , 0  },
 { "su"   , T_TE_handler , "SUB"     , 0   , 0  },
 { "TAB"  , TAB_handler  , 0         , 0   , 0  },
 { "TAB1" , TAB_handler  , "table1"  , 0   , 0  },
 { "TD"   , TD_handler   , "tab"     , 1   , 0  }, // 1 column
 { "TD1"  , TD_handler   , "tab1"    , 1   , 0  }, // format: tab1
 { "TD2"  , TD_handler   , "tab2"    , 1   , 0  }, // format: tab2
 { "TD3"  , TD_handler   , "tab3"    , 1   , 0  }, // format: tab3
 { "TD4"  , TD_handler   , "tab4"    , 1   , 0  }, // format: tab4
 { "TD5"  , TD_handler   , "tab5"    , 1   , 0  }, // format: tab5
 { "TH"   , TH_handler   , 0         , 1   , 0  },
 { "TH1"  , TH_handler   , "tab1"    , 1   , 0  }, // format: tab1
 { "TH12" , TH_handler   , "tab12"   , 2   , 0  }, // format: tab12, 2 columns
 { "TH2"  , TH_handler   , "tab2"    , 1   , 0  }, // format: tab2
 { "TH3"  , TH_handler   , "tab3"    , 1   , 0  }, // format: tab3
 { "TH4"  , TH_handler   , "tab4"    , 1   , 0  }, // format: tab4
 { "TH45" , TH_handler   , "tab45"   , 2   , 0  }, // format: tab45, 2 columns
 { "TH5"  , TH_handler   , "tab5"    , 5   , 0  }, // format: tab5
 { "TO"   , TO_handler   , 0         , 0   , 0  },
 { "TR"   , TR_handler   , 0         , 0   , 2  },
 { "UL"   , UL_handler   , 0         , 0   , 0  },
 { ""     , BR_handler   , 0         , 0   , 0  },
};

enum { open_tags_count  = sizeof(open_tags)  / sizeof(tag_table) };

//-----------------------------------------------------------------------------
int
Range::emit(const char * tag)
{
   // find handler for tag. Search backwards so that we can store shorter
   // tags befor longer ones (like OU before OU1) and still get the longer
   // match.
   //
   for (int h = 0; h < open_tags_count; ++h)
       {
         const tag_table & tab = open_tags[open_tags_count - h - 1];
         if (!strcmp(tag, tab.tag))
            {
              out.indent(tab.indent);
              const int error = tab.handler(tab, *this);
              out.indent(-tab.indent);
              return error;
            }
       }

   fprintf(stderr, "NO handler for '«%s' at line %d col %d\n", tag, line, col);
   return 1;
}
//-----------------------------------------------------------------------------
const char *
get_line_callback(int mode, const char * prompt)
{
static char buffer[2000];

   strncpy(buffer, quad_responses.back().c_str(), sizeof(buffer));
   quad_responses.pop_back();
   return buffer;
}
//-----------------------------------------------------------------------------
int
main(int argc, char * argv[])
{
  init_libapl(argv[0], /* do not log startup */ 0);
  install_get_line_from_user_cb(&get_line_callback);

   assert(argc == 5 && "missing filename(s). Stop.");

const int fd = open(argv[1], O_RDONLY);
   assert(fd != -1 && "bad input filename");

   fout = fopen(argv[2], "w");
   assert(fout && "bad output filename");

   ftoc = fopen(argv[3], "w");
   assert(ftoc && "bad TOC filename");

   ftc =  fopen(argv[4], "w");
   assert(ftc && "bad TC filename");

struct stat st;
   fstat(fd, &st);

void * vp = mmap(0, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
   assert(vp != MAP_FAILED);
   close(fd);

   COUT.rdbuf(&stdOut);
   UERR.rdbuf(&stdErr);


Range r((const char *)vp, st.st_size);

const int error = r.emit("-");

   fclose(ftc);
   fclose(ftoc);
   fclose(fout);

   munmap(vp, st.st_size);
   return 0;
}
//-----------------------------------------------------------------------------
