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
#include "aklisp.h"

/* Starting size of the buffer */
#define DEF_BUFFER_SIZE 50

static void init_buffer(struct akl_io_device *dev)
{
    assert(dev);
    dev->iod_buffer = (char *)akl_malloc(NULL, DEF_BUFFER_SIZE);
    dev->iod_buffer_size = DEF_BUFFER_SIZE;
    dev->iod_tokens.av_vector = NULL;
}

void akl_lex_free(struct akl_io_device *dev)
{
    if (!dev)
        return;

    dev->iod_buffer_size = 0;
    akl_vector_destroy(&dev->iod_tokens);
    FREE_FUNCTION(dev->iod_buffer);
    dev->iod_buffer = NULL;
}

static void put_buffer(struct akl_io_device *dev, int pos, char ch)
{
    if (pos+1 >= dev->iod_buffer_size) {
        dev->iod_buffer_size = dev->iod_buffer_size
                + (dev->iod_buffer_size / 2);
        dev->iod_buffer = realloc(dev->iod_buffer, dev->iod_buffer_size);
        if (dev->iod_buffer == NULL) {
            fprintf(stderr, "ERROR! No memory left!\n");
            exit(1);
        }
    }
    dev->iod_buffer[pos]   = ch;
    /* XXX: Take this serious! */
    dev->iod_buffer[++pos] = '\0';
}

int akl_io_getc(struct akl_io_device *dev)
{
    if (dev == NULL)
        return EOF;

    dev->iod_char_count++;
    char ch;
    switch (dev->iod_type) {
        case DEVICE_FILE:
        ch = fgetc(dev->iod_source.file);
        break;
        
        case DEVICE_STRING:
        ch = dev->iod_source.string[dev->iod_pos++];
        break;
    }
    return ch;
}

int akl_io_ungetc(int ch, struct akl_io_device *dev)
{
    if (dev == NULL)
        return EOF;

    dev->iod_char_count--;
    switch (dev->iod_type) {
        case DEVICE_FILE:
        return ungetc(ch, dev->iod_source.file);

        case DEVICE_STRING:
        dev->iod_pos--;
        return dev->iod_source.string[dev->iod_pos];
    }
    return 0;
}

bool_t akl_io_eof(struct akl_io_device *dev)
{
    if (dev == NULL)
        return EOF;

    switch (dev->iod_type) {
        case DEVICE_FILE:
        return feof(dev->iod_source.file);

        case DEVICE_STRING:
        return dev->iod_source.string[dev->iod_pos] == '\0' ? 1 : 0;
    }
}

size_t copy_number(struct akl_io_device *dev, char op)
{
    int ch;
    size_t i = 0;
    bool_t is_hexa = FALSE;
    assert(dev);
    if (op != 0)
        put_buffer(dev, i++, op);

    while ((ch = akl_io_getc(dev))) {
        /* The second character can be 'x' (because of the hexa numbers) */
        if (isdigit(ch)) {
            put_buffer(dev, i++, ch);
        } else if (is_hexa) {
            put_buffer(dev, i++, ch);
        } else {
            akl_io_ungetc(ch, dev);
            break;
        }
        if (akl_io_eof(dev))
            break;
    }
    return i;
}

size_t copy_string(struct akl_io_device *dev)
{
    int ch;
    size_t i = 0;
    assert(dev);
    
    while ((ch = akl_io_getc(dev))) {
        if (ch == '\\') { // \" escaping
            ch = akl_io_getc(dev);
            if (ch == '"') {
                put_buffer(dev, i++, ch);
            } else {
                put_buffer(dev, i++, '\\');
                put_buffer(dev, i++, ch);
            }
        } else if (ch != '\"') {
            put_buffer(dev, i++, ch);
        } else {
            break;
        }
        if (akl_io_eof(dev))
            break;
    }
    return i;
}

