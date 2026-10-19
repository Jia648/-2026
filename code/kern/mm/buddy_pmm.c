#include <buddy_pmm.h>
#include <stdio.h>

/* lab2 2413000 2414072: PFN-aligned blocks, one free list per order. */
#define MAX_ORDER 15
static list_entry_t areas[MAX_ORDER + 1];
static size_t available;
static unsigned order_for(size_t n) {
    unsigned o = 0;
    while (((size_t)1 << o) < n && o < MAX_ORDER) o++;
    return o;
}
static void buddy_init(void) {
    for (unsigned o = 0; o <= MAX_ORDER; o++) list_init(&areas[o]);
    available = 0;
}
static void add_block(struct Page *p, unsigned order) {
    p->property = order;
    SetPageProperty(p);
    list_add(&areas[order], &p->page_link);
}
static void buddy_init_memmap(struct Page *base, size_t n) {
    assert(n > 0);
    for (size_t i = 0; i < n; i++) {
        assert(PageReserved(base + i));
        base[i].flags = base[i].property = 0;
        set_page_ref(base + i, 0);
    }
    available += n;
    while (n) {
        unsigned o = MAX_ORDER;
        while (((size_t)1 << o) > n || (page2ppn(base) & (((size_t)1 << o) - 1))) o--;
        add_block(base, o);
        base += (size_t)1 << o;
        n -= (size_t)1 << o;
    }
}
static struct Page *buddy_alloc_pages(size_t n) {
    if (!n || n > ((size_t)1 << MAX_ORDER) || n > available) return NULL;
    unsigned wanted = order_for(n), o = wanted;
    while (o <= MAX_ORDER && list_empty(&areas[o])) o++;
    if (o > MAX_ORDER) return NULL;
    struct Page *p = le2page(list_next(&areas[o]), page_link);
    list_del(&p->page_link);
    ClearPageProperty(p);
    while (o > wanted) {
        o--;
        add_block(p + ((size_t)1 << o), o);
    }
    size_t count = (size_t)1 << wanted;
    for (size_t i = 0; i < count; i++) {
        p[i].flags = p[i].property = 0;
        set_page_ref(p + i, i ? -1 : (int)wanted + 1);
    }
    available -= count;
    return p;
}
static void buddy_free_pages(struct Page *p, size_t n) {
    assert(n && n <= ((size_t)1 << MAX_ORDER));
    unsigned order = order_for(n);
    assert(p >= pages && p + ((size_t)1 << order) <= pages + npage - nbase);
    assert(page_ref(p) == (int)order + 1 && !PageProperty(p));
    size_t count = (size_t)1 << order;
    for (size_t i = 0; i < count; i++) {
        assert(!PageReserved(p + i));
        assert(i == 0 || page_ref(p + i) == -1);
        set_page_ref(p + i, 0);
    }
    available += count;
    while (order < MAX_ORDER) {
        size_t partner = page2ppn(p) ^ ((size_t)1 << order);
        if (partner < nbase || partner >= npage) break;
        struct Page *q = pages + partner - nbase;
        if (!PageProperty(q) || q->property != order) break;
        list_del(&q->page_link);
        ClearPageProperty(q);
        if (q < p) p = q;
        order++;
    }
    add_block(p, order);
}
static size_t buddy_nr_free_pages(void) { return available; }
static void validate(void) {
    size_t total = 0;
    for (unsigned o = 0; o <= MAX_ORDER; o++) {
        list_entry_t *le = &areas[o];
        while ((le = list_next(le)) != &areas[o]) {
            struct Page *p = le2page(le, page_link);
            assert(PageProperty(p) && p->property == o);
            assert((page2ppn(p) & (((size_t)1 << o) - 1)) == 0);
            total += (size_t)1 << o;
        }
    }
    assert(total == available);
}
static void buddy_check(void) {
    size_t before = available;
    assert(buddy_alloc_pages(0) == NULL);
    assert(buddy_alloc_pages(before + 1) == NULL);
    struct Page *p = buddy_alloc_pages(3);
    assert(p && available == before - 4);
    struct Page *q = buddy_alloc_pages(1);
    assert(q && (q < p || q >= p + 4));
    buddy_free_pages(p, 3);
    buddy_free_pages(q, 1);
    assert(available == before);
    /* All eight single pages from a 2^3 block must coalesce again. */
    p = buddy_alloc_pages(8);
    assert(p);
    buddy_free_pages(p, 8);
    struct Page *items[64];
    for (size_t i = 0; i < 64; i++) {
        items[i] = buddy_alloc_pages(1);
        assert(items[i]);
        for (size_t j = 0; j < i; j++) assert(items[i] != items[j]);
    }
    for (size_t i = 0; i < 64; i += 2) buddy_free_pages(items[i], 1);
    for (size_t i = 1; i < 64; i += 2) buddy_free_pages(items[i], 1);
    assert(available == before);
    validate();
    cprintf("buddy_check() succeeded!\n");
}
const struct pmm_manager buddy_pmm_manager = {
    .name = "buddy_pmm_manager", .init = buddy_init,
    .init_memmap = buddy_init_memmap, .alloc_pages = buddy_alloc_pages,
    .free_pages = buddy_free_pages, .nr_free_pages = buddy_nr_free_pages,
    .check = buddy_check,
};
