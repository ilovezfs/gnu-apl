
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

enum { MAX_BYTES = 10 };

// a table mapping byte sequences from the keyboard to UTF-8 encoded
// APL characters.
//
struct _map
{
   const char * apl_utf8;
   int         keyboard_bytes[MAX_BYTES];
} table[] =
{
// ignore km_asc() and km_shift entries
#define km_asc(x)
#define km_shift(x)

#define km_apl(x) x
#define key_seq_1(x1)                       { x1,                     -1 },
#define key_seq_2(x1, x2)                   { x1, x2,                 -1 },
#define key_seq_3(x1, x2, x3)               { x1, x2, x3,             -1 },
#define key_seq_4(x1, x2, x3, x4)           { x1, x2, x3, x4,         -1 },
#define key_seq_5(x1, x2, x3, x4, x5)       { x1, x2, x3, x4, x5,     -1 },
#define key_seq_6(x1, x2, x3, x4, x5, x6)   { x1, x2, x3, x4, x5, x6, -1 },

#define apl_keymap(_asc, apl, bytes, _shift) { apl, bytes },
#include "APL_keyboard.def"
};

enum { table_len = sizeof(table) / sizeof(*table) };

const _map * sorted_table[table_len];

struct termios orig_tios;
struct termios new_tios;

//-----------------------------------------------------------------------------
int
compare_map(const void * v1, const void * v2)
{
const _map * m1 = *(const _map **)v1;
const _map * m2 = *(const _map **)v2;

const int * i1 = m1->keyboard_bytes;
const int * i2 = m2->keyboard_bytes;

   for (;;)
       {
         const int c1 = *i1++;
         const int c2 = *i2++;
         if (c1 == -1)   // end of i1
            {
              if (c2 == -1)   return 0;   // also end of i2
              return -1;   // i2 longer -> m1 smaller
            }

         if (c2 == -1)   // end of i2
            {
               return 1;   // i1 longer -> m1 larger
            }

         if (c1 < c2)   return -1;
         if (c1 > c2)   return  1;
       }
}
//-----------------------------------------------------------------------------
int
main(int argc, char * argv[])
{
   tcgetattr(STDIN_FILENO, &orig_tios);
   memcpy(&new_tios, &orig_tios, sizeof(new_tios));

   new_tios.c_iflag = BRKINT | IXANY | ICRNL;
// new_tios.c_oflag = orig_tios.c_oflag;
   new_tios.c_cflag = CS8;
   new_tios.c_lflag = CLOCAL | CREAD | ISIG;

   tcsetattr(STDIN_FILENO, TCSANOW, &new_tios);

   // sort table by keyboard bytes
   //
   for (int t = 0; t < table_len; ++t)   sorted_table[t] = table + t;
   qsort(sorted_table, table_len, sizeof(const _map *), &compare_map);

_map current_map;
const _map * const key = &current_map;
int * const buffer = current_map.keyboard_bytes;
int buflen = 0;

   for (bool goon = true; goon;)
       {
         int cc = getc(stdin);
         if (cc == EOF)   break;
         if (buflen == 0 && (cc & 0x80) == 0)   // ASCII
            {
              if (cc == '\r')   fprintf(stderr, "\n");
              else              fprintf(stderr, "%c", cc);

              buffer[0] = cc;
              size_t len = fwrite(buffer, 1, 1, stdout);
              fflush(stdout);
              if (len < 1)   goon = false;
              buflen = 0;
              continue;
            }

         if (buflen > MAX_BYTES - 2)
            {
              fprintf(stderr, "too many (%d) bytes (discarding them: ", buflen);
              for (int b = 0; b < buflen; ++b)
                  fprintf(stderr, " %d", buffer[b]);
              fprintf(stderr, "\n");
              buflen = 0;
              continue;
            }

         buffer[buflen++] = cc & 0xFF;
         buffer[buflen] = -1;

         void * v = bsearch(&key, sorted_table, table_len,
                             sizeof(const _map *), &compare_map);

         if (v == 0)   continue;   // not found

         fprintf(stderr, "%s", (*(const _map **)v)->apl_utf8);
         const char * out_string = (*(const _map **)v)->apl_utf8;
         const size_t out_len = strlen(out_string);
         const size_t written = fwrite(out_string, 1, out_len, stdout);
         fflush(stdout);
         if (written < out_len)   goon = false;
         buflen = 0;
       }

   tcsetattr(STDIN_FILENO, TCSANOW, &orig_tios);
}
//-----------------------------------------------------------------------------
