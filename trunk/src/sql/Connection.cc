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

#include "Connection.hh"

const string Connection::replace_bind_args( const string &sql )
{
    stringstream out;
    int pos = 0;
    for( size_t i = 0 ; i < sql.size() ; i++ ) {
        char ch = sql[i];
        if( ch == '?' ) {
            out << make_positional_param( pos++ );
        }
        else {
            out << ch;
        }
    }
    return out.str();
}
