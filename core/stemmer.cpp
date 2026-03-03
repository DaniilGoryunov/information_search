#include "stemmer.h"
#include <cstring>

static bool is_consonant(const char* s, int i) {
    switch (s[i]) {
        case 'a': case 'e': case 'i': case 'o': case 'u': return false;
        case 'y': return (i == 0) ? true : !is_consonant(s, i - 1);
        default: return true;
    }
}

static int measure(const char* s, int len) {
    int n = 0;
    int i = 0;
    while (i < len && !is_consonant(s, i)) i++;
    if (i >= len) return 0;
    while (i < len) {
        while (i < len && is_consonant(s, i)) i++;
        if (i >= len) break;
        n++;
        while (i < len && !is_consonant(s, i)) i++;
    }
    return n;
}

static bool has_vowel(const char* s, int len) {
    for (int i = 0; i < len; i++)
        if (!is_consonant(s, i)) return true;
    return false;
}

static bool ends_double_consonant(const char* s, int len) {
    if (len < 2) return false;
    return s[len - 1] == s[len - 2] && is_consonant(s, len - 1);
}

static bool cvc(const char* s, int len) {
    if (len < 3) return false;
    if (!is_consonant(s, len - 1) || is_consonant(s, len - 2) || !is_consonant(s, len - 3))
        return false;
    char c = s[len - 1];
    if (c == 'w' || c == 'x' || c == 'y') return false;
    return true;
}

static bool ends_with(const char* s, int len, const char* suffix, int suf_len) {
    if (suf_len > len) return false;
    return memcmp(s + len - suf_len, suffix, suf_len) == 0;
}

static void set_end(char* s, int* len, int suf_len, const char* repl) {
    int rlen = 0;
    while (repl[rlen]) rlen++;
    *len = *len - suf_len + rlen;
    memcpy(s + *len - rlen, repl, rlen);
    s[*len] = '\0';
}

static void step1a(char* s, int* len) {
    if (ends_with(s, *len, "sses", 4)) { *len -= 2; s[*len] = '\0'; }
    else if (ends_with(s, *len, "ies", 3)) { *len -= 2; s[*len] = '\0'; }
    else if (!ends_with(s, *len, "ss", 2) && ends_with(s, *len, "s", 1)) { *len -= 1; s[*len] = '\0'; }
}

static void step1b(char* s, int* len) {
    bool flag = false;
    if (ends_with(s, *len, "eed", 3)) {
        if (measure(s, *len - 3) > 0) { *len -= 1; s[*len] = '\0'; }
    } else if (ends_with(s, *len, "ed", 2) && has_vowel(s, *len - 2)) {
        *len -= 2; s[*len] = '\0'; flag = true;
    } else if (ends_with(s, *len, "ing", 3) && has_vowel(s, *len - 3)) {
        *len -= 3; s[*len] = '\0'; flag = true;
    }
    if (flag) {
        if (ends_with(s, *len, "at", 2) || ends_with(s, *len, "bl", 2) || ends_with(s, *len, "iz", 2)) {
            s[*len] = 'e'; (*len)++; s[*len] = '\0';
        } else if (ends_double_consonant(s, *len) &&
                   s[*len - 1] != 'l' && s[*len - 1] != 's' && s[*len - 1] != 'z') {
            (*len)--; s[*len] = '\0';
        } else if (measure(s, *len) == 1 && cvc(s, *len)) {
            s[*len] = 'e'; (*len)++; s[*len] = '\0';
        }
    }
}

static void step1c(char* s, int* len) {
    if (s[*len - 1] == 'y' && has_vowel(s, *len - 1)) {
        s[*len - 1] = 'i';
    }
}

static void step2(char* s, int* len) {
    struct Rule { const char* suf; int slen; const char* repl; };
    static const Rule rules[] = {
        {"ational", 7, "ate"}, {"tional", 6, "tion"}, {"enci", 4, "ence"},
        {"anci", 4, "ance"}, {"izer", 4, "ize"}, {"abli", 4, "able"},
        {"alli", 4, "al"}, {"entli", 5, "ent"}, {"eli", 3, "e"},
        {"ousli", 5, "ous"}, {"ization", 7, "ize"}, {"ation", 5, "ate"},
        {"ator", 4, "ate"}, {"alism", 5, "al"}, {"iveness", 7, "ive"},
        {"fulness", 7, "ful"}, {"ousness", 7, "ous"}, {"aliti", 5, "al"},
        {"iviti", 5, "ive"}, {"biliti", 6, "ble"},
        {nullptr, 0, nullptr}
    };
    for (int i = 0; rules[i].suf; i++) {
        if (ends_with(s, *len, rules[i].suf, rules[i].slen)) {
            if (measure(s, *len - rules[i].slen) > 0)
                set_end(s, len, rules[i].slen, rules[i].repl);
            return;
        }
    }
}

static void step3(char* s, int* len) {
    struct Rule { const char* suf; int slen; const char* repl; };
    static const Rule rules[] = {
        {"icate", 5, "ic"}, {"ative", 5, ""}, {"alize", 5, "al"},
        {"iciti", 5, "ic"}, {"ical", 4, "ic"}, {"ful", 3, ""},
        {"ness", 4, ""},
        {nullptr, 0, nullptr}
    };
    for (int i = 0; rules[i].suf; i++) {
        if (ends_with(s, *len, rules[i].suf, rules[i].slen)) {
            if (measure(s, *len - rules[i].slen) > 0)
                set_end(s, len, rules[i].slen, rules[i].repl);
            return;
        }
    }
}

static void step4(char* s, int* len) {
    const char* suffixes[] = {
        "al", "ance", "ence", "er", "ic", "able", "ible", "ant",
        "ement", "ment", "ent", "ion", "ou", "ism", "ate", "iti",
        "ous", "ive", "ize", nullptr
    };
    int slens[] = {2,4,4,2,2,4,4,3,5,4,3,3,2,3,3,3,3,3,3};
    for (int i = 0; suffixes[i]; i++) {
        if (ends_with(s, *len, suffixes[i], slens[i])) {
            int stem_len = *len - slens[i];
            if (i == 11) {
                if (stem_len > 0 && (s[stem_len - 1] == 's' || s[stem_len - 1] == 't') &&
                    measure(s, stem_len) > 1) {
                    *len = stem_len; s[*len] = '\0';
                }
            } else {
                if (measure(s, stem_len) > 1) {
                    *len = stem_len; s[*len] = '\0';
                }
            }
            return;
        }
    }
}

static void step5a(char* s, int* len) {
    if (s[*len - 1] == 'e') {
        int m = measure(s, *len - 1);
        if (m > 1 || (m == 1 && !cvc(s, *len - 1))) {
            (*len)--; s[*len] = '\0';
        }
    }
}

static void step5b(char* s, int* len) {
    if (measure(s, *len) > 1 && ends_double_consonant(s, *len) && s[*len - 1] == 'l') {
        (*len)--; s[*len] = '\0';
    }
}

void porter_stem(MyString* word) {
    if (word->len <= 2) return;

    char buf[256];
    int len = (int)word->len;
    if (len > 255) len = 255;
    memcpy(buf, word->data, len);
    buf[len] = '\0';

    step1a(buf, &len);
    step1b(buf, &len);
    step1c(buf, &len);
    step2(buf, &len);
    step3(buf, &len);
    step4(buf, &len);
    step5a(buf, &len);
    step5b(buf, &len);

    mystr_free(word);
    mystr_init_buf(word, buf, len);
}
