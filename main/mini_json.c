/**
 * @file mini_json.c
 * @brief Tiny dependency-free recursive-descent JSON parser. See mini_json.h.
 */
#include "mini_json.h"
#include <stdlib.h>
#include <string.h>

static mj_node *node_new(mj_type t) {
    mj_node *n = (mj_node *)calloc(1, sizeof(mj_node));
    if (n) n->type = t;
    return n;
}

void mj_free(mj_node *n) {
    while (n) {
        mj_node *next = n->next;
        if (n->child) mj_free(n->child);
        free(n->key);
        free(n->str);
        free(n);
        n = next;
    }
}

static void skip_ws(const char **p) {
    while (**p == ' ' || **p == '\t' || **p == '\n' || **p == '\r') (*p)++;
}

/* Parse a JSON string literal (cursor at opening quote). Returns a malloc'd,
 * unescaped, NUL-terminated string and advances the cursor past the closing
 * quote. Returns NULL on error. */
static char *parse_string(const char **p) {
    if (**p != '"') return NULL;
    (*p)++;
    size_t cap = 16, len = 0;
    char *out = (char *)malloc(cap);
    if (!out) return NULL;

    while (**p && **p != '"') {
        char c = **p;
        if (c == '\\') {
            (*p)++;
            char e = **p;
            switch (e) {
                case '"':  c = '"';  break;
                case '\\': c = '\\'; break;
                case '/':  c = '/';  break;
                case 'b':  c = '\b'; break;
                case 'f':  c = '\f'; break;
                case 'n':  c = '\n'; break;
                case 'r':  c = '\r'; break;
                case 't':  c = '\t'; break;
                case 'u': {
                    unsigned cp = 0;
                    for (int i = 0; i < 4; i++) {
                        (*p)++;
                        char h = **p;
                        cp <<= 4;
                        if      (h >= '0' && h <= '9') cp |= (unsigned)(h - '0');
                        else if (h >= 'a' && h <= 'f') cp |= (unsigned)(h - 'a' + 10);
                        else if (h >= 'A' && h <= 'F') cp |= (unsigned)(h - 'A' + 10);
                        else { free(out); return NULL; }
                    }
                    char buf[4]; int nb;
                    if (cp < 0x80)       { buf[0] = (char)cp; nb = 1; }
                    else if (cp < 0x800) { buf[0] = (char)(0xC0 | (cp >> 6)); buf[1] = (char)(0x80 | (cp & 0x3F)); nb = 2; }
                    else                 { buf[0] = (char)(0xE0 | (cp >> 12)); buf[1] = (char)(0x80 | ((cp >> 6) & 0x3F)); buf[2] = (char)(0x80 | (cp & 0x3F)); nb = 3; }
                    for (int i = 0; i < nb; i++) {
                        if (len + 1 >= cap) { cap *= 2; char *t = (char *)realloc(out, cap); if (!t) { free(out); return NULL; } out = t; }
                        out[len++] = buf[i];
                    }
                    (*p)++;      /* past last hex digit */
                    continue;    /* bytes already appended */
                }
                default: free(out); return NULL;
            }
            (*p)++;              /* past the escaped char */
        } else {
            (*p)++;              /* past the plain char */
        }
        if (len + 1 >= cap) { cap *= 2; char *t = (char *)realloc(out, cap); if (!t) { free(out); return NULL; } out = t; }
        out[len++] = c;
    }

    if (**p != '"') { free(out); return NULL; }
    (*p)++;                      /* past closing quote */
    out[len] = '\0';
    return out;
}

static mj_node *parse_value(const char **p);

static mj_node *parse_object(const char **p) {
    mj_node *obj = node_new(MJ_OBJ);
    if (!obj) return NULL;
    (*p)++;                      /* '{' */
    skip_ws(p);
    if (**p == '}') { (*p)++; return obj; }

    mj_node *tail = NULL;
    for (;;) {
        skip_ws(p);
        if (**p != '"') { mj_free(obj); return NULL; }
        char *key = parse_string(p);
        if (!key) { mj_free(obj); return NULL; }
        skip_ws(p);
        if (**p != ':') { free(key); mj_free(obj); return NULL; }
        (*p)++;
        mj_node *val = parse_value(p);
        if (!val) { free(key); mj_free(obj); return NULL; }
        val->key = key;
        if (tail) tail->next = val; else obj->child = val;
        tail = val;
        skip_ws(p);
        if (**p == ',') { (*p)++; continue; }
        if (**p == '}') { (*p)++; break; }
        mj_free(obj); return NULL;
    }
    return obj;
}

static mj_node *parse_array(const char **p) {
    mj_node *arr = node_new(MJ_ARR);
    if (!arr) return NULL;
    (*p)++;                      /* '[' */
    skip_ws(p);
    if (**p == ']') { (*p)++; return arr; }

    mj_node *tail = NULL;
    for (;;) {
        mj_node *val = parse_value(p);
        if (!val) { mj_free(arr); return NULL; }
        if (tail) tail->next = val; else arr->child = val;
        tail = val;
        skip_ws(p);
        if (**p == ',') { (*p)++; continue; }
        if (**p == ']') { (*p)++; break; }
        mj_free(arr); return NULL;
    }
    return arr;
}

static mj_node *parse_value(const char **p) {
    skip_ws(p);
    char c = **p;
    if (c == '{') return parse_object(p);
    if (c == '[') return parse_array(p);
    if (c == '"') {
        char *s = parse_string(p);
        if (!s) return NULL;
        mj_node *n = node_new(MJ_STR);
        if (!n) { free(s); return NULL; }
        n->str = s;
        return n;
    }
    if (c == 't' || c == 'f') {
        mj_node *n = node_new(MJ_BOOL);
        if (!n) return NULL;
        if      (strncmp(*p, "true", 4)  == 0) { n->num = 1; *p += 4; }
        else if (strncmp(*p, "false", 5) == 0) { n->num = 0; *p += 5; }
        else { free(n); return NULL; }
        return n;
    }
    if (c == 'n') {
        if (strncmp(*p, "null", 4) == 0) { *p += 4; return node_new(MJ_NULL); }
        return NULL;
    }
    if (c == '-' || (c >= '0' && c <= '9')) {
        char *end;
        double v = strtod(*p, &end);
        if (end == *p) return NULL;
        mj_node *n = node_new(MJ_NUM);
        if (!n) return NULL;
        n->num = v;
        *p = end;
        return n;
    }
    return NULL;
}

mj_node *mj_parse(const char *text) {
    if (!text) return NULL;
    const char *p = text;
    return parse_value(&p);
}

mj_node *mj_get(const mj_node *obj, const char *key) {
    if (!obj || obj->type != MJ_OBJ || !key) return NULL;
    for (mj_node *c = obj->child; c; c = c->next)
        if (c->key && strcmp(c->key, key) == 0) return c;
    return NULL;
}

int mj_int(const mj_node *n, int def) {
    if (n && (n->type == MJ_NUM || n->type == MJ_BOOL)) return (int)n->num;
    return def;
}
double mj_double(const mj_node *n, double def) {
    if (n && (n->type == MJ_NUM || n->type == MJ_BOOL)) return n->num;
    return def;
}
bool mj_bool(const mj_node *n, bool def) {
    if (n && (n->type == MJ_BOOL || n->type == MJ_NUM)) return n->num != 0;
    return def;
}
const char *mj_str(const mj_node *n, const char *def) {
    if (n && n->type == MJ_STR && n->str) return n->str;
    return def;
}
