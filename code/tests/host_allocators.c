/* Host simulation: executes the actual allocators, not a reimplementation. */
#include <pmm.h>
#include <best_fit_pmm.h>
#include <buddy_pmm.h>
#include <slub.h>
#include <stdio.h>
static struct Page descriptors[2048];
static unsigned char memory[2048 * 4096] __attribute__((aligned(4096)));
struct Page *pages = descriptors;
size_t npage = 2048;
const size_t nbase = 0;
uint64_t va_pa_offset;
const struct pmm_manager *pmm_manager;
int cprintf(const char *fmt, ...) { return 0; }
void __panic(const char *file, int line, const char *fmt, ...) { __builtin_trap(); }
struct Page *alloc_pages(size_t n) { return pmm_manager->alloc_pages(n); }
void free_pages(struct Page *p, size_t n) { pmm_manager->free_pages(p,n); }
size_t nr_free_pages(void) { return pmm_manager->nr_free_pages(); }
static void setup(const struct pmm_manager *manager) {
    va_pa_offset = (uintptr_t)memory;
    pmm_manager = manager;
    for (size_t i = 0; i < 2048; i++) {
        descriptors[i].flags = 0;
        SetPageReserved(descriptors + i);
    }
    manager->init();
    manager->init_memmap(pages,2048);
}
static void stress(void) {
    struct Page *slots[64] = {0};
    size_t counts[64] = {0}, rounded[64] = {0};
    unsigned seed = 1234567;
    for (unsigned step = 0; step < 20000; step++) {
        seed = seed * 1664525U + 1013904223U;
        unsigned i = (seed >> 16) % 64;
        if (slots[i]) { free_pages(slots[i], counts[i]); slots[i] = NULL; }
        else {
            size_t n = ((seed >> 8) % 16) + 1, r = n;
            if (pmm_manager == &buddy_pmm_manager) {
                r = 1; while (r < n) r *= 2;
            }
            struct Page *p = alloc_pages(n);
            if (p) {
                for (unsigned j = 0; j < 64; j++) if (slots[j])
                    assert(p + r <= slots[j] || slots[j] + rounded[j] <= p);
                slots[i] = p; counts[i] = n; rounded[i] = r;
            }
        }
        size_t used = 0;
        for (unsigned j=0;j<64;j++) if(slots[j]) used += rounded[j];
        assert(nr_free_pages() + used == 2048);
    }
    for(unsigned i=0;i<64;i++) if(slots[i]) free_pages(slots[i],counts[i]);
    assert(nr_free_pages()==2048);
}
#ifdef _WIN32
__declspec(dllexport)
#endif
int run_tests(void) {
    setup(&best_fit_pmm_manager);
    pmm_manager->check();
    stress();
    assert(nr_free_pages() == 2048);
    slub_check();
    assert(nr_free_pages() == 2048);
    setup(&buddy_pmm_manager);
    pmm_manager->check();
    stress();
    slub_check();
    assert(nr_free_pages() == 2048);
    /* Exhaust/recover every page and verify coalescing to a full arena. */
    static struct Page *all[2048];
    for (size_t i=0;i<2048;i++) { all[i]=alloc_page(); assert(all[i]); }
    assert(alloc_page()==NULL && nr_free_pages()==0);
    for(size_t i=0;i<2048;i+=2) free_page(all[i]);
    for(size_t i=1;i<2048;i+=2) free_page(all[i]);
    struct Page *big=alloc_pages(2048); assert(big==pages);
    free_pages(big,2048);
    assert(nr_free_pages()==2048);
    return 0;
}
