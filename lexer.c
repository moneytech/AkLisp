#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "aklisp.h"

static char buffer[256];

struct akl_io_device *
akl_new_file_device(FILE *fp)
{
    struct akl_io_device *dev;
    dev = AKL_MALLOC(akl_io_device);
    dev->iod_type = DEVICE_FILE;
    dev->iod_source.file = fp;
    dev->iod_pos = 0;
    dev->iod_strlen = 0;
    return dev;
}

struct akl_io_device *
akl_new_string_device(const char *str)
{
    struct akl_io_device *dev;
    dev = AKL_MALLOC(akl_io_device);
    dev->iod_type = DEVICE_STRING;
    dev->iod_source.string = str;
    dev->iod_pos = 0;
    dev->iod_strlen = strlen(str);
    return dev;
}

int akl_io_getc(struct akl_io_device *dev)
{
    if (dev == NULL)
        return EOF;
    switch (dev->iod_type) {
        case DEVICE_FILE:
        return fgetc(dev->iod_source.file);

        case DEVICE_STRING:
        if (dev->iod_strlen > dev->iod_pos)
            return dev->source.string[dev->iod_pos];
        else 
            return EOF;
    }
}

int akl_io_ungetc(struct akl_io_device *dev, int ch)
{
    if (dev == NULL)
        return EOF;
    switch (dev->iod_type) {
        case DEVICE_FILE:
        return ungetc(ch, dev->iod_source.file);

        case DEVICE_STRING:
        dev->iod_pos--;
        return dev->source.string[dev->iod_pos];
    }
}

bool_t akl_io_eof(struct akl_io_device *dev)
{
    if (dev == NULL)
        return EOF;

    switch (dev->iod_type) {
        case DEVICE_FILE:
        return feof(dev->iod_source.file);

        case DEVICE_STRING:
        if (dev->iod_strlen > dev->iod_pos)
            return FALSE; 
        else 
            return TRUE;
    }
}

size_t copy_number(struct akl_io_device *dev)
{
    int ch;
    size_t i = 0;
    assert(dev);
    while ((ch = akl_io_getc(dev))) {
        if (akl_io_eof(dev))
            break;

        if (isdigit(ch)) {
            buffer[i] = ch;
            buffer[++i] = '\0';
        } else {
            akl_io_ungetc(ch, dev);
            break;
        }
    }

    buffer[i] = '\0';
    return i;
}

size_t copy_string(struct akl_io_device *dev)
{
    int ch;
    size_t i = 0;
    assert(dev);
    while ((ch = akl_io_getc(dev))) {
        if (akl_io_eof(file))
            break;
        if (ch != '\"') {
            buffer[i] = ch;
            buffer[++i] = '\0';
        } else {
            break;
        }
    }

    buffer[i] = '\0';
    return i;
}

size_t copy_atom(struct akl_io_device *dev)
{
    int ch;
    size_t i = 0;

    assert(dev);
    while ((ch = akl_io_getc(dev))) {
        if (akl_io_eof(dev))
            break;
        if (ch != ' ' && ch != ')' && ch != '\n') {
            buffer[i] = toupper(ch); /* Good old times... */
            buffer[++i] = '\0';
        } else {
            akl_io_ungetc(ch, dev);
            break;
        }
    }

    buffer[i] = '\0';
    return i;
}

token_t akl_lex(struct akl_io_device *dev)
{
    int ch;
    int i;
    assert(dev);
    if (akl_io_eof(dev))
        return tEOF;

    while ((ch = akl_io_ungetc(dev))) {
        if (isdigit(ch)) {
            akl_io_ungetc(ch, dev);
            copy_number(dev);
            return tNUMBER;
        } else if (ch == ' ' || ch == '\n') {
            continue;
        } else if (ch == '"') {
            /* No akl_io_ungetc() */
            copy_string(dev); 
            return tSTRING;
        } else if (ch == '(') {
            return tLBRACE;
        } else if (ch == ')') {
            return tRBRACE;
        } else if (ch == '\'') {
            return tQUOTE;
        } else if (isalpha(ch) || ispunct(ch)) {
            akl_io_ungetc(ch, dev);
            copy_atom(dev);
            if ((strcasecmp(buffer, "NIL")) == 0)
                return tNIL;
            else if ((strcasecmp(buffer, "T")) == 0)
                return tTRUE;
            else
                return tATOM;
        } else {
            continue;
        }
    }
    return tEOF;
}

char *akl_lex_get_string(void)
{
    return strdup(buffer);   
}

char *akl_lex_get_atom(void)
{
    return strdup(buffer);
}

int akl_lex_get_number(void)
{
    return atoi(buffer);
}
