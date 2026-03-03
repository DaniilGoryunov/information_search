#ifndef DYNARRAY_H
#define DYNARRAY_H

#include <cstddef>
#include <cstdlib>
#include <cstring>

struct IntArray {
    int* data;
    size_t len;
    size_t cap;
};

void intarr_init(IntArray* a);
void intarr_free(IntArray* a);
void intarr_push(IntArray* a, int val);
void intarr_clear(IntArray* a);
void intarr_copy(IntArray* dst, const IntArray* src);
bool intarr_contains(const IntArray* a, int val);
void intarr_sort(IntArray* a);

IntArray intarr_intersect(const IntArray* a, const IntArray* b);
IntArray intarr_unite(const IntArray* a, const IntArray* b);
IntArray intarr_subtract(const IntArray* all_docs, const IntArray* b);

struct UInt32Array {
    uint32_t* data;
    size_t len;
    size_t cap;
};

void u32arr_init(UInt32Array* a);
void u32arr_free(UInt32Array* a);
void u32arr_push(UInt32Array* a, uint32_t val);
void u32arr_clear(UInt32Array* a);
void u32arr_sort(UInt32Array* a);

UInt32Array u32arr_intersect(const UInt32Array* a, const UInt32Array* b);
UInt32Array u32arr_unite(const UInt32Array* a, const UInt32Array* b);
UInt32Array u32arr_subtract_from_range(uint32_t total, const UInt32Array* b);

#endif
