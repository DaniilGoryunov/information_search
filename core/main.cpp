#include "tokenizer.h"
#include "stemmer.h"
#include "indexer.h"
#include "searcher.h"
#include "hashtable.h"
#include "dynarray.h"
#include "mystring.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>
#include <dirent.h>
#include <sys/stat.h>

static void print_usage() {
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  search_engine tokenize <input_file>\n");
    fprintf(stderr, "  search_engine stem <word>\n");
    fprintf(stderr, "  search_engine build <texts_dir> <index.bin> <forward.bin>\n");
    fprintf(stderr, "  search_engine search <index.bin> <forward.bin>\n");
    fprintf(stderr, "  search_engine stats <texts_dir>\n");
    fprintf(stderr, "  search_engine freq <texts_dir>\n");
}

static std::string read_file(const char* path) {
    FILE* f = fopen(path, "r");
    if (!f) return "";
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    std::string buf(sz, '\0');
    fread(&buf[0], 1, sz, f);
    fclose(f);
    return buf;
}

static void cmd_tokenize(const char* path) {
    std::string text = read_file(path);
    clock_t start = clock();
    std::vector<std::string> tokens = tokenize(text);
    clock_t end = clock();

    for (auto& t : tokens) printf("%s\n", t.c_str());

    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
    long total_len = 0;
    for (auto& t : tokens) total_len += (long)t.size();
    double avg_len = tokens.empty() ? 0 : (double)total_len / tokens.size();
    double kb = (double)text.size() / 1024.0;
    double speed = elapsed > 0 ? kb / elapsed : 0;

    fprintf(stderr, "Tokens: %zu\n", tokens.size());
    fprintf(stderr, "Average token length: %.2f\n", avg_len);
    fprintf(stderr, "Time: %.4f s\n", elapsed);
    fprintf(stderr, "Speed: %.1f KB/s\n", speed);
}

static void cmd_stem(const char* word) {
    MyString s;
    mystr_init_cstr(&s, word);
    mystr_tolower(&s);
    porter_stem(&s);
    printf("%s\n", s.data);
    mystr_free(&s);
}

struct FileList {
    char** paths;
    char** names;
    int count;
    int cap;
};

static void filelist_init(FileList* fl) {
    fl->count = 0; fl->cap = 4096;
    fl->paths = (char**)malloc(fl->cap * sizeof(char*));
    fl->names = (char**)malloc(fl->cap * sizeof(char*));
}

static void filelist_add(FileList* fl, const char* path, const char* name) {
    if (fl->count >= fl->cap) {
        fl->cap *= 2;
        fl->paths = (char**)realloc(fl->paths, fl->cap * sizeof(char*));
        fl->names = (char**)realloc(fl->names, fl->cap * sizeof(char*));
    }
    fl->paths[fl->count] = strdup(path);
    fl->names[fl->count] = strdup(name);
    fl->count++;
}

static void filelist_free(FileList* fl) {
    for (int i = 0; i < fl->count; i++) { free(fl->paths[i]); free(fl->names[i]); }
    free(fl->paths); free(fl->names);
}

static void list_txt_files(const char* dir, FileList* fl) {
    DIR* d = opendir(dir);
    if (!d) return;
    struct dirent* ent;
    while ((ent = readdir(d)) != nullptr) {
        size_t nlen = strlen(ent->d_name);
        if (nlen > 4 && strcmp(ent->d_name + nlen - 4, ".txt") == 0) {
            char fullpath[4096];
            snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, ent->d_name);
            filelist_add(fl, fullpath, ent->d_name);
        }
    }
    closedir(d);
}

static void cmd_build(const char* texts_dir, const char* idx_path, const char* fwd_path) {
    FileList fl;
    filelist_init(&fl);
    list_txt_files(texts_dir, &fl);

    fprintf(stderr, "Building index from %d documents...\n", fl.count);
    clock_t start = clock();

    HashMap index;
    hashmap_init(&index, 131072);
    ForwardIndex fwd;
    fwd_init(&fwd);

    long total_bytes = 0;
    for (int i = 0; i < fl.count; i++) {
        std::string text = read_file(fl.paths[i]);
        total_bytes += (long)text.size();
        std::vector<std::string> tokens = tokenize(text);

        char title[512];
        strncpy(title, fl.names[i], 507);
        title[507] = '\0';
        size_t tlen = strlen(title);
        if (tlen > 4) title[tlen - 4] = '\0';

        char url[1024];
        snprintf(url, sizeof(url), "doc://%s", fl.names[i]);

        uint32_t doc_id = fwd_add(&fwd, url, title);

        for (auto& tok : tokens) {
            MyString ms;
            mystr_init_cstr(&ms, tok.c_str());
            porter_stem(&ms);
            UInt32Array* pl = hashmap_get_or_insert(&index, ms.data, ms.len);
            if (pl->len == 0 || pl->data[pl->len - 1] != doc_id) {
                u32arr_push(pl, doc_id);
            }
            mystr_free(&ms);
        }

        if ((i + 1) % 5000 == 0) {
            fprintf(stderr, "  Indexed %d/%d documents\n", i + 1, fl.count);
        }
    }

    clock_t end = clock();
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;

    write_inverted_index(idx_path, &index, fwd.count);
    write_forward_index(fwd_path, &fwd);

    fprintf(stderr, "Done. %d docs, %zu terms\n", fl.count, index.size);
    fprintf(stderr, "Total text: %ld bytes\n", total_bytes);
    fprintf(stderr, "Time: %.2f s\n", elapsed);
    fprintf(stderr, "Speed: %.1f docs/s, %.1f KB/s\n",
            fl.count / elapsed, (total_bytes / 1024.0) / elapsed);

    hashmap_free(&index);
    fwd_free(&fwd);
    filelist_free(&fl);
}

