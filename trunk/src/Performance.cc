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

#define perfo_1(id, ab, _name, _thr) \
   CellFunctionStatistics Performance::cfs_ ## id ## ab(PFS_## id ## ab);
#define perfo_2(id, ab, _name, _thr) \
   CellFunctionStatistics Performance::cfs_ ## id ## ab(PFS_ ## id ## ab);
#define perfo_3(id, ab, _name, _thr) \
   FunctionStatistics Performance::fs_ ## id ## ab (PFS_ ## id ## ab);
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
#define perfo_1(id, ab, _name, _thr)   case PFS_ ## id ## ab:   return &cfs_ ## id ## ab;
#define perfo_2(id, ab, _name, _thr)   case PFS_ ## id ## ab:   return &cfs_ ## id ## ab;
#define perfo_3(id, ab, name, _thr)    case PFS_ ## id ## ab:   return &fs_ ## id ## ab;
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
#define perfo_1(id, ab, _name, _thr)   case PFS_ ## id ## ab:   return 1;
#define perfo_2(id, ab, _name, _thr)   case PFS_ ## id ## ab:   return 2;
#define perfo_3(id, ab, _name, _thr)   case PFS_ ## id ## ab:   return 3;
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
#define perfo_1(id, ab, name, _thr)   case PFS_ ## id ## ab:   return name;
#define perfo_2(id, ab, name, _thr)   case PFS_ ## id ## ab:   return name;
#define perfo_3(id, ab, name, _thr)   case PFS_ ## id ## ab:   return name;
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
   if (which != PFS_ALL)   // individual statistics
      {
         Statistics * stat = get_statistics(which);
         if (!stat)
            {
              out << "No such statistics: " << which << endl;
              return;
            }

         if (which < PFS_MAX2)   // cell function statistics
            {
              out <<
"╔═════════════════╦══════════╤═════════╤════════╦══════════╤═════════╤════════╗"
                  << endl;
              stat->print(out);
              out <<
"╚═════════════════╩══════════╧═════════╧════════╩══════════╧═════════╧════════╝"
                  << endl;
            }
         else
            {
              out <<
"╔═════════════════╦════════════╤══════════╤══════════╤══════════╤══════════╗"
                  << endl;
              stat->print(out);
              out <<
"╚═════════════════╩════════════╧══════════╧══════════╧══════════╧══════════╝"
                   << endl;
            }
         return;
      }

   // print all statistics
   //
   out <<
"╔═════════════════════════════════════════════════════════════════════════════╗\n"
"║                      Performance Statistics (CPU cycles)                    ║\n"
"╠═════════════════╦═════════════════════════════╦═════════════════════════════╣\n"
"║                 ║         first pass          ║       subsequent passes     ║\n"
"║      Cell       ╟──────────┬─────────┬────────╫──────────┬─────────┬────────╢\n"
"║    Function     ║        N │ ⌀cycles │  σ÷μ % ║        N │ ⌀cycles │  σ÷μ % ║\n"
"╠═════════════════╬══════════╪═════════╪════════╬══════════╪═════════╪════════╣\n";


#define perfo_1(id, ab, _name, _thr)   cfs_ ## id ## ab.print(out);
#define perfo_2(id, ab, _name, _thr)   cfs_ ## id ## ab.print(out);
#define perfo_3(id, ab, _name, _thr)
#include "Performance.def"

   out <<
"╚═════════════════╩══════════╧═════════╧════════╩══════════╧═════════╧════════╝\n"
"╔═════════════════╦════════════╤══════════╤══════════╤══════════╤══════════╗\n"
"║    Function     ║            │        N │  ⌀ VLEN  │ ⌀ cycles │ cyc÷VLEN ║\n"
"╟─────────────────╫────────────┼──────────┼──────────┼──────────┼──────────╢"
       << endl;

   // subtract cell statistics from function statistics
   //
uint64_t cycles_B = fs_SCALAR_B .get_data().get_sum ();
uint64_t count1_B = 0;
uint64_t countN_B = 0;

uint64_t cycles_AB = fs_SCALAR_AB.get_data().get_sum ();
uint64_t count1_AB = 0;
uint64_t countN_AB = 0;

