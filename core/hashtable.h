#ifndef HASHTABLE_H
#define HASHTABLE_H

#include "mystring.h"
#include "dynarray.h"
#include <cstdint>

uint32_t fnv1a_hash(const char* data, size_t len);

struct HashEntry {
    MyString key;
    UInt32Array posting_list;
    bool occupied;
};

struct HashMap {
    HashEntry* buckets;
    size_t capacity;
    size_t size;
};

void hashmap_init(HashMap* m, size_t initial_cap);
void hashmap_free(HashMap* m);

UInt32Array* hashmap_get(HashMap* m, const char* key, size_t key_len);
UInt32Array* hashmap_get_or_insert(HashMap* m, const char* key, size_t key_len);
bool hashmap_contains(HashMap* m, const char* key, size_t key_len);

struct HashIterator {
    HashMap* map;
    size_t index;
};

void hashiter_init(HashIterator* it, HashMap* m);
bool hashiter_next(HashIterator* it, MyString** key, UInt32Array** val);

#endif
