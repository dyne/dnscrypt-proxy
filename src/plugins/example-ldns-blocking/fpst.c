
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct Leaf {
    const char *key;
    uint32_t    val0;
    uint32_t    val1;
} Leaf;

typedef struct Branch {
    union FPST *twigs;
    uint32_t    bitmap;
    uint32_t    index: 28, flags : 4;
} Branch;

typedef union FPST {
    struct Leaf   leaf;
    struct Branch branch;
} FPST;

#define FPST_DEFINED 1
#include "fpst.h"

#ifdef __GNUC__
# define clz_u8(X)       (__builtin_clz(X) + 8 - sizeof(unsigned int) * 8)
# define popcount_u32(X) ((unsigned int) __builtin_popcount(X))
# define prefetch(X)     __builtin_prefetch(X)
#else
# define prefetch(X)     (void)(X)

static inline unsigned int
clz_u8(unsigned int x)
{
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x -= (x >> 1) & 0x55;
    x = ((x >> 2) & 0x33) + (x & 0x33);
    x = ((x >> 4) + x) & 0x0f;

    return 8 - x;
}

static inline unsigned int
popcount_u32(uint32_t w)
{
    w -= (w >> 1) & 0x55555555;
    w = (w & 0x33333333) + ((w >> 2) & 0x33333333);
    w = (w + (w >> 4)) & 0x0f0f0f0f;
    w = (w * 0x01010101) >> 24;
    return w;
}
#endif

static inline int
is_branch(FPST *t)
{
    return t->branch.flags & 8;
}

static inline uint32_t
nib_bit(unsigned int k, unsigned int flags)
{
    unsigned int shift = 16 - 5 - (flags & 7);

    return 1U << ((k >> shift) & 0x1fU);
}

static inline uint32_t
twig_bit(FPST *t, const char *key, size_t len)
{
    uint64_t     i = t->branch.index;
    unsigned int k;

    if (i >= len) {
        return 1;
    }
    if ((k = (unsigned char)key[i] << 8)) {
        k |= (unsigned char)key[i + 1];
    }
    return nib_bit(k, t->branch.flags);
}

static inline int
has_twig(FPST *t, uint32_t bit)
{
    return t->branch.bitmap & bit;
}

static inline unsigned int
twig_off(FPST *t, uint32_t b)
{
    return popcount_u32(t->branch.bitmap & (b - 1));
}

static inline FPST *
twig(FPST *t, unsigned int i)
{
    return &t->branch.twigs[i];
}

#define TWIG_OFF_MAX(off, max, t, b)                                           \
    do {                                                                       \
        off = twig_off(t, b);                                                  \
        max = popcount_u32(t->branch.bitmap);                                  \
    } while (0)


FPST *
fpst_new(void)
{
    return NULL;
}

int
fpst_starts_with_existing_key(FPST *trie, const char *str, size_t len,
                              const char **found_key_p, uint64_t *found_val_p)
{
    FPST       *t;
    const char *lk;
    size_t      i;

    if (trie == NULL) {
        return 0;
    }
    t = trie;
    while (is_branch(t)) {
        uint32_t b;

        prefetch(t->branch.twigs);
        b = twig_bit(t, str, len);
        if (!has_twig(t, b)) {
            return 0;
        }
        t = twig(t, twig_off(t, b));
    }
    lk = t->leaf.key;
    for (i = 0; lk[i] != 0; i++) {
        if (lk[i] != str[i]) {
            return 0;
        }
    }
    if (found_key_p != NULL) {
        *found_key_p = t->leaf.key;
    }
    if (found_val_p != NULL) {
        *found_val_p = ((uint64_t) t->leaf.val0) | (((uint64_t) t->leaf.val1) << 32);
    }
    return 1;
}

int
fpst_str_starts_with_existing_key(FPST *trie, const char *str,
                                  const char **found_key_p, uint64_t *found_val_p)
{
    return fpst_starts_with_existing_key(trie, str, strlen(str),
                                         found_key_p, found_val_p);
}

int
fpst_has_key(FPST *trie, const char *key, size_t len, uint64_t *found_val_p)
{
    const char *found_key;
    int         ret;

    ret = fpst_starts_with_existing_key(trie, key, len,
                                        &found_key, found_val_p);
    if (ret > 0 && strlen(found_key) != len) {
        ret = 0;
    }
    return ret;
}

int
fpst_has_key_str(FPST *trie, const char *key, uint64_t *found_val_p)
{
    return fpst_has_key(trie, key, strlen(key), found_val_p);
}

