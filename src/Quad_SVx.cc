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
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "CDR.hh"
#include "CharCell.hh"
#include "Common.hh"
#include "FloatCell.hh"
#include "IntCell.hh"
#include "LibPaths.hh"
#include "ProcessorID.hh"
#include "Quad_SVx.hh"
#include "Svar_DB.hh"
#include "Svar_signals.hh"
#include "UserPreferences.hh"
#include "Workspace.hh"

extern char **environ;

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

// shared variable function instances
//
Quad_SVC Quad_SVC::_fun;
Quad_SVO Quad_SVO::_fun;
Quad_SVQ Quad_SVQ::_fun;
Quad_SVR Quad_SVR::_fun;
Quad_SVS Quad_SVS::_fun;

// shared variable function pointers
//
Quad_SVC * Quad_SVC::fun = & Quad_SVC::_fun;
Quad_SVO * Quad_SVO::fun = & Quad_SVO::_fun;
Quad_SVQ * Quad_SVQ::fun = & Quad_SVQ::_fun;
Quad_SVR * Quad_SVR::fun = & Quad_SVR::_fun;
Quad_SVS * Quad_SVS::fun = & Quad_SVS::_fun;

APL_time_us Quad_SVE::timer_end = 0;

//=============================================================================
// there is a different prog_name() function around (in APserver) so we
// declare the one for the APL interpreter here
const char *
prog_name()
{
   return "apl";
}

//=============================================================================
TCP_socket get_TCP_for_key(SV_key key)
{
   return Svar_DB::get_DB_tcp();
}
//=============================================================================
/**
    return true iff \b filename is an executable file
 **/
