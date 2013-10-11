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

#ifndef __TOKENENUMS_HH_DEFINED__
#define __TOKENENUMS_HH_DEFINED__

#include "Id.hh"   // for ID_xxx

/**
    The class tag of a token. Token classes < TC_MAX_PERM are permanent
    (e.g. used in user defined functions) while higher classes are
    temporary (used while parsing),
 **/
enum TokenClass
{
   // token classes.
   //

   // permanent token classes. Token of these classes can appear in
   // the body of a defined function.
   //
   TC_ASSIGN        = 0x01,   // ←
   TC_R_ARROW       = 0x02,   // →N
   TC_L_BRACK       = 0x03,   // [
   TC_R_BRACK       = 0x04,   // ]
   TC_END           = 0x05,   // left end of statement
   TC_FUN0          = 0x06,
   TC_FUN12         = 0x07,
   TC_INDEX         = 0x08,
   TC_OPER1         = 0x09,
   TC_OPER2         = 0x0A,
   TC_L_PARENT      = 0x0B,   // (
   TC_R_PARENT      = 0x0C,   // )
   TC_RETURN        = 0x0D,
   TC_SYMBOL        = 0x0E,
   TC_VALUE         = 0x0F,

   TC_MAX_PERM,               ///< permanent token are < TC_MAX_PERM

   TC_FUN1          = TC_FUN12,
   TC_FUN2          = TC_FUN12,

   // temporary token classes. Token of these classes can appear as
   // intermediate results during tokenization and execution
   //
   TC_PINDEX        = 0x10,
   TC_VOID          = 0x11,
   TC_MAX_PHRASE,             ///< token in phrases are < TC_MAX_PHRASE
   TC_MAX_PHRASE_2 = TC_MAX_PHRASE*TC_MAX_PHRASE,     // TC_MAX_PHRASE ^ 2
   TC_MAX_PHRASE_3 = TC_MAX_PHRASE*TC_MAX_PHRASE_2,   // TC_MAX_PHRASE ^ 3
   TC_MAX_PHRASE_4 = TC_MAX_PHRASE*TC_MAX_PHRASE_3,   // TC_MAX_PHRASE ^ 4

   TC_OFF           = 0x12,
   TC_SI_LEAVE      = 0x13,
   TC_LINE          = 0x14,
   TC_DIAMOND       = 0x15,   // ◊
   TC_NUMERIC       = 0x16,   // 0-9, ¯
   TC_SPACE         = 0x17,   // space, tab, CR (but not LF)
   TC_NEWLINE       = 0x18,   // LF
   TC_COLON         = 0x19,   // :
   TC_QUOTE         = 0x1A,   // ' or "

   TC_MASK          = 0xFF,
   TC_INVALID       = 0xFF,

   // short token class names for phrase table
   //
   SN_A             = TC_VALUE,
   SN_ASS           = TC_ASSIGN,
   SN_B             = TC_VALUE,
   SN_C             = TC_INDEX,
   SN_D             = TC_OPER2,
   SN_END           = TC_END,
   SN_F             = TC_FUN12,
   SN_G             = TC_FUN12,
   SN_GOTO          = TC_R_ARROW,
   SN_I             = TC_PINDEX,
   SN_LBRA          = TC_L_BRACK,
   SN_LPAR          = TC_L_PARENT,
   SN_M             = TC_OPER1,
   SN_N             = TC_FUN0,
   SN_RETC          = TC_RETURN,
   SN_RBRA          = TC_R_BRACK,
   SN_RPAR          = TC_R_PARENT,
   SN_V             = TC_SYMBOL,
   SN_VOID          = TC_VOID,
   SN_              = TC_INVALID
};

   /// binding strengths between token classes
