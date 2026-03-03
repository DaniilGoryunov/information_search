#ifndef SEARCHER_H
#define SEARCHER_H

#include "hashtable.h"
#include "dynarray.h"
#include "indexer.h"

UInt32Array execute_query(const char* query, HashMap* index, uint32_t num_docs);

#endif
