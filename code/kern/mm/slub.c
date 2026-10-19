#include <slub.h>
#include <pmm.h>
#include <string.h>
#include <stdio.h>

/* lab2 2413000 2414072: single-CPU size-class slabs backed by alloc_pages. */
#define CLASSES 8
struct slab {
    list_entry_t link;
    struct Page *page;
    size_t npages, size, total, used;
    unsigned char bitmap[32];
};
#define HEADER ROUNDUP(sizeof(struct slab), 16)
static list_entry_t caches[CLASSES], large;
static bool initialized;
static void init_once(void) {
    if (initialized) return;
    for (unsigned i = 0; i < CLASSES; i++) list_init(&caches[i]);
    list_init(&large);
    initialized = 1;
}
static bool bit(struct slab *s, size_t i) { return (s->bitmap[i / 8] >> (i % 8)) & 1; }
void *kmalloc(size_t size) {
    if (!size) return NULL;
    init_once();
    unsigned cls = 0;
    size_t unit = 16;
    while (unit < size && cls < CLASSES) { unit <<= 1; cls++; }
    if (cls == CLASSES) {
        if (size > (~(size_t)0) - HEADER - PGSIZE + 1) return NULL;
        size_t n = (size + HEADER + PGSIZE - 1) / PGSIZE;
        struct Page *p = alloc_pages(n);
        if (!p) return NULL;
        struct slab *s = (struct slab *)(page2pa(p) + va_pa_offset);
        memset(s, 0, sizeof(*s));
        s->page = p; s->npages = n; s->size = size;
        list_add(&large, &s->link);
        return (char *)s + HEADER;
    }
    list_entry_t *head = &caches[cls], *le = head;
    struct slab *s = NULL;
    while ((le = list_next(le)) != head) {
        struct slab *candidate = to_struct(le, struct slab, link);
        if (candidate->used < candidate->total) { s = candidate; break; }
    }
    if (!s) {
        struct Page *p = alloc_page();
        if (!p) return NULL;
        s = (struct slab *)(page2pa(p) + va_pa_offset);
        memset(s, 0, sizeof(*s));
        s->page = p; s->npages = 1; s->size = unit;
        s->total = (PGSIZE - HEADER) / unit;
        assert(s->total <= sizeof(s->bitmap) * 8);
        list_add(head, &s->link);
    }
    for (size_t i = 0; i < s->total; i++) {
        if (!bit(s, i)) {
            s->bitmap[i / 8] |= 1U << (i % 8);
            s->used++;
            return (char *)s + HEADER + i * s->size;
        }
    }
    panic("slub inconsistent capacity");
}
void kfree(void *ptr) {
    if (!ptr) return;
    init_once();
    list_entry_t *le = &large;
    while ((le = list_next(le)) != &large) {
        struct slab *s = to_struct(le, struct slab, link);
        if ((char *)s + HEADER == ptr) {
            struct Page *p = s->page; size_t n = s->npages;
            list_del(le); free_pages(p, n); return;
        }
    }
    for (unsigned cls = 0; cls < CLASSES; cls++) {
        list_entry_t *head = &caches[cls]; le = head;
        while ((le = list_next(le)) != head) {
            struct slab *s = to_struct(le, struct slab, link);
            uintptr_t begin = (uintptr_t)s + HEADER, address = (uintptr_t)ptr;
            if (address < begin || address >= begin + s->total * s->size) continue;
            size_t offset = address - begin, i = offset / s->size;
            assert(offset % s->size == 0 && bit(s, i));
            s->bitmap[i / 8] &= ~(1U << (i % 8));
            if (--s->used == 0) {
                struct Page *p = s->page;
                list_del(le); free_page(p);
            }
            return;
        }
    }
    panic("kfree: invalid or already freed pointer");
}
void slub_check(void) {
    size_t before = nr_free_pages();
    size_t sizes[] = {1, 15, 16, 17, 31, 32, 33, 63, 64, 65, 127, 128,
                    129, 255, 256, 257, 511, 512, 513, 1023, 1024, 1025,
                    2048, 2049, 4096, 8193};
    void *objects[sizeof(sizes) / sizeof(sizes[0])];
    assert(kmalloc(0) == NULL && kmalloc(~(size_t)0) == NULL);
    kfree(NULL);
    for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); i++) {
        objects[i] = kmalloc(sizes[i]);
        assert(objects[i] && ((uintptr_t)objects[i] % 16 == 0));
        memset(objects[i], (int)i + 1, sizes[i]);
    }
    for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); i++) {
        for (size_t j = 0; j < sizes[i]; j++)
            assert(((unsigned char *)objects[i])[j] == (unsigned char)i + 1);
        kfree(objects[i]);
    }
    assert(nr_free_pages() == before);
    /* Force multiple slabs in one size class and reuse a hole. */
    void *small[300];
    for (size_t i = 0; i < 300; i++) { small[i] = kmalloc(16); assert(small[i]); }
    for (size_t i = 0; i < 300; i += 2) { kfree(small[i]); small[i] = NULL; }
    void *reuse = kmalloc(16); assert(reuse); kfree(reuse);
    for (size_t i = 1; i < 300; i += 2) kfree(small[i]);
    assert(nr_free_pages() == before);
    cprintf("slub_check() succeeded!\n");
}