enum Binding_Strength
{
   BS_ANY     =  0,
   BS_ASS_B   = 10,   ///< ← ANY   : ← to what is on its right
   BS_F_B     = 20,   ///< F ANY   : function to its right argument
   BS_A_F     = 30,   ///< ANY F   : function to its left argument
   BS_LO_OP   = 40,   ///< ANY OP  : operator to its left function
   BS_VAL_VAL = 50,   ///< VEC VEC : vector to vector
   BS_OP_RO   = 60,   ///< OP ANY  : (dyadic) operator to its right function
   BS_V_ASS   = 70,   ///< ANY ←   : ← to what is on its left
   BS_ANY_BRA = 80    ///< ANY []  : [] to what is on its left
};

/**
    The value type of a token
 **/
enum TokenValueType
{
   // token value types. The token value type defines the type of the
   // token in the union 'value'.
   //                              Type              union member
   TV_MASK          = 0xFF00,
   TV_NONE          = 0x0000,   // value not used
   TV_CHAR          = 0x0100,   // Unicode            .char_val
   TV_INT           = 0x0200,   // uint64_t           .int_val;
   TV_FLT           = 0x0300,   // APL_Float          .flt_val;
   TV_CPX           = 0x0400,   // cdouble            .complex_val;
   TV_SYM           = 0x0500,   // Symbol *           .sym_ptr;
   TV_LIN           = 0x0600,   // Function_Line      .fun_line;
   TV_VAL           = 0x0700,   // Value_P            .apl_val;
   TV_INDEX         = 0x0800,   // IndexExpr *        .index_val;
   TV_FUN           = 0x0900,   // Function *         .function;
};

/**
     A token tag. It is comprised of 3 fields: a 16 bit Id, an 8 bit
     TokenValueType, and an 8 bit Token class.

     Bit:  31............................16 15.............8 7-..............0
          --------------------------------------------------------------------
          |               Id               | TokenValueType |   TokenClass   |
          --------------------------------------------------------------------
 **/
enum TokenTag
{
#define TD(tok, tc, tv, id) tok = tc | tv | (id << 16),

   // token without value and without ID.
   //
TD(TOK_NEWLINE       , TC_NEWLINE   , TV_NONE , ID_No_ID        )

   // token with value and without ID.
   //
TD(TOK_LINE          , TC_LINE      , TV_LIN  , ID_No_ID        )
TD(TOK_SYMBOL        , TC_SYMBOL    , TV_SYM  , ID_No_ID        )
TD(TOK_LSYMB         , TC_SYMBOL    , TV_SYM  , ID_No_ID1       )
TD(TOK_LSYMB2        , TC_SYMBOL    , TV_SYM  , ID_No_ID2       )
TD(TOK_NUMERIC       , TC_NUMERIC   , TV_CHAR , ID_No_ID        )
TD(TOK_INDEX         , TC_INDEX     , TV_INDEX, ID_No_ID        )
TD(TOK_PINDEX        , TC_PINDEX    , TV_INDEX, ID_No_ID        )
TD(TOK_AXES          , TC_INDEX     , TV_VAL  , ID_No_ID        )
TD(TOK_FUN0          , TC_FUN0      , TV_FUN  , ID_No_ID        ) // user def'd
TD(TOK_FUN1          , TC_FUN1      , TV_FUN  , ID_No_ID        ) // user def'd
TD(TOK_FUN2          , TC_FUN2      , TV_FUN  , ID_No_ID        ) // user def'd
TD(TOK_OPER1         , TC_OPER1     , TV_FUN  , ID_No_ID        ) // user def'd
TD(TOK_OPER2         , TC_OPER2     , TV_FUN  , ID_No_ID        ) // user def'd