static void cmd_search(const char* idx_path, const char* fwd_path) {
    HashMap index;
    uint32_t num_docs = 0;
    ForwardIndex fwd;
    fwd_init(&fwd);

    fprintf(stderr, "Loading index...\n");
    read_inverted_index(idx_path, &index, &num_docs);
    read_forward_index(fwd_path, &fwd);
    fprintf(stderr, "Loaded: %u docs, %zu terms\n", num_docs, index.size);

    char line[4096];
    while (fgets(line, sizeof(line), stdin)) {
        size_t ln = strlen(line);
        while (ln > 0 && (line[ln - 1] == '\n' || line[ln - 1] == '\r')) line[--ln] = '\0';
        if (ln == 0) continue;

        clock_t start = clock();
        UInt32Array result = execute_query(line, &index, num_docs);
        clock_t end = clock();
        double ms = (double)(end - start) / CLOCKS_PER_SEC * 1000.0;

        printf("QUERY: %s\n", line);
        printf("RESULTS: %zu (%.2f ms)\n", result.len, ms);
        size_t show = result.len < 50 ? result.len : 50;
        for (size_t i = 0; i < show; i++) {
            uint32_t id = result.data[i];
            if (id < fwd.count) {
                printf("  %u\t%s\t%s\n", id, fwd.docs[id].title.data, fwd.docs[id].url.data);
            }
        }
        printf("---\n");
        u32arr_free(&result);
    }

    hashmap_free(&index);
    fwd_free(&fwd);
}

static void cmd_stats(const char* texts_dir) {
    FileList fl;
    filelist_init(&fl);
    list_txt_files(texts_dir, &fl);

    long total_bytes = 0;
    long total_tokens = 0;
    long total_token_len = 0;
    for (int i = 0; i < fl.count; i++) {
        std::string text = read_file(fl.paths[i]);
        total_bytes += (long)text.size();
        std::vector<std::string> tokens = tokenize(text);
        total_tokens += (long)tokens.size();
        for (auto& t : tokens) total_token_len += (long)t.size();
    }

    printf("Documents: %d\n", fl.count);
    printf("Total text size: %ld bytes (%.2f MB)\n", total_bytes, total_bytes / 1048576.0);
    printf("Average document size: %ld bytes\n", fl.count > 0 ? total_bytes / fl.count : 0);
    printf("Total tokens: %ld\n", total_tokens);
    printf("Average token length: %.2f\n", total_tokens > 0 ? (double)total_token_len / total_tokens : 0);
    printf("Average tokens per document: %ld\n", fl.count > 0 ? total_tokens / fl.count : 0);

    filelist_free(&fl);
}

static void cmd_freq(const char* texts_dir) {
    FileList fl;
    filelist_init(&fl);
    list_txt_files(texts_dir, &fl);

    HashMap freq_map;
    hashmap_init(&freq_map, 131072);

    for (int i = 0; i < fl.count; i++) {
        std::string text = read_file(fl.paths[i]);
        std::vector<std::string> tokens = tokenize(text);
        for (auto& tok : tokens) {
            MyString ms;
            mystr_init_cstr(&ms, tok.c_str());
            porter_stem(&ms);
            UInt32Array* cnt = hashmap_get_or_insert(&freq_map, ms.data, ms.len);
            u32arr_push(cnt, 1);
            mystr_free(&ms);
        }
    }

    HashIterator it;
    hashiter_init(&it, &freq_map);
    MyString* key;
    UInt32Array* val;
    while (hashiter_next(&it, &key, &val)) {
        printf("%s\t%zu\n", key->data, val->len);
    }

    hashmap_free(&freq_map);
    filelist_free(&fl);
}

int main(int argc, char* argv[]) {
    if (argc < 2) { print_usage(); return 1; }

    if (strcmp(argv[1], "tokenize") == 0 && argc >= 3) {
        cmd_tokenize(argv[2]);
    } else if (strcmp(argv[1], "stem") == 0 && argc >= 3) {
        cmd_stem(argv[2]);
    } else if (strcmp(argv[1], "build") == 0 && argc >= 5) {
        cmd_build(argv[2], argv[3], argv[4]);
    } else if (strcmp(argv[1], "search") == 0 && argc >= 4) {
        cmd_search(argv[2], argv[3]);
    } else if (strcmp(argv[1], "stats") == 0 && argc >= 3) {
        cmd_stats(argv[2]);
    } else if (strcmp(argv[1], "freq") == 0 && argc >= 3) {
        cmd_freq(argv[2]);
    } else {
        print_usage();
        return 1;
    }
    return 0;
}
