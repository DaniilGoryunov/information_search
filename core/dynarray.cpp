#include "dynarray.h"
#include <cstdint>

static void swap_u32(uint32_t* a, uint32_t* b) {
    uint32_t t = *a; *a = *b; *b = t;
}

static void qsort_u32(uint32_t* arr, int lo, int hi) {
    if (lo >= hi) return;
    uint32_t pivot = arr[(lo + hi) / 2];
    int i = lo, j = hi;
    while (i <= j) {
        while (arr[i] < pivot) i++;
        while (arr[j] > pivot) j--;
        if (i <= j) { swap_u32(&arr[i], &arr[j]); i++; j--; }
    }
    qsort_u32(arr, lo, j);
    qsort_u32(arr, i, hi);
}

void intarr_init(IntArray* a) {
    a->data = nullptr; a->len = 0; a->cap = 0;
}
void intarr_free(IntArray* a) {
    free(a->data); a->data = nullptr; a->len = 0; a->cap = 0;
}
void intarr_push(IntArray* a, int val) {
    if (a->len >= a->cap) {
        a->cap = a->cap == 0 ? 16 : a->cap * 2;
        a->data = (int*)realloc(a->data, a->cap * sizeof(int));
    }
    a->data[a->len++] = val;
}
void intarr_clear(IntArray* a) { a->len = 0; }

void intarr_copy(IntArray* dst, const IntArray* src) {
    dst->len = src->len;
    dst->cap = src->len;
    dst->data = (int*)malloc(dst->cap * sizeof(int));
    memcpy(dst->data, src->data, src->len * sizeof(int));
}

bool intarr_contains(const IntArray* a, int val) {
    for (size_t i = 0; i < a->len; i++)
        if (a->data[i] == val) return true;
    return false;
}

static void swap_int(int* a, int* b) { int t = *a; *a = *b; *b = t; }

static void qsort_int(int* arr, int lo, int hi) {
    if (lo >= hi) return;
    int pivot = arr[(lo + hi) / 2];
    int i = lo, j = hi;
    while (i <= j) {
        while (arr[i] < pivot) i++;
        while (arr[j] > pivot) j--;
        if (i <= j) { swap_int(&arr[i], &arr[j]); i++; j--; }
    }
    qsort_int(arr, lo, j);
    qsort_int(arr, i, hi);
}

void intarr_sort(IntArray* a) {
    if (a->len > 1) qsort_int(a->data, 0, (int)a->len - 1);
}

IntArray intarr_intersect(const IntArray* a, const IntArray* b) {
    IntArray r; intarr_init(&r);
    size_t i = 0, j = 0;
    while (i < a->len && j < b->len) {
        if (a->data[i] == b->data[j]) { intarr_push(&r, a->data[i]); i++; j++; }
        else if (a->data[i] < b->data[j]) i++;
        else j++;
    }
    return r;
}

IntArray intarr_unite(const IntArray* a, const IntArray* b) {
    IntArray r; intarr_init(&r);
    size_t i = 0, j = 0;
    while (i < a->len && j < b->len) {
        if (a->data[i] == b->data[j]) { intarr_push(&r, a->data[i]); i++; j++; }
        else if (a->data[i] < b->data[j]) { intarr_push(&r, a->data[i]); i++; }
        else { intarr_push(&r, b->data[j]); j++; }
    }
    while (i < a->len) intarr_push(&r, a->data[i++]);
    while (j < b->len) intarr_push(&r, b->data[j++]);
    return r;
}

IntArray intarr_subtract(const IntArray* all_docs, const IntArray* b) {
    IntArray r; intarr_init(&r);
    size_t i = 0, j = 0;
    while (i < all_docs->len && j < b->len) {
        if (all_docs->data[i] == b->data[j]) { i++; j++; }
        else if (all_docs->data[i] < b->data[j]) { intarr_push(&r, all_docs->data[i]); i++; }
        else j++;
    }
    while (i < all_docs->len) intarr_push(&r, all_docs->data[i++]);
    return r;
}

void u32arr_init(UInt32Array* a) {
    a->data = nullptr; a->len = 0; a->cap = 0;
}
void u32arr_free(UInt32Array* a) {
    free(a->data); a->data = nullptr; a->len = 0; a->cap = 0;
}
void u32arr_push(UInt32Array* a, uint32_t val) {
    if (a->len >= a->cap) {
        a->cap = a->cap == 0 ? 16 : a->cap * 2;
        a->data = (uint32_t*)realloc(a->data, a->cap * sizeof(uint32_t));
    }
    a->data[a->len++] = val;
}
void u32arr_clear(UInt32Array* a) { a->len = 0; }

void u32arr_sort(UInt32Array* a) {
    if (a->len > 1) qsort_u32(a->data, 0, (int)a->len - 1);
}

UInt32Array u32arr_intersect(const UInt32Array* a, const UInt32Array* b) {
    UInt32Array r; u32arr_init(&r);
    size_t i = 0, j = 0;
    while (i < a->len && j < b->len) {
        if (a->data[i] == b->data[j]) { u32arr_push(&r, a->data[i]); i++; j++; }
        else if (a->data[i] < b->data[j]) i++;
        else j++;
    }
    return r;
}

UInt32Array u32arr_unite(const UInt32Array* a, const UInt32Array* b) {
    UInt32Array r; u32arr_init(&r);
    size_t i = 0, j = 0;
    while (i < a->len && j < b->len) {
        if (a->data[i] == b->data[j]) { u32arr_push(&r, a->data[i]); i++; j++; }
        else if (a->data[i] < b->data[j]) { u32arr_push(&r, a->data[i]); i++; }
        else { u32arr_push(&r, b->data[j]); j++; }
    }
    while (i < a->len) u32arr_push(&r, a->data[i++]);
    while (j < b->len) u32arr_push(&r, b->data[j++]);
    return r;
}

UInt32Array u32arr_subtract_from_range(uint32_t total, const UInt32Array* b) {
    UInt32Array r; u32arr_init(&r);
    size_t j = 0;
    for (uint32_t i = 0; i < total; i++) {
        if (j < b->len && b->data[j] == i) { j++; continue; }
        u32arr_push(&r, i);
    }
    return r;
}