   // token without value and with ID.
   //
TD(TOK_FIRST_TIME    , TC_VOID      , TV_NONE , ID_FIRST_TIME   )
TD(TOK_INVALID       , TC_INVALID   , TV_NONE , ID_INVALID      )
TD(TOK_VOID          , TC_VOID      , TV_NONE , ID_VOID         )
TD(TOK_END           , TC_END       , TV_NONE , ID_END          )
TD(TOK_ENDL          , TC_END       , TV_NONE , ID_ENDL         )   // last END
TD(TOK_OFF           , TC_OFF       , TV_NONE , ID_OFF          )
TD(TOK_ESCAPE        , TC_R_ARROW   , TV_NONE , ID_ESCAPE       )
TD(TOK_SPACE         , TC_SPACE     , TV_NONE , ID_SPACE        )
TD(TOK_COLON         , TC_COLON     , TV_NONE , ID_COLON        )
TD(TOK_QUOTE1        , TC_QUOTE     , TV_NONE , ID_QUOTE1       )
TD(TOK_QUOTE2        , TC_QUOTE     , TV_NONE , ID_QUOTE2       )
TD(TOK_ASSIGN        , TC_ASSIGN    , TV_NONE , ID_ASSIGN       )
TD(TOK_R_ARROW       , TC_R_ARROW   , TV_NONE , ID_R_ARROW      )
TD(TOK_DIAMOND       , TC_DIAMOND   , TV_NONE , ID_DIAMOND      )
TD(TOK_RETURN_EXEC   , TC_RETURN    , TV_NONE , ID_RETURN_EXEC  )
TD(TOK_RETURN_STATS  , TC_RETURN    , TV_NONE , ID_RETURN_STATS )
TD(TOK_RETURN_VOID   , TC_RETURN    , TV_NONE , ID_RETURN_VOID  )
TD(TOK_NO_VALUE      , TC_VALUE     , TV_NONE , ID_NO_VALUE     )
TD(TOK_SI_PUSHED     , TC_SI_LEAVE  , TV_NONE , ID_SI_PUSHED    )
TD(TOK_L_BRACK       , TC_L_BRACK   , TV_NONE , ID_L_BRACK      )
TD(TOK_SEMICOL       , TC_L_BRACK   , TV_NONE , ID_SEMICOL      )
TD(TOK_R_BRACK       , TC_R_BRACK   , TV_NONE , ID_R_BRACK      )
TD(TOK_L_PARENT      , TC_L_PARENT  , TV_NONE , ID_L_PARENT     )
TD(TOK_R_PARENT      , TC_R_PARENT  , TV_NONE , ID_R_PARENT     )

