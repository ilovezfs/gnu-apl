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

#include "SqliteProvider.hh"
#include "SqliteConnection.hh"

static SqliteConnection *create_sqlite_connection( Value_P B )
{
    if( !B->is_char_string() ) {
        Workspace::more_error() = "SQLite database connect argument must be a single string";
        DOMAIN_ERROR;
    }

    string filename = to_string( B->get_UCS_ravel() );
    sqlite3 *db;
    if( sqlite3_open( filename.c_str(), &db ) != SQLITE_OK ) {
        stringstream out;
        out << "Error opening database: " << sqlite3_errmsg( db );
        Workspace::more_error() = out.str().c_str();
        DOMAIN_ERROR;
    }

    return new SqliteConnection( db );
}

Connection *SqliteProvider::open_database( Value_P B )
{
    Connection *connection = create_sqlite_connection( B );
    return connection;
}
