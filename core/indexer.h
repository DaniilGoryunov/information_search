#ifndef INDEXER_H
#define INDEXER_H

#include "hashtable.h"
#include "mystring.h"
#include "dynarray.h"
#include <cstdint>
#include <cstdio>

struct ForwardEntry {
    MyString url;
    MyString title;
};

struct ForwardIndex {
    ForwardEntry* docs;
    uint32_t count;
    uint32_t cap;
};

void fwd_init(ForwardIndex* fi);
void fwd_free(ForwardIndex* fi);
uint32_t fwd_add(ForwardIndex* fi, const char* url, const char* title);

void write_inverted_index(const char* path, HashMap* index, uint32_t num_docs);
void write_forward_index(const char* path, ForwardIndex* fi);

void read_inverted_index(const char* path, HashMap* index, uint32_t* num_docs);
void read_forward_index(const char* path, ForwardIndex* fi);

#endif
