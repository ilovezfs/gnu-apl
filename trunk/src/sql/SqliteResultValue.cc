/*
    This file is part of GNU APL, a free implementation of the
    ISO/IEC Standard 13751, "Programming Language APL, Extended"

    Copyright (C) 2014  Elias Mårtenson

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

#include "apl-sqlite.hh"
#include "SqliteResultValue.hh"

#include "Value.hh"
#include "IntCell.hh"
#include "FloatCell.hh"
#include "CharCell.hh"
#include "PointerCell.hh"

void IntResultValue::update( Cell *cell ) const
{
    new (cell) IntCell( value );
}

void DoubleResultValue::update( Cell *cell ) const
{
    new (cell) FloatCell( value );
}

void StringResultValue::update( Cell *cell ) const
{
    if( value.size() == 0 ) {
        new (cell) PointerCell( Str0( LOC ) );
    }
    else {
        new (cell) PointerCell( make_string_cell( value, LOC ) );
    }
}

void NullResultValue::update( Cell *cell ) const
{
    new (cell) PointerCell( Idx0( LOC ) );
}

void ResultRow::add_values( sqlite3_stmt *statement )
{
    int n = sqlite3_column_count( statement );
    for( int i = 0 ; i < n ; i++ ) {
        ResultValue *value;
        int type = sqlite3_column_type( statement, i );
        switch( type ) {
        case SQLITE_INTEGER:
            value = new IntResultValue( sqlite3_column_int( statement, i ) );
            break;
        case SQLITE_FLOAT:
            value = new DoubleResultValue( sqlite3_column_double( statement, i ) );
            break;
        case SQLITE_TEXT:
            value = new StringResultValue( reinterpret_cast<const char *>( sqlite3_column_text( statement, i ) ) );
            break;
        case SQLITE_BLOB:
            value = new NullResultValue();
            break;
        case SQLITE_NULL:
            value = new NullResultValue();
            break;
        default:
            CERR << "Unsupported column type, column=" << i << ", type+" << type << endl;
            value = new NullResultValue();
        }
        values.push_back( value );
    }
}
