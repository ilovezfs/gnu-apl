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

#ifndef SQLITE_CONNECTION_HH
#define SQLITE_CONNECTION_HH

#include "Connection.hh"

#include <sqlite3.h>

class SqliteConnection : public Connection {
public:
    SqliteConnection( sqlite3 *db_in ) : db( db_in ) {}
    virtual ~SqliteConnection();
    virtual ArgListBuilder *make_prepared_query( const string &sql );
    virtual ArgListBuilder *make_prepared_update( const string &sql );

    virtual void transaction_begin();
    virtual void transaction_commit();
    virtual void transaction_rollback();

    virtual void fill_tables( vector<string> &tables );
    virtual void fill_cols( const string &table, vector<ColumnDescriptor> &cols );
    virtual const string make_positional_param( int pos );

    void raise_sqlite_error( const string &message );
    sqlite3 *get_db( void ) { return db; }

private:
    sqlite3 *db;
    void run_simple( const string &sql );
};

class SqliteStmtWrapper {
public:
    SqliteStmtWrapper( sqlite3_stmt *statement_in ) : statement( statement_in ) {}
    virtual ~SqliteStmtWrapper() { sqlite3_finalize( statement ); }
    sqlite3_stmt *get_statement( void ) { return statement; }

private:
    sqlite3_stmt *statement;
};

#endif
