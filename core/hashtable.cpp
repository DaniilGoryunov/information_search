#include "hashtable.h"
#include <cstdlib>
#include <cstring>

uint32_t fnv1a_hash(const char* data, size_t len) {
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < len; i++) {
        hash ^= (uint8_t)data[i];
        hash *= 16777619u;
    }
    return hash;
}

void hashmap_init(HashMap* m, size_t initial_cap) {
    m->capacity = initial_cap;
    m->size = 0;
    m->buckets = (HashEntry*)calloc(initial_cap, sizeof(HashEntry));
    for (size_t i = 0; i < initial_cap; i++) {
        m->buckets[i].occupied = false;
        mystr_init(&m->buckets[i].key);
        u32arr_init(&m->buckets[i].posting_list);
    }
}

void hashmap_free(HashMap* m) {
    for (size_t i = 0; i < m->capacity; i++) {
        if (m->buckets[i].occupied) {
            mystr_free(&m->buckets[i].key);
            u32arr_free(&m->buckets[i].posting_list);
        }
    }
    free(m->buckets);
    m->buckets = nullptr;
    m->capacity = 0;
    m->size = 0;
}

static void hashmap_resize(HashMap* m) {
    size_t old_cap = m->capacity;
    HashEntry* old_buckets = m->buckets;

    size_t new_cap = old_cap * 2;
    m->buckets = (HashEntry*)calloc(new_cap, sizeof(HashEntry));
    m->capacity = new_cap;
    m->size = 0;

    for (size_t i = 0; i < new_cap; i++) {
        m->buckets[i].occupied = false;
        mystr_init(&m->buckets[i].key);
        u32arr_init(&m->buckets[i].posting_list);
    }

    for (size_t i = 0; i < old_cap; i++) {
        if (old_buckets[i].occupied) {
            UInt32Array* slot = hashmap_get_or_insert(m,
                old_buckets[i].key.data, old_buckets[i].key.len);
            u32arr_free(slot);
            *slot = old_buckets[i].posting_list;
            mystr_free(&old_buckets[i].key);
        }
    }
    free(old_buckets);
}

static size_t find_slot(HashMap* m, const char* key, size_t key_len) {
    uint32_t h = fnv1a_hash(key, key_len);
    size_t idx = h % m->capacity;
    while (m->buckets[idx].occupied) {
        if (m->buckets[idx].key.len == key_len &&
            memcmp(m->buckets[idx].key.data, key, key_len) == 0) {
            return idx;
        }
        idx = (idx + 1) % m->capacity;
    }
    return idx;
}

UInt32Array* hashmap_get(HashMap* m, const char* key, size_t key_len) {
    size_t idx = find_slot(m, key, key_len);
    if (m->buckets[idx].occupied) return &m->buckets[idx].posting_list;
    return nullptr;
}

UInt32Array* hashmap_get_or_insert(HashMap* m, const char* key, size_t key_len) {
    if (m->size * 10 >= m->capacity * 7) {
        hashmap_resize(m);
    }
    size_t idx = find_slot(m, key, key_len);
    if (!m->buckets[idx].occupied) {
        mystr_init_buf(&m->buckets[idx].key, key, key_len);
        u32arr_init(&m->buckets[idx].posting_list);
        m->buckets[idx].occupied = true;
        m->size++;
    }
    return &m->buckets[idx].posting_list;
}

bool hashmap_contains(HashMap* m, const char* key, size_t key_len) {
    size_t idx = find_slot(m, key, key_len);
    return m->buckets[idx].occupied;
}

void hashiter_init(HashIterator* it, HashMap* m) {
    it->map = m;
    it->index = 0;
}

bool hashiter_next(HashIterator* it, MyString** key, UInt32Array** val) {
    while (it->index < it->map->capacity) {
        if (it->map->buckets[it->index].occupied) {
            *key = &it->map->buckets[it->index].key;
            *val = &it->map->buckets[it->index].posting_list;
            it->index++;
            return true;
        }
        it->index++;
    }
    return false;
}
