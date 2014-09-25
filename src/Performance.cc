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

#include <iomanip>

#include <math.h>

#include "Common.hh"
#include "Performance.hh"
#include "PrintOperator.hh"
#include "UCS_string.hh"

#define perfo_1(id, _name, _thr) CellFunctionStatistics Performance::cfs_##id (PFS_##id);
#define perfo_2(id, _name, _thr) CellFunctionStatistics Performance::cfs_##id (PFS_##id);
#define perfo_3(id, _name) FunctionStatistics Performance::fs_##id (PFS_##id);
#include "Performance.def"

//----------------------------------------------------------------------------
Statistics::~Statistics()
{
}
//----------------------------------------------------------------------------
Statistics *
Performance::get_statistics(Pfstat_ID id)
{
   switch(id)
      {
#define perfo_1(id, _name, _thr)   case PFS_ ## id:   return &cfs_ ## id;
#define perfo_2(id, _name, _thr)   case PFS_ ## id:   return &cfs_ ## id;
#define perfo_3(id, _name)   case PFS_ ## id:   return &fs_ ## id;
#include "Performance.def"
        default: return 0;
      }

   // not reached
   return 0;
}
//----------------------------------------------------------------------------
int
Performance::get_statistics_type(Pfstat_ID id)
{
   switch(id)
      {
#define perfo_1(id, _name, _thr)   case PFS_ ## id:   return 1;
#define perfo_2(id, _name, _thr)   case PFS_ ## id:   return 2;
#define perfo_3(id, _name)   case PFS_ ## id:   return 3;
#include "Performance.def"
        default: return 0;
      }

   // not reached
   return 0;
}
//----------------------------------------------------------------------------
const char *
Statistics::get_name(Pfstat_ID id)
{
   switch(id)
      {
#define perfo_1(id, name, _thr)   case PFS_ ## id:   return name;
#define perfo_2(id, name, _thr)   case PFS_ ## id:   return name;
#define perfo_3(id, name)   case PFS_ ## id:   return name;
#include "Performance.def"
        case PFS_SCALAR_B_overhead:  return "  f B overhead";
        case PFS_SCALAR_AB_overhead: return "A f B overhead";
        default: return "Unknown Pfstat_ID";
      }

   // not reached
   return 0;
}
//----------------------------------------------------------------------------
void
Performance::print(Pfstat_ID which, ostream & out)
{
   out <<
"╔═════════════════════════════════════════════════════════════════════════════╗\n"
"║                      Performance Statistics (CPU cycles)                    ║\n"
"╠═════════════════╦═════════════════════════════╦═════════════════════════════╣\n"
"║                 ║         first pass          ║       subsequent passes     ║\n"
"║      Cell       ╟──────────┬─────────┬────────╫──────────┬─────────┬────────╢\n"
"║    Function     ║     N    │     μ   │  σ÷μ % ║     N    │     μ   │  σ÷μ % ║\n"
"╠═════════════════╬══════════╪═════════╪════════╬══════════╪═════════╪════════╣\n";

   if (which != PFS_ALL)   // one statistics
      {
         Statistics * stat = get_statistics(which);
         if (!stat)
            {
              out << "No such statistics: " << which << endl;
              return;
            }

         stat->print(out);
         out <<
"╚═════════════════╩═══════════╧════════╧════════╩══════════╧═════════╧════════╝"
             << endl;
         return;
      }

#define perfo_1(id, _name, _thr)   cfs_##id.print(out);
#define perfo_2(id, _name, _thr)   cfs_##id.print(out);
#define perfo_3(id, _name)
#include "Performance.def"

   out <<
"╚═════════════════╩══════════╧═════════╧════════╩══════════╧═════════╧════════╝\n"
"╔═════════════════╦══════════╤════════════╤════════╦══════════╗\n"
"║    Function     ║     N    │      μ     │  σ÷μ % ║ aver. ⍴, ║\n"
"╟─────────────────╫──────────┼────────────┼────────╫──────────╢"
       << endl;

   // subtract cell statistics from function statistics
   //
uint64_t data_B    = fs_SCALAR_B .get_data().get_sum ();
double   data_B2   = fs_SCALAR_B .get_data().get_sum2();
uint64_t lsum_B = 0;

uint64_t data_AB   = fs_SCALAR_AB.get_data().get_sum ();
double   data_AB2  = fs_SCALAR_AB.get_data().get_sum2();
uint64_t lsum_AB = 0;

#define perfo_1(id, _name, _thr) data_B   -= cfs_##id.get_sum();  \
                           data_B2  -= cfs_##id.get_sum2(); \
                           lsum_B   += cfs_##id.get_count();
#define perfo_2(id, _name, _thr) data_AB  -= cfs_##id.get_sum();  \
                           data_AB2 -= cfs_##id.get_sum2(); \
                           lsum_AB   += cfs_##id.get_count();
#define perfo_3(id, _name)
#include "Performance.def"

Statistics_record rec_B (fs_SCALAR_B.get_data().get_count(), data_B, data_B2);
Statistics_record rec_AB(fs_SCALAR_AB.get_data().get_count(),data_AB,data_AB2);

FunctionStatistics sca_B(PFS_SCALAR_B_overhead, rec_B, lsum_B);
FunctionStatistics sca_AB(PFS_SCALAR_AB_overhead, rec_AB, lsum_AB);

   sca_B.print(out);
   sca_AB.print(out);

#define perfo_1(id, _name, _thr)
#define perfo_2(id, _name, _thr)
#define perfo_3(id, _name) fs_ ## id.print(out);

#include "Performance.def"

   out <<
"╚═════════════════╩══════════╧════════════╧════════╩══════════╝"
       << endl;
}
//----------------------------------------------------------------------------
void
Performance::save_data(ostream & out, ostream & out_file)
{
#define perfo_1(id, _name, _thr)   cfs_##id.save_data(out_file, #id);
#define perfo_2(id, _name, _thr)   cfs_##id.save_data(out_file, #id);
#define perfo_3(id, _name)   fs_##id.save_data(out_file, #id);
#include "Performance.def"
}
//----------------------------------------------------------------------------
void
Performance::reset_all()
{
#define perfo_1(id, _name, _thr)   cfs_##id.reset();
#define perfo_2(id, _name, _thr)   cfs_##id.reset();
#define perfo_3(id, _name)   fs_##id.reset();
#include "Performance.def"
}
//----------------------------------------------------------------------------
void
Statistics_record::print(ostream & out, int l_N, int l_mu)
{
uint64_t mu = 0;
int sigma_percent = 0;

   if (count)
      {
        mu = data/count;
        const double sigma = sqrt(data2/count - mu*mu);
        sigma_percent = (int)((sigma/mu)*100);
      }

   out << setw(l_N) << right << count << " │ "
       << setw(l_mu) << mu << " │ "
       << setw(4) << sigma_percent << " %" << left;
}
//----------------------------------------------------------------------------
void
Statistics_record::save_record(ostream & outf)
{
uint64_t mu = 0;
double sigma = 0;
   if (count)
      {
        mu = data/count;
        sigma = sqrt(data2/count - mu*mu);
      }

   outf << setw(8) << count << ","
        << setw(8) << mu << ","
        << setw(8) << (uint64_t)(sigma + 0.5);
}
//============================================================================
void
FunctionStatistics::print(ostream & out)
{
UTF8_string utf(get_name());
UCS_string uname(utf);
   out << "║ " << utf;
   loop(n, 15 - uname.size())   out << " ";
   out << " ║ ";

uint64_t aver_len = 0;
   if (data.get_count())   aver_len = len_sum / data.get_count();

   data.print(out, 8, 10);
   out << " ║ ";
   out << right << setw(8) << aver_len << left;
   out << " ║" << endl;
}
//----------------------------------------------------------------------------
void
FunctionStatistics::save_data(ostream & outf, const char * id_name)
{
char cc[100];
   snprintf(cc, sizeof(cc), "%s,", id_name);
   outf << "prf_3 (PFS_" << left << setw(12) << cc << right;
   data.save_record(outf);
   outf << ")" << endl;
}
//============================================================================
void
CellFunctionStatistics::print(ostream & out)
{
UTF8_string utf(get_name());
UCS_string uname(utf);
   out << "║ " << uname;
   loop(n, 12 - uname.size())   out << " ";
   out << "    ║ ";

   first.print(out);
   out << " ║ ";
   subsequent.print(out);
   out << " ║" << endl;
}
//----------------------------------------------------------------------------
void
CellFunctionStatistics::save_data(ostream & outf, const char * id_name)
{
char cc[100];
   snprintf(cc, sizeof(cc), "%s,", id_name);
   outf << "prf_12(PFS_" << left << setw(12) << cc << right;
   first.save_record(outf);
   outf << ",";
   subsequent.save_record(outf);
   outf << ")" << endl;
}
//============================================================================