   // token with value and with ID.
   //
TD(TOK_RETURN_SYMBOL , TC_RETURN    , TV_SYM  , ID_RETURN_SYMBOL)
TD(TOK_QUAD_QUAD     , TC_SYMBOL    , TV_SYM  , ID_QUAD_QUAD    )
TD(TOK_QUAD_QUOTE    , TC_SYMBOL    , TV_SYM  , ID_QUOTE_QUAD   )
TD(TOK_QUAD_AI       , TC_SYMBOL    , TV_SYM  , ID_QUAD_AI      )
TD(TOK_QUAD_ARG      , TC_SYMBOL    , TV_SYM  , ID_QUAD_ARG     )
TD(TOK_QUAD_AV       , TC_SYMBOL    , TV_SYM  , ID_QUAD_AV      )
TD(TOK_QUAD_CT       , TC_SYMBOL    , TV_SYM  , ID_QUAD_CT      )
TD(TOK_QUAD_EM       , TC_SYMBOL    , TV_SYM  , ID_QUAD_EM      )
TD(TOK_QUAD_ET       , TC_SYMBOL    , TV_SYM  , ID_QUAD_ET      )
TD(TOK_QUAD_FC       , TC_SYMBOL    , TV_SYM  , ID_QUAD_FC      )
TD(TOK_QUAD_IO       , TC_SYMBOL    , TV_SYM  , ID_QUAD_IO      )
TD(TOK_QUAD_LC       , TC_SYMBOL    , TV_SYM  , ID_QUAD_LC      )
TD(TOK_QUAD_LX       , TC_SYMBOL    , TV_SYM  , ID_QUAD_LX      )
TD(TOK_QUAD_L        , TC_SYMBOL    , TV_SYM  , ID_QUAD_L       )
TD(TOK_QUAD_NLT      , TC_SYMBOL    , TV_SYM  , ID_QUAD_NLT     )
TD(TOK_QUAD_PP       , TC_SYMBOL    , TV_SYM  , ID_QUAD_PP      )
TD(TOK_QUAD_PR       , TC_SYMBOL    , TV_SYM  , ID_QUAD_PR      )
TD(TOK_QUAD_PS       , TC_SYMBOL    , TV_SYM  , ID_QUAD_PS      )
TD(TOK_QUAD_PT       , TC_SYMBOL    , TV_SYM  , ID_QUAD_PT      )
TD(TOK_QUAD_PW       , TC_SYMBOL    , TV_SYM  , ID_QUAD_PW      )
TD(TOK_QUAD_R        , TC_SYMBOL    , TV_SYM  , ID_QUAD_R       )
TD(TOK_QUAD_RL       , TC_SYMBOL    , TV_SYM  , ID_QUAD_RL      )
TD(TOK_QUAD_SVE      , TC_SYMBOL    , TV_SYM  , ID_QUAD_SVE     )
TD(TOK_QUAD_TC       , TC_SYMBOL    , TV_SYM  , ID_QUAD_TC      )
TD(TOK_QUAD_TS       , TC_SYMBOL    , TV_SYM  , ID_QUAD_TS      )
TD(TOK_QUAD_TZ       , TC_SYMBOL    , TV_SYM  , ID_QUAD_TZ      )
TD(TOK_QUAD_UL       , TC_SYMBOL    , TV_SYM  , ID_QUAD_UL      )
TD(TOK_QUAD_WA       , TC_SYMBOL    , TV_SYM  , ID_QUAD_WA      )

TD(TOK_F0_ZILDE      , TC_FUN0      , TV_FUN  , ID_F0_ZILDE     )
TD(TOK_F1_EXECUTE    , TC_FUN1      , TV_FUN  , ID_F1_EXECUTE   )
TD(TOK_QUAD_AF       , TC_FUN1      , TV_FUN  , ID_QUAD_AF      )
TD(TOK_QUAD_CR       , TC_FUN2      , TV_FUN  , ID_QUAD_CR      )
TD(TOK_QUAD_DL       , TC_FUN1      , TV_FUN  , ID_QUAD_DL      )
TD(TOK_QUAD_EC       , TC_FUN1      , TV_FUN  , ID_QUAD_EC      )
TD(TOK_QUAD_ENV      , TC_FUN1      , TV_FUN  , ID_QUAD_ENV     )
TD(TOK_QUAD_EX       , TC_FUN1      , TV_FUN  , ID_QUAD_EX      )
TD(TOK_QUAD_SVQ      , TC_FUN1      , TV_FUN  , ID_QUAD_SVQ     )
TD(TOK_QUAD_SVR      , TC_FUN1      , TV_FUN  , ID_QUAD_SVR     )
TD(TOK_QUAD_SVS      , TC_FUN1      , TV_FUN  , ID_QUAD_SVS     )
TD(TOK_QUAD_SI       , TC_FUN1      , TV_FUN  , ID_QUAD_SI      )
TD(TOK_QUAD_UCS      , TC_FUN1      , TV_FUN  , ID_QUAD_UCS     )

TD(TOK_F2_AND        , TC_FUN2      , TV_FUN  , ID_F2_AND       )
TD(TOK_QUAD_AT       , TC_FUN2      , TV_FUN  , ID_QUAD_AT      )
TD(TOK_F2_EQUAL      , TC_FUN2      , TV_FUN  , ID_F2_EQUAL     )
TD(TOK_F2_FIND       , TC_FUN2      , TV_FUN  , ID_F2_FIND      )
TD(TOK_F2_GREATER    , TC_FUN2      , TV_FUN  , ID_F2_GREATER   )
TD(TOK_F2_INDEX      , TC_FUN2      , TV_FUN  , ID_F2_INDEX     )
TD(TOK_QUAD_INP      , TC_FUN2      , TV_FUN  , ID_QUAD_INP     )
TD(TOK_F2_LEQ        , TC_FUN2      , TV_FUN  , ID_F2_LEQ       )
TD(TOK_F2_LESS       , TC_FUN2      , TV_FUN  , ID_F2_LESS      )
TD(TOK_F2_MEQ        , TC_FUN2      , TV_FUN  , ID_F2_MEQ       )
TD(TOK_F2_NAND       , TC_FUN2      , TV_FUN  , ID_F2_NAND      )
TD(TOK_F2_NOR        , TC_FUN2      , TV_FUN  , ID_F2_NOR       )
TD(TOK_F2_OR         , TC_FUN2      , TV_FUN  , ID_F2_OR        )
TD(TOK_F2_UNEQ       , TC_FUN2      , TV_FUN  , ID_F2_UNEQ      )
TD(TOK_QUAD_EA       , TC_FUN2      , TV_FUN  , ID_QUAD_EA      )
TD(TOK_QUAD_SVC      , TC_FUN2      , TV_FUN  , ID_QUAD_SVC     )
TD(TOK_QUAD_TF       , TC_FUN2      , TV_FUN  , ID_QUAD_TF      )

TD(TOK_F12_BINOM     , TC_FUN2      , TV_FUN  , ID_F12_BINOM    )
TD(TOK_F12_CIRCLE    , TC_FUN2      , TV_FUN  , ID_F12_CIRCLE   )
TD(TOK_F12_COMMA     , TC_FUN2      , TV_FUN  , ID_F12_COMMA    )
TD(TOK_F12_COMMA1    , TC_FUN2      , TV_FUN  , ID_F12_COMMA1   )
TD(TOK_F12_DECODE    , TC_FUN2      , TV_FUN  , ID_F12_DECODE   )
TD(TOK_F12_DIVIDE    , TC_FUN2      , TV_FUN  , ID_F12_DIVIDE   )
TD(TOK_F12_DOMINO    , TC_FUN2      , TV_FUN  , ID_F12_DOMINO   )
TD(TOK_F12_DROP      , TC_FUN2      , TV_FUN  , ID_F12_DROP     )
TD(TOK_F12_ELEMENT   , TC_FUN2      , TV_FUN  , ID_F12_ELEMENT  )
TD(TOK_F12_ENCODE    , TC_FUN2      , TV_FUN  , ID_F12_ENCODE   )
TD(TOK_F12_EQUIV     , TC_FUN2      , TV_FUN  , ID_F12_EQUIV    )
TD(TOK_F12_FORMAT    , TC_FUN2      , TV_FUN  , ID_F12_FORMAT   )
TD(TOK_F12_INDEX_OF  , TC_FUN2      , TV_FUN  , ID_F12_INDEX_OF )
TD(TOK_JOT           , TC_FUN2      , TV_FUN  , ID_JOT          )
TD(TOK_F2_LEFT       , TC_FUN2      , TV_FUN  , ID_F2_LEFT      )
TD(TOK_F12_LOGA      , TC_FUN2      , TV_FUN  , ID_F12_LOGA     )
TD(TOK_F12_MINUS     , TC_FUN2      , TV_FUN  , ID_F12_MINUS    )
TD(TOK_F12_PARTITION , TC_FUN2      , TV_FUN  , ID_F12_PARTITION)
TD(TOK_F12_PICK      , TC_FUN2      , TV_FUN  , ID_F12_PICK     )
TD(TOK_F12_PLUS      , TC_FUN2      , TV_FUN  , ID_F12_PLUS     )
TD(TOK_F12_POWER     , TC_FUN2      , TV_FUN  , ID_F12_POWER    )
TD(TOK_F12_RHO       , TC_FUN2      , TV_FUN  , ID_F12_RHO      )
TD(TOK_F2_RIGHT      , TC_FUN2      , TV_FUN  , ID_F2_RIGHT     )
TD(TOK_F12_RND_DN    , TC_FUN2      , TV_FUN  , ID_F12_RND_DN   )
TD(TOK_F12_RND_UP    , TC_FUN2      , TV_FUN  , ID_F12_RND_UP   )
TD(TOK_F12_ROLL      , TC_FUN2      , TV_FUN  , ID_F12_ROLL     )
TD(TOK_F12_ROTATE    , TC_FUN2      , TV_FUN  , ID_F12_ROTATE   )
TD(TOK_F12_ROTATE1   , TC_FUN2      , TV_FUN  , ID_F12_ROTATE1  )
TD(TOK_F12_SORT_ASC  , TC_FUN2      , TV_FUN  , ID_F12_SORT_ASC )
TD(TOK_F12_SORT_DES  , TC_FUN2      , TV_FUN  , ID_F12_SORT_DES )
TD(TOK_F12_STILE     , TC_FUN2      , TV_FUN  , ID_F12_STILE    )
TD(TOK_F12_TAKE      , TC_FUN2      , TV_FUN  , ID_F12_TAKE     )
TD(TOK_F12_TIMES     , TC_FUN2      , TV_FUN  , ID_F12_TIMES    )
TD(TOK_F12_TRANSPOSE , TC_FUN2      , TV_FUN  , ID_F12_TRANSPOSE)
TD(TOK_F12_WITHOUT   , TC_FUN2      , TV_FUN  , ID_F12_WITHOUT  )
TD(TOK_F1_UNIQUE     , TC_FUN1      , TV_FUN  , ID_F1_UNIQUE    )
TD(TOK_QUAD_ES       , TC_FUN2      , TV_FUN  , ID_QUAD_ES      )
TD(TOK_QUAD_FX       , TC_FUN2      , TV_FUN  , ID_QUAD_FX      )
TD(TOK_QUAD_NA       , TC_FUN2      , TV_FUN  , ID_QUAD_NA      )
TD(TOK_QUAD_NC       , TC_FUN1      , TV_FUN  , ID_QUAD_NC      )
TD(TOK_QUAD_NL       , TC_FUN2      , TV_FUN  , ID_QUAD_NL      )
TD(TOK_QUAD_SVO      , TC_FUN2      , TV_FUN  , ID_QUAD_SVO     )

TD(TOK_OPER1_COMMUTE , TC_OPER1     , TV_FUN  , ID_OPER1_COMMUTE)
TD(TOK_OPER1_EACH    , TC_OPER1     , TV_FUN  , ID_OPER1_EACH   )
TD(TOK_OPER1_RANK    , TC_OPER1     , TV_FUN  , ID_OPER1_RANK   )
TD(TOK_OPER1_REDUCE  , TC_OPER1     , TV_FUN  , ID_OPER1_REDUCE )
TD(TOK_OPER1_REDUCE1 , TC_OPER1     , TV_FUN  , ID_OPER1_REDUCE1)
TD(TOK_OPER1_SCAN    , TC_OPER1     , TV_FUN  , ID_OPER1_SCAN   )
TD(TOK_OPER1_SCAN1   , TC_OPER1     , TV_FUN  , ID_OPER1_SCAN1  )
TD(TOK_OPER2_PRODUCT , TC_OPER2     , TV_FUN  , ID_OPER2_PRODUCT)

TD(TOK_VARIABLE      , TC_VALUE     , TV_SYM  , ID_VARIABLE     )
TD(TOK_APL_VALUE     , TC_VALUE     , TV_VAL  , ID_APL_VALUE    ) // ungrouped
TD(TOK_APL_VALUE1    , TC_VALUE     , TV_VAL  , ID_APL_VALUE1   ) // grouped
TD(TOK_APL_VALUE2    , TC_VALUE     , TV_VAL  , ID_APL_VALUE2   ) // assigned
TD(TOK_BRANCH        , TC_R_ARROW   , TV_INT  , ID_BRANCH       )
TD(TOK_CHARACTER     , TC_VALUE     , TV_CHAR , ID_CHARACTER    )
TD(TOK_INTEGER       , TC_VALUE     , TV_INT  , ID_INTEGER      )
TD(TOK_REAL          , TC_VALUE     , TV_FLT  , ID_REAL         )
TD(TOK_COMPLEX       , TC_VALUE     , TV_CPX  , ID_COMPLEX      )
TD(TOK_ERROR         , TC_SI_LEAVE  , TV_INT  , ID_ERROR        )
};

#endif // __TOKENENUMS_HH_DEFINED__
