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


/// return the path to the APL binary (excluding the binary itself).
/// used to find collocated programs like APs.
extern const char * get_APL_bin_path();

/// return the path to the APL libraries (excluding the libraries itself).
/// used to resolve library numbers to paths
extern const char * get_APL_lib_root();

/// set the path to the APL libraries
extern void set_APL_lib_root(const char * new_root);

/// clean up
extern void cleanup();

/// true if Control-C was hit
extern bool attention_raised;

/// true if Control-C was hit twice within 500 ms
extern bool interrupt_raised;

/// true if no banner/Goodbye is wanted.
extern bool silent;

/// true if no banner/Goodbye is wanted.
extern bool do_not_echo;
