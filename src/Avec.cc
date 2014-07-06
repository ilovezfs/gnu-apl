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

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "Avec.hh"
#include "Command.hh"
#include "Output.hh"
#include "Parser.hh"
#include "PrintOperator.hh"
#include "UTF8_string.hh"
#include "Value.icc"

using namespace std;

// See RFC 3629 for UTF-8

/// Various character attributes.
struct Character_definition
{
   CHT_Index     av_val;      ///< Atomic vector enum of the char.
   Unicode       unicode;     ///< Unicode of the char.
   const char *  char_name;   ///< Name of the char.
   int           def_line;    ///< Line where the char is defined.
   TokenTag      token_tag;   ///< Token tag for the char.
   CharacterFlag flags;       ///< Character class.
   int           av_pos;      ///< position in ⎕AV (== ⎕AF unicode)
};

//-----------------------------------------------------------------------------
#define char_def(n, _u, t, f, p) \
   { AV_ ## n, UNI_ ## n, # n, __LINE__, TOK_ ## t, FLG_ ## f, 0x ## p },
#define char_df1(_n, _u, _t, _f, _p)
const Character_definition character_table[MAX_AV] =
{
#include "Avec.def"
};
//-----------------------------------------------------------------------------
void
Avec::show_error_pos(int i, int line, bool cond, int def_line)
{
   if (!cond)
      {
        CERR << "Error index (line " << line << ", Avec.def line "
             << def_line << ") = " << i << " = " << HEX(i) << endl;
        Assert(0);
      }
}
//-----------------------------------------------------------------------------
#if 0

void
Avec::check_file(const char * filename)
{
const int fd = open(filename, O_RDONLY);
   if (fd == -1)   return;

   Log(LOG_startup)   CERR << "Checking " << filename << endl;

uint32_t datalen;
   {
     struct stat s;
     const int res = fstat(fd, &s);
     Assert(res == 0);
     datalen = s.st_size;
   }

const void * data = mmap(NULL, datalen, PROT_READ, MAP_PRIVATE, fd, 0);
   Assert(data != (void *)-1);

const UTF8_string utf = UTF8_string((const UTF8 *)data, datalen);
UCS_string ucs(utf);

   loop(i, ucs.size())
       {
         if (!is_known_char(ucs[i]))
            CERR << "APL char " << UNI(ucs[i]) << " is missing in AV ("
                 << i << ")" << endl;
       }

   close(fd);
}
#endif
//-----------------------------------------------------------------------------
void
Avec::init()
{
   check_av_table();
}
//-----------------------------------------------------------------------------
void
Avec::check_av_table()
{
   if (MAX_AV != 256)
      {
        CERR << "AV has " << MAX_AV << " entries (should be 256)" << endl;
        return;
      }

   Assert(sizeof(character_table) / sizeof(*character_table) == MAX_AV);

   // check that character_table.unicode is sorted by increasing UNICODE
   //
   for (int i = 1; i < MAX_AV; ++i)
       show_error_pos(i, __LINE__, character_table[i].def_line,
          character_table[i].unicode > character_table[i - 1].unicode);

   // check that ASCII chars map to themselves.
   //
   loop(i, 0x80)   show_error_pos(i, __LINE__, character_table[i].def_line,
                                  character_table[i].unicode == i);

   // display holes and duplicate AV positions in character_table
   {
     int holes = 0;
     loop(pos, MAX_AV)
        {
          int count = 0;
          loop(i, MAX_AV)
              {
                if (character_table[i].av_pos == pos)   ++count;
              }

          if (count == 0)
             {
               ++holes;
               CERR << "AV position " << HEX(pos) << " unused" << endl;
             }

          if (count > 1)
             CERR << "duplicate AV position " << HEX(pos) << endl;
        }

     if (holes)   CERR << holes << " unused positions in ⎕AV" << endl;
   }

   // check that find_char() works
   //
   loop(i, MAX_AV)   show_error_pos(i, __LINE__, character_table[i].def_line,
                                    i == find_char(character_table[i].unicode));

   Log(LOG_SHOW_AV)
      {
        for (int i = 0x80; i < MAX_AV; ++i)
            {
              CERR << character_table[i].unicode << " is AV[" << i << "] AV_"
                   << character_table[i].char_name << endl;
            }
      }

// check_file("../keyboard");
// check_file("../keyboard1");
}
//-----------------------------------------------------------------------------
Unicode
Avec::unicode(CHT_Index av)
{
   if (av < 0)         return UNI_AV_MAX;
   if (av >= MAX_AV)   return UNI_AV_MAX;
   return character_table[av].unicode;
}
//-----------------------------------------------------------------------------
uint32_t
Avec::get_av_pos(CHT_Index av)
{
   if (av < 0)         return UNI_AV_MAX;
   if (av >= MAX_AV)   return UNI_AV_MAX;
   return character_table[av].av_pos;
}
//-----------------------------------------------------------------------------
Token
Avec::uni_to_token(Unicode uni, const char * loc)
{
CHT_Index idx = find_char(uni);
   if (idx != Invalid_CHT)   return Token(character_table[idx].token_tag, uni);

   // not found: try alternative characters.
   //
   idx = map_alternative_char(uni);
   if (idx != Invalid_CHT)   return Token(character_table[idx].token_tag,
                                          character_table[idx].unicode);

   Log(LOG_verbose_error)
      {
        CERR << endl << "Avec::uni_to_token() : Char " << UNI(uni)
             << " (" << uni << ") not found in ⎕AV! (called from "
             << loc << ")" << endl;

         Backtrace::show(__FILE__, __LINE__);
      }
   return Token();
}
//-----------------------------------------------------------------------------
uint32_t Avec::find_av_pos(Unicode av)
{
const CHT_Index pos = find_char(av);

   if (pos < 0)   return MAX_AV;   // not found

   return character_table[pos].av_pos;
}
//-----------------------------------------------------------------------------
CHT_Index
Avec::find_char(Unicode av)
{
int l = 0;
int h = sizeof(character_table)/sizeof(*character_table) - 1;

   for (;;)
       {
         if (l > h)   return Invalid_CHT;   // not in table.

         const int m((h + l)/2);
         const Unicode um = character_table[m].unicode;
         if      (av < um)   h = m - 1;
         else if (av > um)   l = m + 1;
         else                return CHT_Index(m);
       }
}
//-----------------------------------------------------------------------------
CHT_Index
Avec::map_alternative_char(Unicode alt_av)
{
   // map characters that look similar to characters used in GNU APL
   // to the GNU APL character.
   //
   switch((int)alt_av)
      {
        case 0x005E: return AV_AND;              //  map ^ to ∧
        case 0x007C: return AV_DIVIDES;          //  map | to ∣
        case 0x007E: return AV_TILDE_OPERATOR;   //  map ~ to ∼
        case 0x03B1: return AV_ALPHA;            //  map α to ⍺
        case 0x03B5: return AV_ELEMENT;          //  map ε to ∈
        case 0x03B9: return AV_IOTA;             //  map ι to ⍳
        case 0x03C1: return AV_RHO;              //  map ρ to ⍴
        case 0x03C9: return AV_OMEGA;            //  map ω to ⍵
        case 0x2018: return AV_SINGLE_QUOTE;     //  map ‘ to '
        case 0x2019: return AV_SINGLE_QUOTE;     //  map ’ to '
        case 0x220A: return AV_ELEMENT;          //  map ∊ to ∈
        case 0x2212: return AV_ASCII_MINUS;      //  map − to -
        case 0x22BC: return AV_NAND;             //  map ⊽ to ⍲
        case 0x22BD: return AV_NOR;              //  map ⊼ to ⍱
        case 0x22C4: return AV_DIAMOND;          //  map ⋄ to ◊
        case 0x2377: return AV_EPSILON_UBAR;     //  map ⍷ to ⋸
        case 0x25AF: return AV_Quad_Quad;        //  map ▯ to ⎕
        case 0x25E6: return AV_RING_OPERATOR;    //  map ◦ to ∘
        case 0x26AA: return AV_CIRCLE;           //  map ⚪ to ○
        case 0x2A7D: return AV_LESS_OR_EQUAL;    //  map ⩽ to ≤
        case 0x2A7E: return AV_MORE_OR_EQUAL;    //  map ⩾ to ≥
        case 0x2B25: return AV_DIAMOND;          //  map ⬥ to ◊
        case 0x2B26: return AV_DIAMOND;          //  map ⬦ to ◊
        case 0x2B27: return AV_DIAMOND;          //  map ⬧ to ◊
        default:     break;
      }

   return Invalid_CHT;
}
//-----------------------------------------------------------------------------
bool
Avec::is_known_char(Unicode av)
{
   switch(av)
      {
#define char_def(_n, u, _t, _f, _p)  case u: 
#define char_df1(_n, u, _t, _f, _p)  case u: 
#include "Avec.def"
          return true;

        default: break;
      }

   return false;   // not found
}
//-----------------------------------------------------------------------------
bool
Avec::need_UCS(Unicode uni)
{
   if (uni < UNI_ASCII_SPACE)    return true;   // ASCII control char
   if (uni < UNI_ASCII_DELETE)   return false;   // printable ASCII char

const CHT_Index idx = find_char(uni);
   if (idx == Invalid_CHT)   return true;           // char not in GNU APL's ⎕AV
   if (unicode_to_cp(uni) == 0xB0)   return true;   // not in IBM's ⎕AV

   return false;
}
//-----------------------------------------------------------------------------
bool
Avec::is_symbol_char(Unicode av)
{
const int32_t idx = find_char(av);
   if (idx == -1)   return false;
   return (character_table[idx].flags & FLG_SYMBOL) != 0;
}
//-----------------------------------------------------------------------------
bool
Avec::is_first_symbol_char(Unicode av)
{
   return is_symbol_char(av) && ! is_digit(av);
}
//-----------------------------------------------------------------------------
bool
Avec::is_digit(Unicode av)
{
   return (av <= UNI_ASCII_9 && av >= UNI_ASCII_0);
}
//-----------------------------------------------------------------------------
bool
Avec::is_digit_or_space(Unicode av)
{
   return (av <= UNI_ASCII_SPACE || (av <= UNI_ASCII_9 && av >= UNI_ASCII_0));
}
//-----------------------------------------------------------------------------
bool
Avec::is_number(Unicode av)
{
const int32_t idx = find_char(av);
   if (idx == -1)   return false;
   return (character_table[idx].flags & FLG_NUMERIC) != 0;
}
//-----------------------------------------------------------------------------
bool
Avec::no_space_after(Unicode av)
{
const int32_t idx = find_char(av);
   if (idx == -1)   return false;
   return (character_table[idx].flags & FLG_NO_SPACE_AFTER) != 0;
}
//-----------------------------------------------------------------------------
bool
Avec::no_space_before(Unicode av)
{
const int32_t idx = find_char(av);
   if (idx == -1)   return false;
   return (character_table[idx].flags & FLG_NO_SPACE_BEFORE) != 0;
}
//-----------------------------------------------------------------------------
Unicode
Avec::subscript(uint32_t i)
{
   switch(i)
      {
        case 0: return Unicode(0x2080);
        case 1: return Unicode(0x2081);
        case 2: return Unicode(0x2082);
        case 3: return Unicode(0x2083);
        case 4: return Unicode(0x2084);
        case 5: return Unicode(0x2085);
        case 6: return Unicode(0x2086);
        case 7: return Unicode(0x2087);
        case 8: return Unicode(0x2088);
        case 9: return Unicode(0x2089);
      }

   return Unicode(0x2093);
}
//-----------------------------------------------------------------------------
Unicode
Avec::superscript(uint32_t i)
{
   switch(i)
      {
        case 0: return Unicode(0x2070);
        case 1: return Unicode(0x00B9);
        case 2: return Unicode(0x00B2);
        case 3: return Unicode(0x00B3);
        case 4: return Unicode(0x2074);
        case 5: return Unicode(0x2075);
        case 6: return Unicode(0x2076);
        case 7: return Unicode(0x2077);
        case 8: return Unicode(0x2078);
        case 9: return Unicode(0x2079);
      }

   return Unicode(0x207A);
}
//-----------------------------------------------------------------------------
/* the IBM APL2 character set shown in lrm figure 68 on page 470

   The table is indexed with an 8-bit position in IBM's ⎕AV and returns
   the Unicode for that position
 */
static const uint32_t ibm_av[] =
{
  0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 
  0x0008, 0x0009, 0x000A, 0x000B, 0x000C, 0x000D, 0x000E, 0x000F, 
  0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017, 
  0x0018, 0x0019, 0x001A, 0x001B, 0x001C, 0x001D, 0x001E, 0x001F, 
  0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027, 
  0x0028, 0x0029, 0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F, 
  0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 
  0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F, 
  0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 
  0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F, 
  0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 
  0x0058, 0x0059, 0x005A, 0x005B, 0x005C, 0x005D, 0x005E, 0x005F, 
  0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 
  0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F, 
  0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077, 
  0x0078, 0x0079, 0x007A, 0x007B, 0x007C, 0x007D, 0x007E, 0x007F, 
  0x00C7, 0x00FC, 0x00E9, 0x00E2, 0x00E4, 0x00E0, 0x00E5, 0x00E7,
  0x00EA, 0x00EB, 0x00E8, 0x00EF, 0x00EE, 0x00EC, 0x00C4, 0x00C5,
  0x2395, 0x235E, 0x2339, 0x00F4, 0x00F6, 0x00F2, 0x00FB, 0x00F9,
  0x22A4, 0x00D6, 0x00DC, 0x00F8, 0x00A3, 0x22A5, 0x2376, 0x2336,
  0x00E1, 0x00ED, 0x00F3, 0x00FA, 0x00F1, 0x00D1, 0x00AA, 0x00BA,
  0x00BF, 0x2308, 0x00AC, 0x00BD, 0x222A, 0x00A1, 0x2355, 0x234E,
  0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x235F, 0x2206, 0x2207,
  0x2192, 0x2563, 0x2551, 0x2557, 0x255D, 0x2190, 0x230A, 0x2510,
  0x2514, 0x2534, 0x252C, 0x251C, 0x2500, 0x253C, 0x2191, 0x2193,
  0x255A, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256C, 0x2261,
  0x2378, 0x22F8, 0x2235, 0x2337, 0x2342, 0x233B, 0x22A2, 0x22A3,
  0x25CA, 0x2518, 0x250C, 0x2588, 0x2584, 0x00A6, 0x00CC, 0x2580,
  0x237A, 0x2379, 0x2282, 0x2283, 0x235D, 0x2372, 0x2374, 0x2371,
  0x233D, 0x2296, 0x25CB, 0x2228, 0x2373, 0x2349, 0x2208, 0x2229,
  0x233F, 0x2340, 0x2265, 0x2264, 0x2260, 0x00D7, 0x00F7, 0x2359,
  0x2218, 0x2375, 0x236B, 0x234B, 0x2352, 0x00AF, 0x00A8, 0x00A0
};

const Unicode *
Avec::IBM_quad_AV()
{
   return (const Unicode *)ibm_av;
}
//-----------------------------------------------------------------------------
Avec::Unicode_to_IBM_codepoint Avec::inverse_ibm_av[128] =
{
  { 0x00A0, 255 }, { 0x00A1, 173 }, { 0x00A3, 156 }, { 0x00A6, 221 },
  { 0x00A8, 254 }, { 0x00AA, 166 }, { 0x00AC, 170 }, { 0x00AF, 253 },
  { 0x00BA, 167 }, { 0x00BD, 171 }, { 0x00BF, 168 }, { 0x00C4, 142 },
  { 0x00C5, 143 }, { 0x00C7, 128 }, { 0x00CC, 222 }, { 0x00D1, 165 },
  { 0x00D6, 153 }, { 0x00D7, 245 }, { 0x00DC, 154 }, { 0x00E0, 133 },
  { 0x00E1, 160 }, { 0x00E2, 131 }, { 0x00E4, 132 }, { 0x00E5, 134 },
  { 0x00E7, 135 }, { 0x00E8, 138 }, { 0x00E9, 130 }, { 0x00EA, 136 },
  { 0x00EB, 137 }, { 0x00EC, 141 }, { 0x00ED, 161 }, { 0x00EE, 140 },
  { 0x00EF, 139 }, { 0x00F1, 164 }, { 0x00F2, 149 }, { 0x00F3, 162 },
  { 0x00F4, 147 }, { 0x00F6, 148 }, { 0x00F7, 246 }, { 0x00F8, 155 },
  { 0x00F9, 151 }, { 0x00FA, 163 }, { 0x00FB, 150 }, { 0x00FC, 129 },
  { 0x2190, 189 }, { 0x2191, 198 }, { 0x2192, 184 }, { 0x2193, 199 },
  { 0x2206, 182 }, { 0x2207, 183 }, { 0x2208, 238 }, { 0x2218, 248 },
  { 0x2228, 235 }, { 0x2229, 239 }, { 0x222A, 172 }, { 0x2235, 210 },
  { 0x2260, 244 }, { 0x2261, 207 }, { 0x2264, 243 }, { 0x2265, 242 },
  { 0x2282, 226 }, { 0x2283, 227 }, { 0x2296, 233 }, { 0x22A2, 214 },
  { 0x22A3, 215 }, { 0x22A4, 152 }, { 0x22A5, 157 }, { 0x22F8, 209 },
  { 0x2308, 169 }, { 0x230A, 190 }, { 0x2336, 159 }, { 0x2337, 211 },
  { 0x2339, 146 }, { 0x233B, 213 }, { 0x233D, 232 }, { 0x233F, 240 },
  { 0x2340, 241 }, { 0x2342, 212 }, { 0x2349, 237 }, { 0x234B, 251 },
  { 0x234E, 175 }, { 0x2352, 252 }, { 0x2355, 174 }, { 0x2359, 247 },
  { 0x235D, 228 }, { 0x235E, 145 }, { 0x235F, 181 }, { 0x236B, 250 },
  { 0x2371, 231 }, { 0x2372, 229 }, { 0x2373, 236 }, { 0x2374, 230 },
  { 0x2375, 249 }, { 0x2376, 158 }, { 0x2378, 208 }, { 0x2379, 225 },
  { 0x237A, 224 }, { 0x2395, 144 }, { 0x2500, 196 }, { 0x2502, 179 },
  { 0x250C, 218 }, { 0x2510, 191 }, { 0x2514, 192 }, { 0x2518, 217 },
  { 0x251C, 195 }, { 0x2524, 180 }, { 0x252C, 194 }, { 0x2534, 193 },
  { 0x253C, 197 }, { 0x2550, 205 }, { 0x2551, 186 }, { 0x2554, 201 },
  { 0x2557, 187 }, { 0x255A, 200 }, { 0x255D, 188 }, { 0x2560, 204 },
  { 0x2563, 185 }, { 0x2566, 203 }, { 0x2569, 202 }, { 0x256C, 206 },
  { 0x2580, 223 }, { 0x2584, 220 }, { 0x2588, 219 }, { 0x2591, 176 },
  { 0x2592, 177 }, { 0x2593, 178 }, { 0x25CA, 216 }, { 0x25CB, 234 }
};

// the inverse mapping table of ibm_av above. We provide only the upper
// (non-ASCII) half to spead up bsearch() in the map.

#if 0
void
Avec::print_inverse_IBM_quad_AV()
{
   // a helper function that sorts ibm_av by Unicode and prints it on CERR
   //
   // To use it change #if 0 to #if ` bove, recompile, start apl
   // and enter command )OUT QQQ
   ..
   loop(c, 128)
      {
        inverse_ibm_av[c].uni = -1;
        inverse_ibm_av[c].cp  = -1;
      }

int current_max = 0;
const uint32_t * upper = ibm_av + 128;
Unicode_to_IBM_codepoint * map = inverse_ibm_av;
   loop(c, 128)
      {
        // find next Unicode after current_max
        //
        int next_idx = -1;
        loop(n, 128)
           {
             const int uni = upper[n];
             if (uni <= current_max)   continue;   // already done
             if (next_idx == -1)               next_idx = n;
             else if (uni < upper[next_idx])   next_idx = n;
           }
        current_max = map->uni = upper[next_idx];
        map->cp  = next_idx;
        ++map;
      }

   loop(row, 32)
      {
        CERR << " ";
        loop(col, 4)
           {
             const int pos = col + 4*row;
             CERR << " { " << HEX4(inverse_ibm_av[pos].uni) << ", "
                  << 128 + inverse_ibm_av[pos].cp << " }";
             if (pos < 127)   CERR << ",";
           }
        CERR << endl;
      }

   exit(1);
}
#endif
//-----------------------------------------------------------------------------
/// compare the unicodes of two chars in the IBM ⎕AV
int
Avec::compare_uni(const void * key, const void * entry)
{
   return *(const Unicode *)key -
          ((const Unicode_to_IBM_codepoint *)entry)->uni;
}
//-----------------------------------------------------------------------------
unsigned char
Avec::unicode_to_cp(Unicode uni)
{
   if (uni <= 0x80)                return uni;
   if (uni == UNI_STAR_OPERATOR)   return '*';   // ⋆ → *
   if (uni == UNI_AND)             return '^';   // ∧ → ^
   if (uni == UNI_TILDE_OPERATOR)  return 126;   // ∼ → ~

   // search in uni_to_cp_map table
   //
const void * where = bsearch(&uni, inverse_ibm_av, 128,
                             sizeof(Unicode_to_IBM_codepoint), compare_uni);

   if (where == 0)
      {
        // the workspace being )OUT'ed can contain characters that are not
        // in IBM's APL character set. We replace such characters by 0xB0
        //
        return 0xB0;
      }

   Assert(where);
   return ((const Unicode_to_IBM_codepoint *)where)->cp;
}
//-----------------------------------------------------------------------------

