/*
    This file is part of GNU APL, a free implementation of the
    ISO/IEC Standard 13751, "Programming Language APL, Extended"

    Copyright (C) 2013  Dr. Jürgen Sauermann

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "Error.hh"
#include "LibPaths.hh"
#include "PrintOperator.hh"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

bool LibPaths::root_from_env = false;
bool LibPaths::root_from_pwd = false;

char LibPaths::APL_bin_path[PATH_MAX + 1] = "";
const char * LibPaths::APL_bin_name = LibPaths::APL_bin_path;
char LibPaths::APL_lib_root[PATH_MAX + 10] = "";

LibPaths::LibDir LibPaths::lib_dirs[LIB_MAX];

const void * unused = 0;

//-----------------------------------------------------------------------------
void LibPaths::init(const char * argv0)
{
   compute_bin_path(argv0);
   search_APL_lib_root();

   loop(d, LIB_MAX)
      {
        if (root_from_env)        lib_dirs[d].cfg_src = LibDir::CS_ENV;
        else if (root_from_pwd)   lib_dirs[d].cfg_src = LibDir::CS_ARGV0;
        else                      lib_dirs[d].cfg_src = LibDir::CS_NONE;
      }
}
//-----------------------------------------------------------------------------
void LibPaths::compute_bin_path(const char * argv0)
{
   // compute APL_bin_path from argv0
   //
   if (strchr(argv0, '/') == 0)
      {
         // if argv0 contains no / then realpath() seems to prepend the current
         // directory to argv0 (which is wrong since argv0 may be in $PATH).
         //
         // we fix this by searching argv0 in $PATH
         //
         const char * path = getenv("PATH");   // must NOT be modified

         if (path)
            {
              // we must not modify path, so we copy it to path1 and
              // replace the semicolons in path1 by 0. That converts
              // p1;p2; ... into a sequence of 0-terminated strings
              // p1 p2 ... The variable next points to the start of each
              // string.
              //
              const size_t alen = strlen(argv0);
              const size_t plen = strlen(path);
              DynArray(char, path1, plen + 1);
              strncpy(&path1[0], path, plen + 1);
              char * next = &path1[0];
              for (;;)
                  {
                    char * semi = strchr(next, ':');
                    if (semi)   *semi = 0;
                    DynArray(char, filename, plen + alen + 10);
                    snprintf(&filename[0], plen + alen + 8, "%s/%s",
                             next, argv0);

                    if (access(&filename[0], X_OK) == 0)
                       {
                         strncpy(APL_bin_path, next, sizeof(APL_bin_path));
                         return;
                       }

                    if (semi == 0)   break;
                    next = semi + 1;
                  }
            }
      }

   unused = realpath(argv0, APL_bin_path);
   APL_bin_path[PATH_MAX] = 0;
char * slash =   strrchr(APL_bin_path, '/');
   if (slash)   { *slash = 0;   APL_bin_name = slash + 1; }
   else         { APL_bin_name = APL_bin_path;            }

   // if we have a PWD and it is a prefix of APL_bin_path then replace PWD
   // by './'
   //
const char * PWD = getenv("PWD");
   if (PWD)   // we have a pwd
      {
        const int PWD_len = strlen(PWD);
        if (!strncmp(PWD, APL_bin_path, PWD_len) && PWD_len > 1)
           {
             strcpy(APL_bin_path + 1, APL_bin_path + PWD_len);
             APL_bin_path[0] = '.';
           }
      }
}
//-----------------------------------------------------------------------------
bool
LibPaths::is_lib_root(const char * dir)
{
char filename[PATH_MAX + 1];

   snprintf(filename, sizeof(filename), "%s/workspaces", dir);
   if (access(filename, F_OK))   return false;

   snprintf(filename, sizeof(filename), "%s/wslib1", dir);
   if (access(filename, F_OK))   return false;

   return true;
}
//-----------------------------------------------------------------------------
void
LibPaths::search_APL_lib_root()
{
const char * path = getenv("APL_LIB_ROOT");
   if (path)
      {
        unused = realpath(path, APL_lib_root);
        root_from_env = true;
        return;
      }

   // search from "." to "/" for  a valid lib-root
   //
int last_len = 2*PATH_MAX;
   APL_lib_root[0] = '.';
   APL_lib_root[1] = 0;
   for (;;)
       {
         unused = realpath(APL_lib_root, APL_lib_root);
         int len = strlen(APL_lib_root);

         if (is_lib_root(APL_lib_root))   return;   // lib-rot found

         // if length has not decreased then we stop in order to
         // avoid an endless loop.
         //
         if (len >= last_len)   break;
         last_len = len;

         // move up by appending /..
         //
         APL_lib_root[len++] = '/';
         APL_lib_root[len++] = '.';
         APL_lib_root[len++] = '.';
         APL_lib_root[len++] = 0;
       }

   // no lib-root found. use ".";
   //
   unused = realpath(".", APL_lib_root);
   root_from_pwd = true;
}
//-----------------------------------------------------------------------------
void
LibPaths::set_APL_lib_root(const char * new_root)
{
   unused = realpath(new_root, APL_lib_root);
}
//-----------------------------------------------------------------------------
void
LibPaths::set_lib_dir(LibRef libref, const char * path, LibDir::CfgSrc src)
{
   lib_dirs[libref].dir_path = UTF8_string(path);
   lib_dirs[libref].cfg_src = src;
}
//-----------------------------------------------------------------------------
UTF8_string
LibPaths::get_lib_dir(LibRef libref)
{
   switch(lib_dirs[libref].cfg_src)
      {
        case LibDir::CS_NONE: return UTF8_string();

        case LibDir::CS_ENV:
        case LibDir::CS_ARGV0:     break;   // continue below

        case LibDir::CS_PREF_SYS:
        case LibDir::CS_PREF_HOME: return lib_dirs[libref].dir_path;

      }

UTF8_string ret(APL_lib_root);
   if (libref == LIB0)   // workspaces
      {
        const UTF8_string subdir("/workspaces");
        ret.append(subdir);
      }
   else                  // wslibN
      {
        const UTF8_string subdir("/wslib");
        ret.append(subdir);
        ret.append(Unicode(libref + '0'));
      }

   return ret;
}
//-----------------------------------------------------------------------------
void
LibPaths::maybe_warn_ambiguous(int name_has_extension, const UTF8_string name,
                               const char * ext1, const char * ext2)
{
   if (name_has_extension)   return;   // extension was provided
   if (ext2 == 0)            return;   // no second extension

UTF8_string filename_ext2 = name;
   filename_ext2.append_str(ext2);
   if (access(filename_ext2.c_str(), F_OK))   return;   // not existing

   CERR << endl 
        << "WARNING: filename " << name << endl
        << "    is ambiguous because another file" << endl << "    "
        << filename_ext2 << endl
        << "    exists as well. Using the first." << endl << endl;
}
//-----------------------------------------------------------------------------
UTF8_string
LibPaths::get_lib_filename(LibRef lib, const UTF8_string & name, 
                           bool existing, const char * ext1, const char * ext2)
{
   // check if name has one of the extensions ext1 or ext2 already.
   //
int name_has_extension = 0;   // assume name has neither extension ext1 nor ext2
   if      (name.ends_with(ext1))   name_has_extension = 1;
   else if (name.ends_with(ext2))   name_has_extension = 2;

   if (name.starts_with("/")   || 
       name.starts_with("./")  || 
       name.starts_with("../"))
      {
        // paths from / or ./ are fallbacks for the case where the library
        // path setup id wrong. So that the user can survive by using an
        // explicit path
        //
        if (name_has_extension)   return name;

        UTF8_string filename(name);
        if (access(filename.c_str(), F_OK) == 0)   return filename;

        if (ext1)
           {
             UTF8_string filename_ext1 = name;
             filename_ext1.append_str(ext1);
             if (!access(filename_ext1.c_str(), F_OK))
                {
                   // filename_ext1 exists, but filename_ext2 may exist as well.
                   // warn user unless an explicit extension was given.
                   //
                   maybe_warn_ambiguous(name_has_extension, name, ext1, ext2);
                   return filename_ext1;
                }
           }

        if (ext2)
           {
             UTF8_string filename_ext2 = name;
             filename_ext2.append_str(ext2);
             if (!access(filename_ext2.c_str(), F_OK))   return filename_ext2;
           }

         // neither ext1 nor ext2 worked: return original name
         //
         filename = name;
         return filename;
      }

UTF8_string filename = get_lib_dir(lib);
   filename.append(UNI_ASCII_SLASH);
   filename.append(name);

   if (name_has_extension)   return filename;

   if (existing)
      {
        // file ret is supposed to exist (and will be openend read-only).
        // If it does return filename otherwise filename.extension.
        //
        if (access(filename.c_str(), F_OK) == 0)   return filename;

        if (ext1)
           {
             UTF8_string filename_ext1 = filename;
             filename_ext1.append_str(ext1);
             if (!access(filename_ext1.c_str(), F_OK))
                {
                  maybe_warn_ambiguous(name_has_extension,
                                       filename, ext1, ext2);
                   return filename_ext1;
                }
           }

        if (ext2)
           {
             UTF8_string filename_ext2 = filename;
             filename_ext2.append_str(ext2);
             if (!access(filename_ext2.c_str(), F_OK))   return filename_ext2;
           }

        return filename;   // without ext
      }
   else
      {
        // file may or may not exist (and will be created if not).
        // therefore checking the existence does not work.
        // check that the file ends with ext1 or ext2 if provided
        //
        if (name_has_extension)   return filename;

        if      (ext1) filename.append_str(ext1);
        else if (ext2) filename.append_str(ext2);
        return filename;
      }
}
//-----------------------------------------------------------------------------

