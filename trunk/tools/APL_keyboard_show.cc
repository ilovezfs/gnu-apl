#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>

struct termios orig_tios;

//----------------------------------------------------------------------------
int
main(int argc, char *argv[])
{
   printf("\nhit keys, ESC to quit...\n\n");

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

   for (;;)
       {
         char buffer[40];

         // read first char blocking
         //
         const ssize_t len1 = read(STDIN_FILENO, buffer, 1);
         if (len1 < 1)   break;

         if (buffer[0] == 27)   break;
         int buflen = 1;

         // read subsequent chars with timeout
         //
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

           printf("key_seq_%d(", buflen);
            for (int b = 0; b < buflen; ++b)
                {
                  if (b)   printf(", ");
                  printf("0x%2.2X", buffer[b] & 0xFF);
                }
           printf("))\r\n");
       }

   tcsetattr(STDIN_FILENO, TCSANOW, &orig_tios);
}