#define perfo_1(id, ab, _name, _thr)                                  \
                           cycles_B   -= cfs_ ## id ## ab.get_sum();  \
                           count1_B += cfs_ ## id ## ab.get_count1(); \
                           countN_B += cfs_ ## id ## ab.get_countN();

#define perfo_2(id, ab, _name, _thr)                                   \
                           cycles_AB  -= cfs_ ## id ## ab.get_sum();   \
                           count1_AB += cfs_ ## id ## ab.get_count1(); \
                           countN_AB += cfs_ ## id ## ab.get_countN();

#define perfo_3(id, ab, _name, _thr)
#include "Performance.def"

   {
     const uint64_t vlen_B = count1_B ? (count1_B + countN_B)/count1_B : 1;
     const uint64_t div1_B  = count1_B ? count1_B : 1;
     const uint64_t div2_B  = count1_B + countN_B ? count1_B + countN_B : 1;

     out << "║   f B overhead  ║ " << right
         <<         setw(10) << cycles_B
         << " │ " << setw(8) << fs_SCALAR_B.get_data().get_count()
         << " │ " << setw(8) << vlen_B
         << " │ " << setw(8) << cycles_B/div1_B
         << " │ " << setw(8) << cycles_B/div2_B
         << " ║"  << endl;
   }

   {
     const uint64_t vlen_AB = count1_AB ? (count1_AB + countN_AB)/count1_AB : 1;
     const uint64_t div1_AB = count1_AB ? count1_AB : 1;
     const uint64_t div2_AB = count1_AB + countN_AB ? count1_AB + countN_AB : 1;
     out << "║ A f B overhead  ║ "
         <<         setw(10) << cycles_AB
         << " │ " << setw(8) << fs_SCALAR_AB.get_data().get_count()
         << " │ " << setw(8) << vlen_AB
         << " │ " << setw(8) << cycles_AB/div1_AB
         << " │ " << setw(8) << cycles_AB/div2_AB
         << " ║"  << left << endl;
   }

#define perfo_1(id, ab, _name, _thr)
#define perfo_2(id, ab, _name, _thr)
#define perfo_3(id, ab, _name, _thr) fs_ ## id ## ab.print(out);

#include "Performance.def"

   out <<
"╚═════════════════╩════════════╧══════════╧══════════╧══════════╧══════════╝"
       << endl;
}
//----------------------------------------------------------------------------
void
Performance::save_data(ostream & out, ostream & out_file)
{
#define perfo_1(id, ab, _name, _thr)   cfs_ ## id ## ab.save_data(out_file, #id);
#define perfo_2(id, ab, _name, _thr)   cfs_ ## id ## ab.save_data(out_file, #id);
#define perfo_3(id, ab, _name, _thr)   fs_ ## id ## ab.save_data(out_file, #id);
#include "Performance.def"
}
//----------------------------------------------------------------------------
void
Performance::reset_all()
{
#define perfo_1(id, ab, _name, _thr)   cfs_ ## id ## ab.reset();
#define perfo_2(id, ab, _name, _thr)   cfs_ ## id ## ab.reset();
#define perfo_3(id, ab, _name, _thr)   fs_ ## id ## ab.reset();
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

const uint64_t div = vec_lengths.get_average() ? vec_lengths.get_average() : 1;

   out << " ║ " << right
       <<         setw(10) << vec_cycles.get_sum()
       << " │ " << setw(8) << vec_lengths.get_count()
       << " │ " << setw(8) << vec_lengths.get_average() 
       << " │ " << setw(8) << vec_cycles.get_average()
       << " │ " << setw(8) << (vec_cycles.get_average() / div)
       << left << " ║" << endl;
}
//----------------------------------------------------------------------------
void
FunctionStatistics::save_data(ostream & outf, const char * id_name)
{
char cc[100];
   snprintf(cc, sizeof(cc), "%s,", id_name);
   outf << "prf_3 (PFS_" << left << setw(12) << cc << right;
   vec_cycles.save_record(outf);
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
   out << "    ║";

   first.print(out, 9, 7);
   out << " ║";
   subsequent.print(out, 9, 7);
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

