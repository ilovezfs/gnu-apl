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

#include "SqliteArgListBuilder.hh"

#include <string.h>
#include "ResultValue.hh"

void SqliteArgListBuilder::init_sql( void )
{
    const char *sql_charptr = sql.c_str();
    if( sqlite3_prepare_v2( connection->get_db(),
                            sql_charptr, strlen( sql_charptr ) + 1,
                            &statement, NULL ) != SQLITE_OK ) {
        connection->raise_sqlite_error( "Error preparing query" );
    }
}

SqliteArgListBuilder::SqliteArgListBuilder( SqliteConnection *connection_in, const string &sql_in )
    : sql( sql_in ), connection( connection_in )
{
    init_sql();
}

SqliteArgListBuilder::~SqliteArgListBuilder()
{
    sqlite3_finalize( statement );
}

void SqliteArgListBuilder::clear_args( void )
{
    sqlite3_finalize( statement );
    init_sql();
}

static void free_text_arg( void *arg )
{
    free( arg );
}

void SqliteArgListBuilder::append_string( const string &arg, int pos )
{
    char *text = strdup( arg.c_str() );
    if( text == NULL ) {
        CERR << "Failed to allocate memory for bind arg" << endl;
        abort();
    }
    sqlite3_bind_text( statement, pos + 1, text, -1, free_text_arg );
}

void SqliteArgListBuilder::append_long( long arg, int pos )
{
    sqlite3_bind_int64( statement, pos + 1, arg );
}

void SqliteArgListBuilder::append_double( double arg, int pos )
{
    sqlite3_bind_double( statement, pos + 1, arg );
}

void SqliteArgListBuilder::append_null( int pos )
{
    sqlite3_bind_null( statement, pos + 1 );
}

Value_P SqliteArgListBuilder::run_query( bool ignore_result )
{
    vector<ResultRow> results;
    int result;
    while( (result = sqlite3_step( statement )) != SQLITE_DONE ) {
        if( result != SQLITE_ROW ) {
            connection->raise_sqlite_error( "Error reading sql result" );
        }

        ResultRow row;
        row.add_values( statement );
        results.push_back( row );
    }

    Value_P db_result_value;
    int row_count = results.size();
    if( row_count > 0 ) {
        int col_count = results[0].get_values().size();
        Shape result_shape( row_count, col_count );
        db_result_value = new Value( result_shape, LOC );
        for( vector<ResultRow>::iterator row_iterator = results.begin() ; row_iterator != results.end() ; row_iterator++ ) {
            const vector<const ResultValue *> &row = row_iterator->get_values();
            for( vector<const ResultValue *>::const_iterator col_iterator = row.begin() ; col_iterator != row.end() ; col_iterator++ ) {
                (*col_iterator)->update( db_result_value->next_ravel() );
            }
        }
    }
    else {
        db_result_value = Idx0( LOC );
    }

    db_result_value->check_value( LOC );
    return db_result_value;
}
