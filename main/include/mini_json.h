/**
 * @file mini_json.h
 * @brief Tiny dependency-free JSON parser (recursive descent).
 *
 * Just enough to read the machine config (objects, arrays, strings, numbers,
 * bools, null). No serializer — writing is done by hand with fprintf in
 * writeConfigFile. Builds on both the ESP32 board and the PC simulator.
 *
 * Usage:
 *   mj_node *root = mj_parse(text);
 *   mj_node *sp   = mj_get(root, "settingsParams");
 *   int spd       = mj_int(mj_get(sp, "pumpSpeed"), 30);
 *   ... iterate an array with for(mj_node*c = arr->child; c; c = c->next) ...
 *   mj_free(root);
 */
#ifndef MINI_JSON_H
#define MINI_JSON_H

#include <stdbool.h>

typedef enum { MJ_NULL, MJ_BOOL, MJ_NUM, MJ_STR, MJ_ARR, MJ_OBJ } mj_type;

typedef struct mj_node {
    mj_type          type;
    char            *key;      /* member key when this node is an object member (owned) */
    double           num;      /* MJ_NUM / MJ_BOOL (0/1) */
    char            *str;      /* MJ_STR text (owned, unescaped) */
    struct mj_node  *child;    /* first child (MJ_OBJ / MJ_ARR) */
    struct mj_node  *next;     /* next sibling */
} mj_node;

/* Parse a NUL-terminated JSON string. Returns the root node or NULL on error.
 * The caller owns the tree and must free it with mj_free(). */
mj_node *mj_parse(const char *text);
void     mj_free(mj_node *node);

/* Object member lookup by key (NULL-safe). Returns NULL if absent. */
mj_node *mj_get(const mj_node *obj, const char *key);

/* Typed accessors with a fallback if the node is NULL / wrong type. */
int         mj_int(const mj_node *n, int def);
double      mj_double(const mj_node *n, double def);
bool        mj_bool(const mj_node *n, bool def);
const char *mj_str(const mj_node *n, const char *def);

#endif /* MINI_JSON_H */