bool
Quad_SVx::is_executable(const char * file_and_args)
{
string filename(file_and_args);
const char * end = strchr(file_and_args, ' ');
   if (end)   filename.resize(end - file_and_args);

   return access(filename.c_str(), X_OK) == 0;
}
//-----------------------------------------------------------------------------
void
Quad_SVx::start_AP(AP_num ap)
{
   if (!uprefs.system_do_svars)   // something went wrong
      {
        if (uprefs.user_do_svars)   // user wanted APs
           {
             // user wanted APs, but something went wrong
             //
             CERR << "*** Not starting AP << ap because the connecton to "
                     " APserver has failed earlier." << endl;
           }
        else                        // user gave --noSV
           {
             CERR << "*** Not starting AP " << ap
                  << " because --noSV (or equivalent) was given." << endl
                  << " That conflicts with the use of ⎕SVxxx functions "
                     "and variables." << endl;
           }
        ATTENTION;
      }

const char * dirs[] = { "", "/APs" };
   enum { dircount = sizeof(dirs) / sizeof(*dirs) };

const AP_num own_ID = ProcessorID::get_own_ID();
const AP_num par_ID = ProcessorID::get_parent_ID();

const char * verbose = "";
   Log(LOG_shared_variables)   verbose = " -v";

char filename[PATH_MAX + 1];

bool found_executable = false;
   for (int d = 0; d < dircount; ++d)
       {
         snprintf(filename, PATH_MAX,
                  "%s%s/AP%u --id %u --par %u --gra %u --auto%s",
                  LibPaths::get_APL_bin_path(), dirs[d], ap, ap,
                  own_ID, par_ID, verbose);

         found_executable = is_executable(filename);
         if (found_executable)   break;
       }

   if (!found_executable)
      {
        CERR << "No binary found for AP " << ap << " (interpreter path = "
             << LibPaths::get_APL_bin_path() << ")" << endl;
        return;
      }

FILE * fp = popen(filename, "r");
   if (fp == 0)
      {
        CERR << "popen(" << filename << " failed: " << strerror(errno)
             << endl;
        return;
      }

   for (int cc; (cc = getc(fp)) != EOF;)   CERR << (char)cc;
   CERR << endl;

   pclose(fp);

   usleep(100000);   // give new AP time to register with APserver
}
//=============================================================================
Token
Quad_SVC::eval_AB(Value_P A, Value_P B)
{
   if (A->get_rank() > 2)   RANK_ERROR;
   if (B->get_rank() > 2)   RANK_ERROR;

const ShapeItem var_count = B->get_rows();
   if (A->get_rows() != var_count)               LENGTH_ERROR;
   if (A->get_rank() > 0 && A->get_cols() != 4)   LENGTH_ERROR;
vector<UCS_string> vars(var_count);
   B->to_varnames(vars, false);

const APL_Float qct = Workspace::get_CT();
const Cell * cA = &A->get_ravel(0);

Shape sh_Z;
   if (var_count > 1)   sh_Z.add_shape_item(var_count);
   sh_Z.add_shape_item(4);
Value_P Z(sh_Z, LOC);

   loop(z, var_count)
      {
        // construct control value
        //
        int ctl = 0;
        if (A->is_scalar())
           {
             const bool val = A->get_ravel(0).get_near_bool(qct);
              ctl = val ? ALL_SVAR_CONTROLS : NO_SVAR_CONTROL;
           }
        else if (A->is_vector())
           {
             if (A->get_ravel(0).get_near_bool(qct))   ctl |= SET_BY_1;
             if (A->get_ravel(1).get_near_bool(qct))   ctl |= SET_BY_2;
             if (A->get_ravel(2).get_near_bool(qct))   ctl |= USE_BY_1;
             if (A->get_ravel(3).get_near_bool(qct))   ctl |= USE_BY_2;
           }
        else // matrix
           {
             if (cA++->get_near_bool(qct))   ctl |= SET_BY_1;
             if (cA++->get_near_bool(qct))   ctl |= SET_BY_2;
             if (cA++->get_near_bool(qct))   ctl |= USE_BY_1;
             if (cA++->get_near_bool(qct))   ctl |= USE_BY_2;
           }

        // set control.
        //
        Symbol * sym = Workspace::lookup_existing_symbol(vars[z]);
        if (sym == 0)   throw_symbol_error(vars[z], LOC);

        const SV_key key = sym->get_SV_key();
        Svar_DB::set_control(key, Svar_Control(ctl));
        ctl = Svar_DB::get_control(key);

        new (Z->next_ravel())   IntCell(ctl & SET_BY_1 ? 1 : 0);
        new (Z->next_ravel())   IntCell(ctl & SET_BY_2 ? 1 : 0);
        new (Z->next_ravel())   IntCell(ctl & USE_BY_1 ? 1 : 0);
        new (Z->next_ravel())   IntCell(ctl & USE_BY_2 ? 1 : 0);
      }

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Token
Quad_SVC::eval_B(Value_P B)
{
   if (B->get_rank() > 2)   RANK_ERROR;

const ShapeItem var_count = B->get_rows();
vector<UCS_string> vars(var_count);
   B->to_varnames(vars, false);

Shape sh_Z;
   if (var_count > 1)   sh_Z.add_shape_item(var_count);
   sh_Z.add_shape_item(4);
Value_P Z(sh_Z, LOC);

   loop(z, var_count)
      {
        Symbol * sym = Workspace::lookup_existing_symbol(vars[z]);
        if (sym == 0)   throw_symbol_error(vars[z], LOC);

        const SV_key key = sym->get_SV_key();
        const Svar_Control control = Svar_DB::get_control(key);

        new (Z->next_ravel())   IntCell(control & SET_BY_1 ? 1 : 0);
        new (Z->next_ravel())   IntCell(control & SET_BY_2 ? 1 : 0);
        new (Z->next_ravel())   IntCell(control & USE_BY_1 ? 1 : 0);
        new (Z->next_ravel())   IntCell(control & USE_BY_2 ? 1 : 0);
      }

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//=============================================================================
Quad_SVE::Quad_SVE()
   : NL_SystemVariable(ID::Quad_SVE)
{
   Symbol::assign(IntScalar(0, LOC), LOC);
}
//-----------------------------------------------------------------------------
void
Quad_SVE::assign(Value_P value, const char * loc)
{
   // ⎕SVE←X merely remembers the timer expiration time. The timer is only
   // started if ⎕SVE is referenced (in Quad_SVE::get_apl_value())
   //
   if (!value->is_scalar())   RANK_ERROR;

const APL_time_us duration = 1000000 * value->get_ravel(0).get_real_value();
   if (duration < 0)   DOMAIN_ERROR;

   if (duration == 0.0)
      {
        // ⎕SVE←0 causes get_apl_value() to return immediately. We achieve
        // that by setting the end time to the past.
        //
        timer_end = now() - 1;   // end time in the past
        return;
      }

   // set timer end
   //
   timer_end = now() + duration;
}
//-----------------------------------------------------------------------------
Value_P
Quad_SVE::get_apl_value() const
{
   for (;;)
       {
         Svar_event events = SVE_NO_EVENTS;
         SV_key key = Svar_DB::get_events(events, ProcessorID::get_id());

          if (key || (events != SVE_NO_EVENTS))   break; // something happend

         if (timer_end < now())   // timer expired: return 0
            {
              Svar_DB::clear_all_events(ProcessorID::get_id());
              return IntScalar(0, LOC);
            }

          usleep(50000);  // wait 50 ms
       }

   // referencing ⎕SVE always clears all events
   //
   Svar_DB::clear_all_events(ProcessorID::get_id());

   // we have got an event; return remaining time.
   //
const APL_Float remaining = timer_end - now();

   if (remaining < 0)   return IntScalar(0, LOC);

Value_P Z(LOC);
   new (&Z->get_ravel(0))   FloatCell(0.000001 * remaining);
   return Z;
}
//=============================================================================
/**
 ** Offer variables in B to corresponding processors in A
 **/
Token
Quad_SVO::eval_AB(Value_P A, Value_P B)
{
   if (A->get_rank() > 1)   RANK_ERROR;
   if (B->get_rank() > 2)   RANK_ERROR;

const ShapeItem var_count = B->get_rows();
vector<UCS_string> apl_vars(var_count);
   B->to_varnames(apl_vars,   false);

vector<UCS_string> surrogates(var_count);
   B->to_varnames(surrogates, true);

   if (A->get_rank() == 1 && A->element_count() != var_count)   LENGTH_ERROR;

Shape sh_Z;
   if (var_count > 1)   sh_Z.add_shape_item(var_count);
Value_P Z(sh_Z, LOC);

   loop(z, var_count)
      {
        ShapeItem a = A->is_scalar() ? 0 : z;
        uint32_t vname[MAX_SVAR_NAMELEN + 1];
        const int vlen = surrogates[z].size();
        if (vlen > MAX_SVAR_NAMELEN)   DOMAIN_ERROR;
        bool bad_name = false;

        loop(v, vlen)
           {
             const Unicode uni = surrogates[z][v];
             if (!Avec::is_symbol_char(uni))   bad_name = true;
             vname[v] = uni;
           }
        vname[vlen] = 0;

        if (bad_name || vlen == 0)
           {
             new (Z->next_ravel()) IntCell(NO_COUPLING);
             continue;
           }

        // make sure current name class allows ⎕SVO
        //
        Symbol * sym = Workspace::lookup_symbol(apl_vars[z]);
        assert(sym);

        const AP_num proc = AP_num(A->get_ravel(a).get_int_value());

        ValueStackItem * vsp = sym->top_of_stack();
        Assert(vsp);

        switch(sym->get_nc())
           {
             case NC_UNUSED_USER_NAME:
             case NC_VARIABLE:
                  break;

             case NC_SHARED_VAR:
                  {
                    // CERR << "Shared variable " << apl_vars[z]
                    //      << " is already shared" << endl;

                    const SV_key key = sym->get_SV_key();
                    new (Z->next_ravel()) IntCell(Svar_DB::get_coupling(key));
                  }
                  continue;   // next z

             default: // function etc.
                  Q1(sym->get_nc())   DOMAIN_ERROR;
           }

        SV_Coupling coupling = NO_COUPLING;
        const SV_key key = share_one_variable(proc, vname, coupling);
        if (coupling >= SV_OFFERED)   // offered or coupled
           {
             sym->share_var(key);
           }

        new (Z->next_ravel()) IntCell(coupling);
      }

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
SV_key
Quad_SVO::share_one_variable(AP_num to_ap, const uint32_t * vname,
                             SV_Coupling & coupling)
{
   // if to_ap is a (dependent) AP then start it if necessary...
   //
AP_num3 to_proc(to_ap, ProcessorID::get_own_ID(), ProcessorID::get_parent_ID());
bool auto_started = false;

   if (to_ap && to_ap < AP_FIRST_USER)   // to_proc is an AP
      {
        if (!Svar_DB::is_registered_id(to_proc))
           {
             start_AP(to_ap);
             auto_started = true;
           }
      }
   else                         // to_proc is an independent processor
      {
        new (&to_proc)   AP_num3(to_ap);
      }

Svar_partner from(ProcessorID::get_id(), NO_TCP_SOCKET);

const SV_key key = Svar_DB::match_or_make(vname, to_proc, from);
   coupling = Svar_DB::get_coupling(key);

   if (coupling == SV_COUPLED)
      {
        // if an AP was started automatically, then coupling could be
        // SV_COUPLED or SV_OFFERED, depending on how fast the AP did start
        // up. We pretend here that it started slowly so that auto-started
        // APs always return SV_OFFERED
        //
        if (auto_started)   coupling = SV_OFFERED;
        
        return key;
      }

   // if something went wrong, complain.
   //
   if (coupling == NO_COUPLING)
      {
        Log(LOG_shared_variables)
           CERR << "Svar_DB::match_or_store() returned unexpected 0" << endl;
        return 0;
      }

   Assert(coupling == SV_OFFERED);

   return key;
}
//-----------------------------------------------------------------------------
/**
 ** Return degree of coupling for variables in B
 **/
Token
Quad_SVO::eval_B(Value_P B)
{
   if (B->get_rank() > 2)   RANK_ERROR;

const ShapeItem var_count = B->get_rows();
vector<UCS_string> vars(var_count);
   B->to_varnames(vars, false);

Shape sh_Z;
   if (var_count > 1)   sh_Z.add_shape_item(var_count);
Value_P Z(sh_Z, LOC);

   loop(z, var_count)
      {
        Symbol * sym = Workspace::lookup_existing_symbol(vars[z]);
        if (sym == 0)   // variable does not exist
           {
             new (Z->next_ravel()) IntCell(0);
             continue;
           }

        // the coupling of a non-shared variables has coupling 0
        //
        if (sym->get_nc() != NC_SHARED_VAR)
           {
             new (Z->next_ravel()) IntCell(0);
             continue;
           }

        const SV_key key = sym->get_SV_key();
        const SV_Coupling coupling = Svar_DB::get_coupling(key);
         new (Z->next_ravel()) IntCell(coupling);
      }

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//=============================================================================
Token
Quad_SVQ::eval_B(Value_P B)
{
   if (B->get_rank() > 1)        RANK_ERROR;
   if (B->element_count() > 1)   LENGTH_ERROR;

Value_P Z;

   // B is empty or has 1 element.
   //
   if (B->element_count()  == 0)    // B empty: return processors offering
      {
        Z = get_processors();
      }
   else                            // return variables offered by processor ↑B
      {
        const AP_num proc = AP_num(B->get_ravel(0).get_int_value());
        Z = get_variables(proc);
      }

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
Value_P
Quad_SVQ::get_processors()
{
   // shared variable processors are identified by one of the following:
   //
   // 1. auxiliary processors that can be started on demand
   //    (e.g. AP100, AP210). These APs are collocated with the apl binary,
   //    either in the same directory or in the 'APs' subdirectory,
   //
   // 2. running processors that have offered variables in Svar_DB.
   //
vector<AP_num> processors;

   // case 1...
   //
const char * dirs[] = { "", "/APs" };
   enum { dircount = sizeof(dirs) / sizeof(*dirs) };

   for (int d = 0; d < dircount; ++d)
       {
         char dirname[PATH_MAX + 1];
         snprintf(dirname, PATH_MAX, "%s%s", LibPaths::get_APL_bin_path(),
                  dirs[d]);
         dirname[PATH_MAX] = 0;
         DIR * dir = opendir(dirname);

         // the APs directory only exists below the src directory (i.e when
         // apl is started locally without installing it. We do not complain
         // if the APs directory does not exist.
         //
         if (dir == 0)
            {
              if (*dirs[d] == 0)
                 {
                   CERR << "Could not open directory " << dirname << " : "
                        << strerror(errno) << " at " << LOC << endl;
                 }
              continue;
            }

         for (;;)   // scan files in directory
             {
                dirent * entry = readdir(dir);
                if (entry == 0)   break;   // directory done

#ifdef _DIRENT_HAVE_D_TYPE
                if (entry->d_type != DT_REG)   continue; // not a regular file
#endif

                char filename[PATH_MAX + 1];
                snprintf(filename, PATH_MAX, "%s/%s", dirname, entry->d_name);
                filename[PATH_MAX] = 0;

                if (!is_executable(filename))   continue;

                int apnum;
                if (sscanf(entry->d_name, "AP%u", &apnum) != 1)   continue;

                char expected[PATH_MAX + 1];
                snprintf(expected, sizeof(expected), "AP%u", apnum);
                if (strcmp(entry->d_name, expected))   continue;

                processors.push_back((AP_num)apnum);
             }

         closedir(dir);
       }

   // case 2...
   //
   Svar_DB::get_offering_processors(ProcessorID::get_own_ID(), processors);

   // sort and remove duplicates
   //
vector<int32_t> sorted;
   while (processors.size())
      {
        // find smallest
        //
        int smallest = processors[0];
        for (int s = 1; s < processors.size(); ++s)
            if (smallest > processors[s])   smallest = processors[s];

       // add smallest to sorted
       //
       sorted.push_back(smallest);

       // remove smallest from processors
       //
        for (int s = 0; s < processors.size();)
            {
              if (processors[s] != smallest)
                 {
                   ++s;
                 }
              else
                 {
                   processors[s] = processors.back();
                   processors.pop_back();
                 }
            }
      }

Value_P Z(sorted.size(), LOC);
   loop(z, sorted.size())   new (&Z->get_ravel(z)) IntCell(sorted[z]);
   return Z;
}
//-----------------------------------------------------------------------------
Value_P
Quad_SVQ::get_variables(AP_num proc)
{
vector<uint32_t> varnames;
   Svar_DB::get_offered_variables(ProcessorID::get_own_ID(), proc, varnames);

   // varnames is a sequence of 0-terminated Unicodes
   //
vector<int> var_lengths;   // including terminating 0
int last_zero = -1;
   loop(v, varnames.size())
      {
        if (varnames[v] == 0)
           {
             var_lengths.push_back(v + 1 - last_zero);
             last_zero = v;
           }
        
      }

const int count = var_lengths.size();
ShapeItem max_len = 0;
   loop(z, count)
       if (max_len < (var_lengths[z] - 1))   max_len = var_lengths[z] - 1;

const Shape shZ(count, max_len);
Value_P Z(shZ, LOC);
int v = 0;

   loop(z, count)
   loop(c, max_len)
      {
        if (c < var_lengths[z])
           {
             if (varnames[v] == 0)   ++v;
             new (Z->next_ravel()) CharCell(Unicode(varnames[v++]));
           }
        else
           {
             new (Z->next_ravel()) CharCell(UNI_ASCII_SPACE);
           }
      }

   Z->set_default_Spc();   // prototype: character
   return Z;
}
//=============================================================================
Token
Quad_SVR::eval_B(Value_P B)
{
   if (B->get_rank() > 2)   RANK_ERROR;

const ShapeItem var_count = B->get_rows();
vector<UCS_string> vars(var_count);
   B->to_varnames(vars, false);

Shape sh_Z;
   if (var_count > 1)   sh_Z.add_shape_item(var_count);
Value_P Z(sh_Z, LOC);

   loop(z, var_count)
      {
        Symbol * sym = Workspace::lookup_existing_symbol(vars[z]);
        if (sym == 0)   DOMAIN_ERROR;

        const SV_Coupling coupling = sym->unshare_var();
        new (Z->next_ravel()) IntCell(coupling);
      }

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//=============================================================================
Token
Quad_SVS::eval_B(Value_P B)
{
   if (B->get_rank() > 2)   RANK_ERROR;

const ShapeItem var_count = B->get_rows();
vector<UCS_string> vars(var_count);
   B->to_varnames(vars, false);

Shape sh_Z;
   if (var_count > 1)   sh_Z.add_shape_item(var_count);
   sh_Z.add_shape_item(4);
Value_P Z(sh_Z, LOC);

   loop(z, var_count)
      {
        Symbol * sym = Workspace::lookup_existing_symbol(vars[z]);
        if (sym == 0)   throw_symbol_error(vars[z], LOC);

        const SV_key key = sym->get_SV_key();
        const Svar_state state = Svar_DB::get_state(key);

        new (Z->next_ravel())   IntCell(state & SET_BY_1 ? 1 : 0);
        new (Z->next_ravel())   IntCell(state & SET_BY_2 ? 1 : 0);
        new (Z->next_ravel())   IntCell(state & USE_BY_1 ? 1 : 0);
        new (Z->next_ravel())   IntCell(state & USE_BY_2 ? 1 : 0);
      }

   Z->check_value(LOC);
   return Token(TOK_APL_VALUE1, Z);
}
//=============================================================================
