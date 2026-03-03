#include "indexer.h"
#include <cstdlib>
#include <cstring>

void fwd_init(ForwardIndex* fi) {
    fi->count = 0;
    fi->cap = 1024;
    fi->docs = (ForwardEntry*)malloc(fi->cap * sizeof(ForwardEntry));
}

void fwd_free(ForwardIndex* fi) {
    for (uint32_t i = 0; i < fi->count; i++) {
        mystr_free(&fi->docs[i].url);
        mystr_free(&fi->docs[i].title);
    }
    free(fi->docs);
    fi->docs = nullptr;
    fi->count = 0;
}

uint32_t fwd_add(ForwardIndex* fi, const char* url, const char* title) {
    if (fi->count >= fi->cap) {
        fi->cap *= 2;
        fi->docs = (ForwardEntry*)realloc(fi->docs, fi->cap * sizeof(ForwardEntry));
    }
    uint32_t id = fi->count;
    mystr_init_cstr(&fi->docs[id].url, url);
    mystr_init_cstr(&fi->docs[id].title, title);
    fi->count++;
    return id;
}

void write_inverted_index(const char* path, HashMap* index, uint32_t num_docs) {
    FILE* f = fopen(path, "wb");
    if (!f) return;

    fwrite("BIDX", 1, 4, f);
    uint16_t version = 1;
    fwrite(&version, 2, 1, f);
    uint32_t num_terms = (uint32_t)index->size;
    fwrite(&num_terms, 4, 1, f);
    fwrite(&num_docs, 4, 1, f);

    HashIterator it;
    hashiter_init(&it, index);
    MyString* key;
    UInt32Array* val;
    while (hashiter_next(&it, &key, &val)) {
        uint16_t tlen = (uint16_t)key->len;
        fwrite(&tlen, 2, 1, f);
        fwrite(key->data, 1, tlen, f);
        uint32_t df = (uint32_t)val->len;
        fwrite(&df, 4, 1, f);
        for (size_t i = 0; i < val->len; i++) {
            fwrite(&val->data[i], 4, 1, f);
        }
    }
    fclose(f);
}

void write_forward_index(const char* path, ForwardIndex* fi) {
    FILE* f = fopen(path, "wb");
    if (!f) return;

    fwrite("FIDX", 1, 4, f);
    fwrite(&fi->count, 4, 1, f);

    for (uint32_t i = 0; i < fi->count; i++) {
        uint16_t ulen = (uint16_t)fi->docs[i].url.len;
        fwrite(&ulen, 2, 1, f);
        fwrite(fi->docs[i].url.data, 1, ulen, f);
        uint16_t tlen = (uint16_t)fi->docs[i].title.len;
        fwrite(&tlen, 2, 1, f);
        fwrite(fi->docs[i].title.data, 1, tlen, f);
    }
    fclose(f);
}

void read_inverted_index(const char* path, HashMap* index, uint32_t* num_docs) {
    FILE* f = fopen(path, "rb");
    if (!f) return;

    char magic[4];
    fread(magic, 1, 4, f);
    uint16_t version;
    fread(&version, 2, 1, f);
    uint32_t num_terms;
    fread(&num_terms, 4, 1, f);
    fread(num_docs, 4, 1, f);

    hashmap_init(index, num_terms * 2 + 1);

    char buf[4096];
    for (uint32_t t = 0; t < num_terms; t++) {
        uint16_t tlen;
        fread(&tlen, 2, 1, f);
        fread(buf, 1, tlen, f);
        buf[tlen] = '\0';

        uint32_t df;
        fread(&df, 4, 1, f);

        UInt32Array* pl = hashmap_get_or_insert(index, buf, tlen);
        for (uint32_t d = 0; d < df; d++) {
            uint32_t doc_id;
            fread(&doc_id, 4, 1, f);
            u32arr_push(pl, doc_id);
        }
    }
    fclose(f);
}

void read_forward_index(const char* path, ForwardIndex* fi) {
    FILE* f = fopen(path, "rb");
    if (!f) return;

    char magic[4];
    fread(magic, 1, 4, f);
    fread(&fi->count, 4, 1, f);
    fi->cap = fi->count;
    fi->docs = (ForwardEntry*)malloc(fi->cap * sizeof(ForwardEntry));

    char buf[65536];
    for (uint32_t i = 0; i < fi->count; i++) {
        uint16_t ulen;
        fread(&ulen, 2, 1, f);
        fread(buf, 1, ulen, f);
        buf[ulen] = '\0';
        mystr_init_cstr(&fi->docs[i].url, buf);

        uint16_t tlen;
        fread(&tlen, 2, 1, f);
        fread(buf, 1, tlen, f);
        buf[tlen] = '\0';
        mystr_init_cstr(&fi->docs[i].title, buf);
    }
    fclose(f);
}