FPST *
fpst_insert(FPST *trie, const char *key, size_t len, uint64_t val)
{
    FPST *       t;
    size_t       i;
    unsigned int f;
    unsigned int k1, k2;
    uint32_t     b1;
    uint32_t     val0 = (uint32_t) val;
    uint32_t     val1 = (uint32_t) (val >> 32);

    if ((val1 & 0x80000000) != 0 || len > 0xFFFFFF) {
        abort();
    }
    if (trie == NULL) {
        if ((trie = malloc(sizeof *trie)) == NULL) {
            return NULL;
        }
        trie->leaf.key = key;
        trie->leaf.val0 = val0;
        trie->leaf.val1 = val1;

        return trie;
    }
    t = trie;
    while (is_branch(t)) {
        uint32_t b;

        prefetch(t->branch.twigs);
        b = twig_bit(t, key, len);
        t = twig(t, has_twig(t, b) ? twig_off(t, b) : 0);
    }
    for (i = 0; i <= len; i++) {
        f = (unsigned char)key[i] ^ (unsigned char)t->leaf.key[i];
        if (f != 0) {
            goto newkey;
        }
    }
    t->leaf.val0 = val0;
    t->leaf.val1 = val1;

    return trie;

newkey: {
    size_t bit = i * 8 + clz_u8(f);
    size_t qi  = bit / 5;

    i  = qi * 5 / 8;
    f  = qi * 5 % 8 | 8;
    k1 = (unsigned char)key[i] << 8;
    k2 = (unsigned char)t->leaf.key[i] << 8;
    k1 |= (k1 ? (unsigned char)key[i + 1] : 0);
    k2 |= (k2 ? (unsigned char)t->leaf.key[i + 1] : 0);
    b1 = nib_bit(k1, f);
    t  = trie;
    while (is_branch(t)) {
        uint32_t b;

        prefetch(t->branch.twigs);
        if (i == t->branch.index && f == t->branch.flags) {
            goto growbranch;
        }
        if ((i == t->branch.index && f < t->branch.flags) ||
            (i < t->branch.index)) {
            goto newbranch;
        }
        b = twig_bit(t, key, len);
        t = twig(t, twig_off(t, b));
    }
}

newbranch : {
    FPST *   twigs;
    FPST     t1;
    FPST     t2;
    uint32_t b2;

    if ((twigs = malloc(sizeof(FPST) * 2)) == NULL) {
        return NULL;
    }
    t1.leaf.key      = key;
    t1.leaf.val0     = val0;
    t1.leaf.val1     = val1;
    t2               = *t;
    b2               = nib_bit(k2, f);
    t->branch.twigs  = twigs;
    t->branch.flags  = f;
    t->branch.index  = i;
    t->branch.bitmap = b1 | b2;
    *twig(t, twig_off(t, b1)) = t1;
    *twig(t, twig_off(t, b2)) = t2;

    return trie;
}

growbranch: {
    FPST *       twigs;
    FPST         t1;
    unsigned int s, m;

    TWIG_OFF_MAX(s, m, t, b1);
    if ((twigs = realloc(t->branch.twigs, sizeof(FPST) * (m + 1))) == NULL) {
        return NULL;
    }
    t1.leaf.key  = key;
    t1.leaf.val0 = val0;
    t1.leaf.val1 = val1;
    memmove(twigs + s + 1, twigs + s, sizeof(FPST) * (m - s));
    memmove(twigs + s, &t1, sizeof(FPST));
    t->branch.twigs = twigs;
    t->branch.bitmap |= b1;

    return trie;
}}

FPST *
fpst_insert_str(FPST *trie, const char *key, uint64_t val)
{
        return fpst_insert(trie, key, strlen(key), val);
}

static void
fpst_free_leaf(Leaf *leaf, FPST_FreeFn fpst_free_kv_fn)
{
    if (fpst_free_kv_fn != NULL) {
        fpst_free_kv_fn(leaf->key,
                        ((uint64_t) leaf->val0) | (((uint64_t) leaf->val1) << 32));
    }
    leaf->key = NULL;
    leaf->val0 = leaf->val1 = 0;
}

static void
fpst_free_branch(Branch *branch, FPST_FreeFn fpst_free_kv_fn)
{
    unsigned int count;
    unsigned int i;

    count = popcount_u32(branch->bitmap);
    for (i = 0; i < count; i++) {
        if (is_branch(&branch->twigs[i])) {
            fpst_free_branch(&branch->twigs[i].branch, fpst_free_kv_fn);
        } else {
            fpst_free_leaf(&branch->twigs[i].leaf, fpst_free_kv_fn);
        }
    }
    free(branch->twigs);
    branch->twigs = NULL;
}

void
fpst_free(FPST *trie, FPST_FreeFn free_kv_fn)
{
    if (trie == NULL) {
        return;
    }
    if (is_branch(trie)) {
        fpst_free_branch(&trie->branch, free_kv_fn);
    } else {
        fpst_free_leaf(&trie->leaf, free_kv_fn);
        free(trie);
    }
}
