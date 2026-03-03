#include "searcher.h"
#include "stemmer.h"
#include <cstring>
#include <cstdlib>

struct Parser {
    const char* input;
    int pos;
    int len;
    HashMap* index;
    uint32_t num_docs;
};

static void skip_spaces(Parser* p) {
    while (p->pos < p->len && p->input[p->pos] == ' ') p->pos++;
}

static UInt32Array parse_expr(Parser* p);

static UInt32Array parse_word(Parser* p) {
    skip_spaces(p);
    char buf[256];
    int n = 0;
    while (p->pos < p->len && p->input[p->pos] != ' ' &&
           p->input[p->pos] != '(' && p->input[p->pos] != ')' &&
           p->input[p->pos] != '!' && p->input[p->pos] != '|' &&
           p->input[p->pos] != '&') {
        char c = p->input[p->pos];
        if (c >= 'A' && c <= 'Z') c = c - 'A' + 'a';
        if (n < 255) buf[n++] = c;
        p->pos++;
    }
    buf[n] = '\0';

    MyString term;
    mystr_init_buf(&term, buf, n);
    porter_stem(&term);

    UInt32Array result;
    u32arr_init(&result);
    UInt32Array* pl = hashmap_get(p->index, term.data, term.len);
    if (pl) {
        for (size_t i = 0; i < pl->len; i++) {
            u32arr_push(&result, pl->data[i]);
        }
    }
    mystr_free(&term);
    return result;
}

static UInt32Array parse_atom(Parser* p) {
    skip_spaces(p);
    if (p->pos < p->len && p->input[p->pos] == '(') {
        p->pos++;
        UInt32Array r = parse_expr(p);
        skip_spaces(p);
        if (p->pos < p->len && p->input[p->pos] == ')') p->pos++;
        return r;
    }
    return parse_word(p);
}

static UInt32Array parse_not(Parser* p) {
    skip_spaces(p);
    if (p->pos < p->len && p->input[p->pos] == '!') {
        p->pos++;
        UInt32Array inner = parse_not(p);
        u32arr_sort(&inner);
        UInt32Array result = u32arr_subtract_from_range(p->num_docs, &inner);
        u32arr_free(&inner);
        return result;
    }
    return parse_atom(p);
}

static bool at_end_or_close(Parser* p) {
    skip_spaces(p);
    return p->pos >= p->len || p->input[p->pos] == ')';
}

static bool peek_or(Parser* p) {
    skip_spaces(p);
    return (p->pos + 1 < p->len && p->input[p->pos] == '|' && p->input[p->pos + 1] == '|');
}

static bool peek_and_explicit(Parser* p) {
    skip_spaces(p);
    return (p->pos + 1 < p->len && p->input[p->pos] == '&' && p->input[p->pos + 1] == '&');
}

static UInt32Array parse_and(Parser* p) {
    UInt32Array left = parse_not(p);
    u32arr_sort(&left);

    while (!at_end_or_close(p) && !peek_or(p)) {
        if (peek_and_explicit(p)) {
            p->pos += 2;
        }
        UInt32Array right = parse_not(p);
        u32arr_sort(&right);
        UInt32Array merged = u32arr_intersect(&left, &right);
        u32arr_free(&left);
        u32arr_free(&right);
        left = merged;
    }
    return left;
}

static UInt32Array parse_or(Parser* p) {
    UInt32Array left = parse_and(p);
    u32arr_sort(&left);

    while (peek_or(p)) {
        p->pos += 2;
        UInt32Array right = parse_and(p);
        u32arr_sort(&right);
        UInt32Array merged = u32arr_unite(&left, &right);
        u32arr_free(&left);
        u32arr_free(&right);
        left = merged;
    }
    return left;
}

static UInt32Array parse_expr(Parser* p) {
    return parse_or(p);
}

UInt32Array execute_query(const char* query, HashMap* index, uint32_t num_docs) {
    Parser p;
    p.input = query;
    p.pos = 0;
    p.len = 0;
    while (query[p.len]) p.len++;
    p.index = index;
    p.num_docs = num_docs;

    UInt32Array result = parse_expr(&p);
    u32arr_sort(&result);
    return result;
}
