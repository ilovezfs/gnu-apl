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
#include "PostgresConnection.hh"
#include "PostgresArgListBuilder.hh"

PostgresConnection::~PostgresConnection()
{
    PQfinish( db );
}

ArgListBuilder *PostgresConnection::make_prepared_query( const string &sql )
{
    return new PostgresArgListBuilder( this, sql );
}

ArgListBuilder *PostgresConnection::make_prepared_update( const string &sql )
{
    return new PostgresArgListBuilder( this, sql );    
}

void PostgresConnection::transaction_begin( void )
{
    PostgresResultWrapper result( PQexec( db, "begin" ) );
    if( PQresultStatus( result.get_result() ) != PGRES_COMMAND_OK ) {
        stringstream out;
        out << "Error when calling begin: " << PQresultErrorMessage( result.get_result() );
        Workspace::more_error() = out.str().c_str();
        DOMAIN_ERROR;
    }
}

void PostgresConnection::transaction_commit( void )
{
    PostgresResultWrapper result( PQexec( db, "commit" ) );
    if( PQresultStatus( result.get_result() ) != PGRES_COMMAND_OK ) {
        stringstream out;
        out << "Error when calling commit: " << PQresultErrorMessage( result.get_result() );
        Workspace::more_error() = out.str().c_str();
        DOMAIN_ERROR;
    }
}

void PostgresConnection::transaction_rollback( void )
{
    PostgresResultWrapper result( PQexec( db, "rollback" ) );
    if( PQresultStatus( result.get_result() ) != PGRES_COMMAND_OK ) {
        stringstream out;
        out << "Error when calling rollback: " << PQresultErrorMessage( result.get_result() );
        Workspace::more_error() = out.str().c_str();
        DOMAIN_ERROR;
    }
}

void PostgresConnection::fill_tables( vector<string> &tables )
{
    PostgresResultWrapper result( PQexec( db, "select tablename from pg_tables where schemaname = 'public'" ) );
    ExecStatusType status = PQresultStatus( result.get_result() );
    if( status != PGRES_TUPLES_OK ) {
        stringstream out;
        out << "Error getting list of tables: " << PQresultErrorMessage( result.get_result() );
        Workspace::more_error() = out.str().c_str();
        DOMAIN_ERROR;            
    }

    int rows = PQntuples( result.get_result() );
    for( int row = 0 ; row < rows ; row++ ) {
        tables.push_back( PQgetvalue( result.get_result(), row, 0 ) );
    }
}

const string PostgresConnection::make_positional_param( int pos )
{
    stringstream out;
    out << "$" << (pos + 1);
    return out.str();
}
