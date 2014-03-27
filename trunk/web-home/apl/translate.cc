#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>

//               H0 H1  H2  H3  H4  H5
int Hx_num[] = { 0, 0,  0,  0,  0,  0 };
enum { Hx_max = sizeof(Hx_num) / sizeof(int) };
int Hx_len = 0;

FILE * fout = 0;   // body text
FILE * ftoc = 0;   // table off content
FILE * ftc = 0;    // testcase file

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
   Range(const char * cp, int len)
   : parent(0),
     from(cp),
     to(cp + len)
   {}

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
   r.copy();
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
             fprintf(ftc, "      ⍝ %s ---------------------------"
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
   out.print("<TABLE>\n");
   r.copy();
   out.print("</TABLE>\n");
   return 0;
}
//-----------------------------------------------------------------------------
int
TD_handler(const tag_table & tab, Range & r)
{
   out.print("<TD>");
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
   out.print("<TH>");
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
 // tag    handler        cinfo       iinfo indent
 { "#",    Comment      , ""        , 0   , 0  },
 { "BO"  , T_TE_handler , "B"       , 0   , 0  },
 { "H1"  , Hx_handler   , 0         , 1   , 0  },
 { "H2"  , Hx_handler   , 0         , 2   , 0  },
 { "H3"  , Hx_handler   , 0         , 3   , 0  },
 { "H4"  , Hx_handler   , 0         , 4   , 0  },
 { "H5"  , Hx_handler   , 0         , 5   , 0  },
 { "H6"  , Hx_handler   , 0         , 6   , 0  }, // cont HTML tc
 { "IN"  , IN_handler   , "input"   , 0   , 0  }, // no   yes  yes
 { "IN1" , IN_handler   , "input1"  , 1   , 0  }, // yes  yes  yes
 { "IN2" , IN_handler   , "input1"  , 2   , 0  }, // yes  yes  no 
 { "IN3" , IN_handler   , "input3"  , 3   , 0  }, // inl  yes  no 
 { "IN7" , IN_handler   , "input"   , 7   , 0  }, // no   yes  no 
 { "IN8" , IN_handler   , "input"   , 8   , 0  }, // no   no   yes
 { "LI"  , LI_handler   , 0         , 0   , 0  },
 { "OL"  , OL_handler   , 0         , 0   , 0  },
 { "OU"  , OU_handler   , "output"  , 0   , 0  }, // no   yes  yes
 { "OU1" , OU_handler   , "output1" , 1   , 0  }, // yes  yes  yes
 { "OU2" , OU_handler   , "output1" , 2   , 0  }, // yes  yes  no
 { "OU7" , OU_handler   , "output"  , 7   , 0  }, // no   yes  no
 { "OU8" , OU_handler   , "output"  , 8   , 0  }, // no   no   yes
 { "-",    Null_handler , 0         , 0   , 1  },
 { "SU"  , T_TE_handler , "SUP"     , 0   , 0  },
 { "su"  , T_TE_handler , "SUB"     , 0   , 0  },
 { "TAB" , TAB_handler  , 0         , 0   , 0  },
 { "TD"  , TD_handler   , 0         , 0   , 0  },
 { "TO"  , TO_handler   , 0         , 0   , 0  },
 { "TH"  , TH_handler   , 0         , 0   , 0  },
 { "TR"  , TR_handler   , 0         , 0   , 2  },
 { "UL"  , UL_handler   , 0         , 0   , 0  },
 { ""    , BR_handler   , 0         , 0   , 0  },
};

enum { open_tags_count  = sizeof(open_tags)  / sizeof(tag_table) };

//-----------------------------------------------------------------------------
int
Range::emit(const char * tag)
{
   // find handler for tag
   //
   for (int h = 0; h < open_tags_count; ++h)
       {
         const tag_table & tab = open_tags[h];
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
int
main(int argc, char * argv[])
{
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

Range r((const char *)vp, st.st_size);

const int error = r.emit("-");

   fclose(ftc);
   fclose(ftoc);
   fclose(fout);

   munmap(vp, st.st_size);
   return 0;
}
//-----------------------------------------------------------------------------
