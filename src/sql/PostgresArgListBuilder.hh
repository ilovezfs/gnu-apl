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

#ifndef POSTGRES_ARG_LIST_BUILDER_HH
#define POSTGRES_ARG_LIST_BUILDER_HH

#include "apl-sqlite.hh"
#include "PostgresConnection.hh"
#include "ArgListBuilder.hh"

class PostgresArg
{
public:
    virtual ~PostgresArg() {}
    virtual void update( Oid *types, const char **values, int *lengths, int *formats, int pos ) = 0;
};

template<class T>
class PostgresBindArg : public PostgresArg
{
public:
    PostgresBindArg( const T &arg_in ) : arg( arg_in ), string_arg( NULL ) {}
    virtual ~PostgresBindArg();
    virtual void update( Oid *types, const char **values, int *lengths, int *formats, int pos );

private:
    const T arg;
    char *string_arg;
};

class PostgresNullArg : public PostgresArg
{
public:
    virtual ~PostgresNullArg() {}
    virtual void update( Oid *types, const char **values, int *lengths, int *formats, int pos );
};

class PostgresArgListBuilder : public ArgListBuilder {
public:
    PostgresArgListBuilder( PostgresConnection *connection_in, const string &sql_in );
    virtual ~PostgresArgListBuilder();
    virtual void append_string( const string &arg, int pos );
    virtual void append_long( long arg, int pos );
    virtual void append_double( double arg, int pos );
    virtual void append_null( int pos );
    virtual Value_P run_query( bool ignore_result );
    virtual void clear_args( void );

private:
    PostgresConnection *connection;
    string sql;
    vector<PostgresArg *> args;
};

class PostgresResultWrapper {
public:
    PostgresResultWrapper( PGresult *result_in ) : result( result_in ) {}
    ~PostgresResultWrapper() { PQclear( result ); }
    PGresult *get_result( void ) { return result; }

private:
    PGresult *result;
};

#endif
