#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>

struct termios orig_tios;

//----------------------------------------------------------------------------
// read bytes for one key press
int
read_one_key(char * buffer, int buffer_size)
{
   // read first char blocking
   //
   int len1 = read(STDIN_FILENO, buffer, 1);
   if (len1 < 1)          return 0;
   if (buffer[0] == 27)   return 0;

   // read subsequent chars with timeout
   //
int buflen = 1;

   for (;;)
       {
         fd_set in_fds;
         FD_ZERO(&in_fds);
         FD_SET(STDIN_FILENO, &in_fds);
         timeval tv = { 0, 20000 };   // 20 ms

         int count = select(STDIN_FILENO + 1, &in_fds, 0, 0, &tv);
         if (count < 1)   break;

         const ssize_t len = read(STDIN_FILENO, buffer + buflen,
                                        sizeof(buffer) - buflen);

         if (len <= 0)   break;
         buflen += len;
         if (buflen >= sizeof(buffer))   break;
       }

   return buflen;
}
//----------------------------------------------------------------------------
int
main(int argc, char *argv[])
{
char * outfile_name = strdup(argv[0]);
   strcpy(outfile_name + strlen(outfile_name) - 6, ".def");

FILE * out = fopen(outfile_name, "w");

   fprintf(out,
"/*\n"
" This file contains a mapping between UTF8 encoded APL characters\n"\
" and byte(s) produced by a keyboard.\n"
"\n"
" To use this file, #define macros km_asc(), km_apl(), km_shift(), and\n"
" key_seq_N() for N=1 ,,. max number of bytes per key.\n"
"\n"
" for apl_keymap(km_asc(ascii),\n"
"                km_apl(alt_apl),\n"
"                key_seq_N(keyboard),\n"
"                km_shift(shift)) :\n"
"\n"
" ascii:    an ASCII string identifying the key (not part of the mapping,\n"
"           but possibly usefule for tools).\n"
"\n"
" alt_apl:  the APL character that shall be produced by, for example (but\n"
"           not necessarily) by pressing ALT-key or ALT-SHIFT-KEY\n"
"\n"
" keyboard: the byte(s) produced by the keyboard\n"
"\n"
" shift:    whether shift is needed\n"
"\n"
"\n"
" The mapping below is the mapping produced by a Dyalog US-APL keyboard\n"
" and can be adapted to different keyboards before compiling the tools in\n"
" this directory.\n"
"*/\n"
"\n"
         );

   fprintf(stderr, "\nhit keys, ESC to quit...\n\n");

struct termios new_tios;
   tcgetattr(STDIN_FILENO, &orig_tios);
   tcgetattr(STDIN_FILENO, &new_tios);

   new_tios.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP
                                | INLCR | IGNCR | ICRNL | IXON);
   new_tios.c_oflag &= ~OPOST;
   new_tios.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
   new_tios.c_cflag &= ~(CSIZE | PARENB);
   new_tios.c_cflag |= CS8;

   tcsetattr(STDIN_FILENO, TCSANOW, &new_tios);

struct _map
{
   const char * ascii;
   const char * apl;
   bool shift;
} table[] =
{
#define apl_keymap(asc, apl, xxx, shi) { asc, apl, shi },
#define km_asc(x)  x
#define km_apl(x)    x
#define key_seq_1(x1)
#define key_seq_2(x1, x2)
#define key_seq_3(x1, x2, x3)
#define key_seq_4(x1, x2m, x3, x4)
#define km_shift(x)    x
#include "APL_keyboard.orig.def"
};

enum { table_len = sizeof(table) / sizeof(*table) };

   for (int t = 0; t < table_len; ++t)
       {
         const _map & entry = table[t];
         fprintf(stderr, "hit key (%d of %d) for APL character %s ",
                t, table_len, entry.apl);
         if (entry.shift)   fprintf(stderr, "(shift): ");
         else               fprintf(stderr, ": ");

         char buffer[40];
         const int buflen = read_one_key(buffer, sizeof(buffer));
         if (buflen == 0)
            {
               fprintf(stderr, "\r\n");
               break;
            }

         fprintf(out, "apl_keymap(km_asc(\"%s\"), km_apl(\"%s\"), key_seq_%d(",
                 entry.ascii, entry.apl, buflen);
         for (int b = 0; b < buflen; ++b)
             {
               if (b)   fprintf(out, ", ");
               fprintf(out, "0x%2.2X", buffer[b] & 0xFF);
             }
         if (entry.shift)    fprintf(out, "), km_shift(true))\n");
         else                fprintf(out, "), km_shift(false))\n");

         for (int b = 0; b < buflen; ++b)
             {
               fprintf(stderr, " 0x%2.2X", buffer[b] & 0xFF);
             }
         fprintf(stderr, " (%d bytes)\r\n", buflen);
       }

   tcsetattr(STDIN_FILENO, TCSANOW, &orig_tios);
}

