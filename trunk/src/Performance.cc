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
#include "UCS_string.hh"

#define perfo(id, name)   \
CellFunctionStatistics Performance::cfs_##id (PFS_##id, name);
#include "Performance.def"

//----------------------------------------------------------------------------
Statistics::Statistics()
{
}
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
#define perfo(id, _name)   case PFS_ ## id:   return &cfs_ ## id;
#include "Performance.def"
        default: return 0;
      }

   // not reached
   return 0;
}
//----------------------------------------------------------------------------
void
Performance::print(Pfstat_ID which, ostream & out)
{
   // figure longest statistics name
   //
int max_len = 0;

#define perfo(id, _name)                           \
   { const char * name = cfs_ ## id.get_name();    \
     UTF8_string utf(name);  UCS_string ucs(utf);  \
     if (max_len < ucs.size())   max_len = ucs.size(); }
#include "Performance.def"

   out <<
"----------------------------------------------------------------------------"
       << endl << "         Performance Statistics (CPU cycles)" << endl <<
"Fun      Pass 1    μ         σ           Pass N    μ         σ" << endl <<
"----------------------------------------------------------------------------"
       << endl;

   if (which == -1)   // all
      {
#define perfo(id, _name)   cfs_##id.print(out, max_len);
#include "Performance.def"
      }
   else
      {
         Statistics * stat = get_statistics(which);
         if (!stat)
            {
              out << "No such statistics: " << which << endl;
              return;
            }

         stat->print(out, max_len);
      }

   out <<
"============================================================================"
       << endl;
}
//----------------------------------------------------------------------------
void
Performance::reset_all()
{
#define perfo(id, _name)   cfs_##id.reset();
#include "Performance.def"
}
//----------------------------------------------------------------------------
void
CellFunctionStatistics::print(ostream & out, int max_namelen)
{
UTF8_string utf(name);
UCS_string uname(utf);
   out << name << ":   ";
   loop(n, max_namelen - uname.size())   out << " ";

   first.print(out);
   out << "  ";
   subsequent.print(out);
   out << endl;
}
//----------------------------------------------------------------------------
void
Statistics_record::print(ostream & out)
{
uint64_t mu = 0;
double sigma = 0;
   if (count)
      {
        mu = data/count;
        sigma = sqrt(data2/count - mu*mu);
      }

   out << left << setw(10) << count
               << setw(10) << mu
               << setw(10) << sigma << right;
}
//----------------------------------------------------------------------------

