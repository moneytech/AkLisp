/************************************************************************
 *   Copyright (c) 2012 Ákos Kovács - AkLisp Lisp dialect
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 ************************************************************************/
#include <aklisp.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <glob.h>

AKL_CFUN_DEFINE(mkdir, in, args)
{
    struct akl_value *a1, *a2;
    char *dirname;
    int rights = 01644;
    a1 = AKL_FIRST_VALUE(args);
    if (AKL_CHECK_TYPE(a1, TYPE_STRING)) {
        dirname = AKL_GET_STRING_VALUE(a1);
        a2 = AKL_SECOND_VALUE(args);
        if (AKL_CHECK_TYPE(a2, TYPE_NUMBER))
            rights = AKL_GET_NUMBER_VALUE(a2);

        if (mkdir(dirname, rights) != 0)
            return &NIL_VALUE;
    } else {
        akl_add_error(in, AKL_ERROR, a1->va_lex_info
            , "ERROR: mkdir: First argument should be a string");
        return &NIL_VALUE;
    }
    return &TRUE_VALUE;
}

static struct akl_value *rm_function(struct akl_state *in
            , struct akl_list *args, int (*rmfun)(const char *))
{
    struct akl_value *a1 = AKL_FIRST_VALUE(args);
    
    if (AKL_CHECK_TYPE(a1, TYPE_STRING)) {
        if (rmfun((AKL_GET_STRING_VALUE(a1))) != 0)
            return &NIL_VALUE;
    } else {
        akl_add_error(in, AKL_ERROR, a1->va_lex_info
            , "ERROR: mkdir: First argument should be a string");
        return &NIL_VALUE;
    }
    return a1;
}

AKL_CFUN_DEFINE(unlink, in, args)
{
    return rm_function(in, args, unlink);
}

AKL_CFUN_DEFINE(remove, in, args)
{
    return rm_function(in, args, remove);
}

AKL_CFUN_DEFINE(rmdir, in, args)
{
    return rm_function(in, args, rmdir);
}

AKL_CFUN_DEFINE(dirlist, in, args)
{
    struct akl_value *a1 = AKL_FIRST_VALUE(args);
    struct akl_list *list;    
    DIR *dir;
    struct dirent *ent;
    struct akl_value *ret;
    if (AKL_CHECK_TYPE(a1, TYPE_STRING)) {
        dir = opendir(AKL_GET_STRING_VALUE(a1));
        if (!dir)
            return &NIL_VALUE;
        list = akl_new_list(in); 
        while ((ent = readdir(dir)) != NULL) {
            akl_list_append(in, list
                , akl_new_string_value(in, strdup(ent->d_name)));
        }
        closedir(dir);
        ret = akl_new_list_value(in, list);
        list->is_quoted = TRUE;
        ret->is_quoted = TRUE;
        return ret;
    } else {
        akl_add_error(in, AKL_ERROR, a1->va_lex_info
            , "ERROR: mkdir: First argument must be a string");
    }
    return &NIL_VALUE;
}

AKL_CFUN_DEFINE(glob, in, args)
{
    struct akl_value *a1 = AKL_FIRST_VALUE(args);
    struct akl_value *lv;
    struct akl_list *list;
    char **gdir;
    glob_t dirs;
    if (AKL_CHECK_TYPE(a1, TYPE_STRING)) {
        if (glob(AKL_GET_STRING_VALUE(a1), 0, NULL, &dirs) == 0) {
            list = akl_new_list(in);
            gdir = dirs.gl_pathv;
            while (*gdir) {
                akl_list_append(in, list, akl_new_string_value(in, *gdir));
                gdir++;
            }
            lv = akl_new_list_value(in, list);
            list->is_quoted = TRUE;
            lv->is_quoted = TRUE;
            return lv;
        }
    } else {
        akl_add_error(in, AKL_ERROR, a1->va_lex_info
            , "ERROR: glob: First argument must be a string");
    }
    return &NIL_VALUE;
}

static int dir_load(struct akl_state *in)
{
    AKL_ADD_CFUN(in, mkdir, "MKDIR", "Create directories, with the given rights");
    AKL_ADD_CFUN(in, unlink, "UNLINK", "Unlink files");
    AKL_ADD_CFUN(in, remove, "REMOVE", "Delete files");
    AKL_ADD_CFUN(in, rmdir, "RMDIR", "Remove directories");
    AKL_ADD_CFUN(in, dirlist, "DIRLIST", "List all directory entries as a list");
    AKL_ADD_CFUN(in, glob, "GLOB", "Searches for all pathnames, matching a given pattern ");
    return AKL_LOAD_OK;
}

static int dir_unload(struct akl_state *in)
{
    AKL_REMOVE_CFUN(in, mkdir);
    AKL_REMOVE_CFUN(in, unlink);
    AKL_REMOVE_CFUN(in, remove);
    AKL_REMOVE_CFUN(in, rmdir);
    AKL_REMOVE_CFUN(in, dirlist);
    AKL_REMOVE_CFUN(in, glob);
    return AKL_LOAD_OK;
}

AKL_MODULE_DEFINE(dir_load, dir_unload, "dir"
    , "Directory managing functions", "Kovacs Akos");
