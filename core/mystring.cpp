#include "mystring.h"
#include <cstdlib>
#include <cstring>

void mystr_init(MyString* s) {
    s->data = nullptr;
    s->len = 0;
    s->cap = 0;
}

void mystr_init_cstr(MyString* s, const char* cstr) {
    s->len = mystr_len(cstr);
    s->cap = s->len + 1;
    s->data = (char*)malloc(s->cap);
    memcpy(s->data, cstr, s->len);
    s->data[s->len] = '\0';
}

void mystr_init_buf(MyString* s, const char* buf, size_t n) {
    s->len = n;
    s->cap = n + 1;
    s->data = (char*)malloc(s->cap);
    memcpy(s->data, buf, n);
    s->data[n] = '\0';
}

void mystr_copy(MyString* dst, const MyString* src) {
    dst->len = src->len;
    dst->cap = src->len + 1;
    dst->data = (char*)malloc(dst->cap);
    memcpy(dst->data, src->data, src->len);
    dst->data[dst->len] = '\0';
}

void mystr_free(MyString* s) {
    free(s->data);
    s->data = nullptr;
    s->len = 0;
    s->cap = 0;
}

void mystr_reserve(MyString* s, size_t new_cap) {
    if (new_cap <= s->cap) return;
    s->data = (char*)realloc(s->data, new_cap);
    s->cap = new_cap;
}

void mystr_push(MyString* s, char c) {
    if (s->len + 1 >= s->cap) {
        size_t nc = (s->cap == 0) ? 16 : s->cap * 2;
        mystr_reserve(s, nc);
    }
    s->data[s->len++] = c;
    s->data[s->len] = '\0';
}

void mystr_append_cstr(MyString* s, const char* cstr) {
    size_t clen = mystr_len(cstr);
    mystr_append_buf(s, cstr, clen);
}

void mystr_append_buf(MyString* s, const char* buf, size_t n) {
    if (s->len + n + 1 > s->cap) {
        size_t nc = s->cap == 0 ? 16 : s->cap;
        while (nc < s->len + n + 1) nc *= 2;
        mystr_reserve(s, nc);
    }
    memcpy(s->data + s->len, buf, n);
    s->len += n;
    s->data[s->len] = '\0';
}

void mystr_clear(MyString* s) {
    s->len = 0;
    if (s->data) s->data[0] = '\0';
}

void mystr_tolower(MyString* s) {
    for (size_t i = 0; i < s->len; i++) {
        if (s->data[i] >= 'A' && s->data[i] <= 'Z') {
            s->data[i] = s->data[i] - 'A' + 'a';
        }
    }
}

int mystr_cmp(const MyString* a, const MyString* b) {
    size_t min_len = a->len < b->len ? a->len : b->len;
    for (size_t i = 0; i < min_len; i++) {
        if (a->data[i] != b->data[i])
            return (unsigned char)a->data[i] - (unsigned char)b->data[i];
    }
    if (a->len < b->len) return -1;
    if (a->len > b->len) return 1;
    return 0;
}

int mystr_cmp_cstr(const MyString* a, const char* cstr) {
    MyString tmp;
    mystr_init_cstr(&tmp, cstr);
    int r = mystr_cmp(a, &tmp);
    mystr_free(&tmp);
    return r;
}

bool mystr_ends_with(const MyString* s, const char* suffix) {
    size_t suf_len = mystr_len(suffix);
    if (suf_len > s->len) return false;
    for (size_t i = 0; i < suf_len; i++) {
        if (s->data[s->len - suf_len + i] != suffix[i]) return false;
    }
    return true;
}

bool mystr_eq(const MyString* a, const MyString* b) {
    return mystr_cmp(a, b) == 0;
}

size_t mystr_len(const char* cstr) {
    size_t n = 0;
    while (cstr[n]) n++;
    return n;
}

void mystr_copy_cstr(char* dst, const char* src) {
    while (*src) *dst++ = *src++;
    *dst = '\0';
}