size_t copy_atom(struct akl_io_device *dev)
{
    int ch;
    size_t i = 0;
    assert(dev);

    while ((ch = akl_io_getc(dev))) {
        if (ch != ' ' && ch != ')' && ch != '\n') {
            put_buffer(dev, i++, toupper(ch)); /* Good old times... */
        } else {
            akl_io_ungetc(ch, dev);
            break;
        }
        if (akl_io_eof(dev))
            break;
    }
    return i;
}

akl_token_t akl_lex(struct akl_io_device *dev)
{
    int ch;
    /* We should take care of the '+', '++',
      and etc. style functions. Moreover the
      positive and negative numbers must also work:
      '(++ +5)' should be valid. */
    char op = 0;
    if (!dev->iod_buffer)
        init_buffer(dev);

    assert(dev);
    while ((ch = akl_io_getc(dev))) {
        /* We should avoid the interpretation of the Unix shebang */
        if (dev->iod_char_count == 1 && ch == '#') {
            while ((ch = akl_io_getc(dev)) && ch != '\n') 
                ;
        }
        if (ch == EOF) {
           return tEOF;
        } else if (ch == '\n') {
           dev->iod_line_count++;
           dev->iod_char_count = 0;
        } else if (ch == '+' || ch == '-') {
            if (op != 0) {
                if (op == '+')
                    strcpy(dev->iod_buffer, "++");
                else
                    strcpy(dev->iod_buffer, "--");
                op = 0;
                return tATOM;
            }
            op = ch;
        } else if (isdigit(ch)) {
            akl_io_ungetc(ch, dev);
            copy_number(dev, op);
            op = 0;
            return tNUMBER;
        } else if (ch == ' ' || ch == '\n') {
            dev->iod_column = dev->iod_char_count+1;
            if (op != 0) {
                if (op == '+')
                    strcpy(dev->iod_buffer, "+");
                else
                    strcpy(dev->iod_buffer, "-");
                op = 0;
                return tATOM;
            } else {
                continue;
            }
        } else if (ch == '"') {
            ch = akl_io_getc(dev);
            if (ch == '"')
                return tNIL;
            akl_io_ungetc(ch, dev);
            copy_string(dev); 
            return tSTRING;
        } else if (ch == '(') {
            dev->iod_column = dev->iod_char_count+1;
            ch = akl_io_getc(dev);
            if (ch == ')')
                return tNIL;
            akl_io_ungetc(ch, dev);
            return tLBRACE;
        } else if (ch == ')') {
            return tRBRACE;
        } else if (ch == '\'' || ch == ':') {
            return tQUOTE;
        } else if (ch == ';') {
            while ((ch = akl_io_getc(dev)) != '\n') {
                if (akl_io_eof(dev))
                    return tEOF;
            }
        } else if (isalpha(ch) || ispunct(ch)) {
            akl_io_ungetc(ch, dev);
            copy_atom(dev);
            if ((strcasecmp(dev->iod_buffer, "NIL")) == 0)
                return tNIL;
            else if ((strcasecmp(dev->iod_buffer, "T")) == 0)
                return tTRUE;
            else
                return tATOM;
        } else {
            continue;
        }
    }
    return tEOF;
}

akl_token_t akl_lex_get(struct akl_io_device *dev)
{
    if (dev && dev->iod_tokens.av_vector == NULL) {
    }
}

char *akl_lex_get_string(struct akl_io_device *dev)
{
    char *str = strdup(dev->iod_buffer);
    dev->iod_buffer[0] = '\0';
    return str;
}

char *akl_lex_get_atom(struct akl_io_device *dev)
{
    return akl_lex_get_string(dev);
}

double akl_lex_get_number(struct akl_io_device *dev)
{
    double n;
    unsigned int o; /* Should be type safe */
    /* strtod() do not handle octal numbers */
    if (dev->iod_buffer[0] == '0' && tolower(dev->iod_buffer[1]) != 'x') {
        sscanf(dev->iod_buffer, "%o", &o);
        n = (double)o;
    } else {
        n = strtod(dev->iod_buffer, NULL);
    }
    dev->iod_buffer[0] = '\0';
    return n;
}
