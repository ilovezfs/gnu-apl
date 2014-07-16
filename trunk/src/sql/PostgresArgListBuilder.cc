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

#include "PostgresArgListBuilder.hh"

#include <string.h>

template<class T>
PostgresBindArg<T>::~PostgresBindArg()
{
    if( string_arg != NULL ) {
        free( string_arg );
    }
}


template<>
void PostgresBindArg<string>::update( Oid *types, const char **values, int *lengths, int *formats, int pos )
{
    types[pos] = 1043; // VARCHAROID
    stringstream out;
    out << arg;
    string_arg = strdup( out.str().c_str() );
    if( string_arg == NULL ) {
        abort();
    }
    values[pos] = string_arg;
    lengths[pos] = 0;
    formats[pos] = 0;    
}

template<>
void PostgresBindArg<long>::update( Oid *types, const char **values, int *lengths, int *formats, int pos )
{
    types[pos] = 20; // INT8OID
    stringstream out;
    out << arg;
    string_arg = strdup( out.str().c_str() );
    if( string_arg == NULL ) {
        abort();
    }
    values[pos] = string_arg;
    lengths[pos] = 0;
    formats[pos] = 0;
}

template<>
void PostgresBindArg<double>::update( Oid *types, const char **values, int *lengths, int *formats, int pos )
{
    types[pos] = 701; // FLOAT8OID
    stringstream out;
    out << setprecision(20) << arg;
    string_arg = strdup( out.str().c_str() );
    if( string_arg == NULL ) {
        abort();
    }
    values[pos] = string_arg;
    lengths[pos] = 0;
    formats[pos] = 0;
}

void PostgresNullArg::update( Oid *types, const char **values, int *lengths, int *formats, int pos )
{
    types[pos] = 0;
    values[pos] = NULL;
    lengths[pos] = 0;
    formats[pos] = 0;
}

PostgresArgListBuilder::PostgresArgListBuilder( PostgresConnection *connection_in, const string &sql_in )
    : connection( connection_in ), sql( sql_in )
{
}

PostgresArgListBuilder::~PostgresArgListBuilder()
{
    clear_args();
}

void PostgresArgListBuilder::clear_args( void )
{
    for( vector<PostgresArg *>::iterator i = args.begin() ; i != args.end() ; i++ ) {
        delete *i;
    }
    args.clear();
}

void PostgresArgListBuilder::append_string( const string &arg, int pos )
{
    Assert( static_cast<size_t>( pos ) == args.size() );
    args.push_back( new PostgresBindArg<string>( arg ) );
}

void PostgresArgListBuilder::append_long( long arg, int pos )
{
    Assert( static_cast<size_t>( pos ) == args.size() );
    args.push_back( new PostgresBindArg<long>( arg ) );
}

void PostgresArgListBuilder::append_double( double arg, int pos )
{
    Assert( static_cast<size_t>( pos ) == args.size() );
    args.push_back( new PostgresBindArg<double>( arg ) );
}

void PostgresArgListBuilder::append_null( int pos )
{
    Assert( static_cast<size_t>( pos ) == args.size() );
    args.push_back( new PostgresNullArg() );
}

static void update_int_cell( Cell *cell, char *content )
{
    if( *content == 0 ) {
        Workspace::more_error() = "Numeric content from database was empty";
        DOMAIN_ERROR;
    }

    char *endptr;
    long n = strtol( content, &endptr, 10 );
    if( *endptr != 0 ) {
        Workspace::more_error() = "Error parsing values returned from database";
        DOMAIN_ERROR;
    }

    new (cell) IntCell( n );
}

static void update_double_cell( Cell *cell, char *content )
{
    char *endptr;
    double n = strtod( content, &endptr );
    if( *endptr != 0 ) {
        Workspace::more_error() = "Error parsing decimal numbers returned from database";
        DOMAIN_ERROR;
    }

    new (cell) FloatCell( n );
}

Value_P PostgresArgListBuilder::run_query( bool ignore_result )
{
    int n = args.size();
    int array_len = n == 0 ? 1 : n;
    DynArray( Oid, types, array_len );
    DynArray( const char *, values, array_len );
    DynArray( int, lengths, array_len );
    DynArray( int, formats, array_len );

    for( int i = 0 ; i < n ; i++ ) {
        PostgresArg *arg = args[i];
        arg->update( types, values, lengths, formats, i );
    }

    PostgresResultWrapper result( PQexecParams( connection->get_db(), sql.c_str(), n,
                                                NULL, values, lengths, formats,
                                                0 ) );
    ExecStatusType status = PQresultStatus( result.get_result() );
    Value_P db_result_value;
    if( status == PGRES_COMMAND_OK ) {
        db_result_value = Str0( LOC );
    }
    else if( status == PGRES_TUPLES_OK ) {
        int rows = PQntuples( result.get_result() );
        if( rows == 0 ) {
            db_result_value = Idx0( LOC );
        }
        else {
            int cols = PQnfields( result.get_result() );
            Shape shape( rows, cols );
            db_result_value = new Value( shape, LOC );
            for( int row = 0 ; row < rows ; row++ ) {
                for( int col = 0 ; col < cols ; col++ ) {
                    if( PQgetisnull( result.get_result(), row, col ) ) {
                        new (db_result_value->next_ravel()) PointerCell( Idx0( LOC ) );
                    }
                    else {
                        Oid col_type = PQftype( result.get_result(), col );
                        char *value = PQgetvalue( result.get_result(), row, col );
                        if( col_type == 23 // INT4OID
                            || col_type == 20 // INT8OID
                            ) {
                            update_int_cell( db_result_value->next_ravel(), value );
                        }
                        else if( col_type == 1700 ) { // NUMERICOID
                            if( strchr( value, '.' ) == NULL ) {
                                update_int_cell( db_result_value->next_ravel(), value );
                            }
                            else {
                                update_double_cell( db_result_value->next_ravel(), value );
                            }
                        }
                        else {
                            if( *value == 0 ) {
                                new (db_result_value->next_ravel()) PointerCell( Str0( LOC ) );
                            }
                            else {
                                new (db_result_value->next_ravel()) PointerCell( make_string_cell( value, LOC ) );
                            }
                        }
                    }
                }
            }
        }
    }
    else {
        stringstream out;
        out << "Error executing query: " << PQresStatus( status ) << endl
            << "Message: " << PQresultErrorMessage( result.get_result() );
        Workspace::more_error() = out.str().c_str();
        DOMAIN_ERROR;
    }

    db_result_value->check_value( LOC );
    return db_result_value;
}
