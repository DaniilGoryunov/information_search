#ifndef MYSTRING_H
#define MYSTRING_H

#include <cstddef>
#include <cstdint>

struct MyString {
    char* data;
    size_t len;
    size_t cap;
};

void mystr_init(MyString* s);
void mystr_init_cstr(MyString* s, const char* cstr);
void mystr_init_buf(MyString* s, const char* buf, size_t n);
void mystr_copy(MyString* dst, const MyString* src);
void mystr_free(MyString* s);

void mystr_reserve(MyString* s, size_t new_cap);
void mystr_push(MyString* s, char c);
void mystr_append_cstr(MyString* s, const char* cstr);
void mystr_append_buf(MyString* s, const char* buf, size_t n);
void mystr_clear(MyString* s);

void mystr_tolower(MyString* s);
int  mystr_cmp(const MyString* a, const MyString* b);
int  mystr_cmp_cstr(const MyString* a, const char* cstr);
bool mystr_ends_with(const MyString* s, const char* suffix);
bool mystr_eq(const MyString* a, const MyString* b);

size_t mystr_len(const char* cstr);
void   mystr_copy_cstr(char* dst, const char* src);

#endif
