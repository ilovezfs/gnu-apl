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

#include <stdlib.h>
#include <string.h>
#include <netdb.h>

#include "Common.hh"
#include "main.hh"
#include "Output.hh"
#include "ProcessorID.hh"
#include "Quad_SVx.hh"
#include "Svar_signals.hh"

AP_num3 ProcessorID::id(NO_AP, AP_NULL, AP_NULL);

Network_Profile ProcessorID::network_profile;

uint16_t ProcessorID::APnnn_port = 0;
UdpSocket ProcessorID::APnnn_socket(true);
bool ProcessorID::doing_SV = true;

//-----------------------------------------------------------------------------
int
ProcessorID::init(int argc, const char *argv[], bool do_sv)
{
   doing_SV = do_sv;

   for (int a = 1; a < argc; )
       {
         const char * opt = argv[a++];
         const char * val = (a < argc) ? argv[a] : 0;

         if (!strcmp(opt, "--id"))
            {
              ++a;
              if (!val)
                 {
                   CERR << "--id without processor number" << endl;
                   return 1;
                 }

             if (check_own_id(val))   // sets own_id if successful
                {
                  CERR << "--id with bad or used processor number" << endl;
                  return 2;
                }
            }
         else if (!strcmp(opt, "--par"))
            {
              ++a;
              if (!val)
                 {
                   CERR << "--par without processor number" << endl;
                   return 3;
                 }
              id.parent = AP_num(atoi(val));
            }
       }

   if (id.proc == NO_AP)
      {
        // no --id option in argv: use first free user ID
        //
        id.proc = Svar_DB::get_unused_id();
      }

   if (!do_sv)   return 0;

   Quad_SVx::start_AP(id.proc, true);
   APnnn_port = Svar_DB::get_udp_port(id.proc, id.parent);
   if (APnnn_port == 0)
      {
        CERR << "*** Failed to start APnnn: processor " << id.proc
             << " will not accept incoming shared variable offers."
             << endl;
        return 4;
      }

   new (&APnnn_socket) UdpClientSocket(APnnn_port, 0);

   return 0;
}
//-----------------------------------------------------------------------------
bool
ProcessorID::check_own_id(const char * arg)
{
   // arg is the value provided for command line option --id,
   // check that is does nor conflict with an existing ID
   //
   id.proc = NO_AP;
   id.parent = AP_NULL;
   id.grand = AP_NULL;

int own = NO_AP;
int par = AP_NULL;
int grand = AP_NULL;
const int count = sscanf(arg, "%u,%u,%u", &own, &par, &grand);
   if (count < 1)   return true;

   if (Svar_DB::is_unused_id(AP_num(own)))
      {
        id.proc   = AP_num(own);
        id.parent = AP_num(par);
        id.grand  = AP_num(grand);
        return false;   // OK
      }

   return true;   // id is in use
}
//-----------------------------------------------------------------------------
const char *
ProcessorID::read_svopid(FILE * file, SvoPid & svopid, int & line)
{
const char * loc = 0;

   for (line = 1;; ++line)
       {
         char buffer[200];
         const long filepos = ftell(file);   // remember file position
         char * s = fgets(buffer, sizeof(buffer) - 1, file);

         if (s == 0)   break;   // end of file


         // skip leading whitespaces
         //
         while (*s && *s < ' ')   ++s;

         if (*s == '*')   continue;   // comment line
         if (*s == 0)     continue;   // empty line

         if (!strcmp(s, ":processor,"))
            {
              int i = NO_AP;
              int j = AP_NULL;
              int k = AP_NULL;

              const int count = sscanf(s, ":processor,%u,%u,%u", &i, &j, &k);
              if (count < 1)   { loc = LOC;   break; }
              if (count > 3)   { loc = LOC;   break; }

              svopid.id = AP_num3(AP_num(i), AP_num(j), AP_num(k));
              continue;
            }

         if (!strcmp(s, ":address,"))
            {
              // skip tag and remove trailing whitespaces
              //
              s += 9;
              char * e = s + strlen(s);
              while (e > s && *e > 0 && *e < ' ')   *--e = 0;

              hostent * host = gethostbyname(s);
              if (host == 0)
                 {
                   CERR << "gethostbyname(" << s << ") failed" << endl;
                   loc = LOC;
                   break;
                 }

              unsigned int hh = host->h_addr_list[0][0];
              unsigned int hl = host->h_addr_list[0][1];
              unsigned int lh = host->h_addr_list[0][2];
              unsigned int ll = host->h_addr_list[0][3];

              svopid.ip_addr = hh << 24 | hl << 16 | lh << 8 | ll;
              continue;
            }

         if (!strcmp(s, ":userid,"))
            {
              s += 8;
              for (int u = 1; u < sizeof(svopid.user); ++u)
                  {
                    if (*s < ' ')   break;
                    svopid.user[u - 1] = *s++;
                    svopid.user[u] = 0;
                  }

              continue;
            }

         if (!strcmp(s, ":crypt,"))
            {
              // ignored
              continue;
            }

         // invalid tag (probably next entry). restore file position
         //
         fseek(file, filepos, SEEK_SET);
         break;
       }

   return loc;
}
//-----------------------------------------------------------------------------
const char *
ProcessorID::read_procauth(FILE * file, ProcAuth & procauth, int & line)
{
const char * loc = 0;

   for (line = 1;; ++line)
       {
         char buffer[200];
         const long filepos = ftell(file);   // remember file position
         char * s = fgets(buffer, sizeof(buffer) - 1, file);

         if (s == 0)   break;   // end of file


         // skip leading whitespaces
         //
         while (*s && *s < ' ')   ++s;

         if (*s == '*')   continue;   // comment line
         if (*s == 0)     continue;   // empty line

         if (!strcmp(s, ":rsvopid,"))
            {
              // skip tag and remove trailing whitespaces
              //
              s += 8;   // leave comma
              char * e = s + strlen(s);
              while (e > s && *e > 0 && *e < ' ')   *--e = 0;

              for (unsigned int id; s;)
                  {
                    ++s;   // skip comma
                    if (1 != sscanf(s, "%u", &id))   break;
                    
                    procauth.rsvopid.push_back(id);
                    s = strchr(s, ',');
                  }
              continue;
            }

         // invalid tag (probably next entry). restore file position
         //
         fseek(file, filepos, SEEK_SET);
         break;
       }

   return loc;
}
//-----------------------------------------------------------------------------
int
ProcessorID::read_network_profile()
{
const char * filename = getenv("APL2SVPPRF");

   if (filename)
      {
        return read_network_profile(filename);
      }
   else
      {
        string fname = get_APL_bin_path();
        fname += "/apl2svp.prf";
        return read_network_profile(fname.c_str());
      }
}
//-----------------------------------------------------------------------------
int
ProcessorID::read_network_profile(const char * filename)
{
FILE * file = fopen(filename, "r");
int line = 0;
const char * loc = 0;

   if (file == 0)
      {
        CERR << "Cannot open network profile '" << filename << "'" << endl;
        return 1;
      }

   network_profile.clear();
   for (line = 1;; ++line)
       {
         char buffer[200];
         const char * s = fgets(buffer, sizeof(buffer) - 1, file);

         if (s == 0)   // end of file
            {
              line = 0;
              break;
            }

         // skip leading whitespaces
         //
         while (*s && *s < ' ')   ++s;

        if (*s == '*')   continue;   // comment line
        if (*s == 0)     continue;   // empty line

        if (!strcmp(s, ":svopid,"))
           {
             SvoPid svopid;
              if (1 != sscanf(s, ":svopid,%u", &svopid.svopid))
                 { loc = LOC;   break; }

             loc = read_svopid(file, svopid, line);
             if (loc)   break;

             network_profile.svo_pids.push_back(svopid);
             continue;
           }

        if (!strcmp(s, ":procauth,"))
           {
             ProcAuth procauth;
             int i = NO_AP;
             int j = AP_NULL;
             int k = AP_NULL;
             const int count = sscanf(s, ":procauth,%u,%u,%u", &i, &j, &k);
             if (count < 1)   { loc = LOC;   break; }
             if (count > 3)   { loc = LOC;   break; }

             procauth.id = AP_num3(AP_num(i), AP_num(j), AP_num(k));

             loc = read_procauth(file, procauth, line);
             if (loc)   break;

             network_profile.proc_auths.push_back(procauth);
             continue;
           }

        loc = LOC;
        break;
       }

   fclose(file);

   if (loc)
      {
        CERR << "Syntax error in network profile '" << filename
             << "' line " << line << ", detected at " << loc << endl;
      }

   return line;
}
//-----------------------------------------------------------------------------
void
ProcessorID::disconnect()
{
   if (APnnn_socket.is_valid())
      {
        DISCONNECT_c signal(APnnn_socket);
        APnnn_socket.udp_close();
      }
}
//-----------------------------------------------------------------------------
