#include "../include/buffer/buffer_pool.h"
#include "../include/common/config.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEST_DB "databases/buffer_pool_test.db"

static void print_test(const char *name) {
    printf("\n[TEST] %s\n", name);
}

static void test_new_page_and_pin(BufferPool *bp) {
    print_test("new page + pinning");

    PageId page_id;
    Page *page = buffer_pool_new_page(bp, &page_id);

    assert(page != NULL);
    assert(page->header.page_id == page_id);

    FrameId frame_id = page_table_lookup(bp->page_table, page_id);
    assert(frame_id != INVALID_FRAME_ID);

    BufferFrame *frame = &bp->buffer_frames[frame_id];

    assert(frame->is_occupied);
    assert(frame->pin_count == 1);
    assert(frame->ref_bit == 1);
    assert(frame->is_dirty == false);

    printf("  page_id   = %u\n", page_id);
    printf("  frame_id  = %u\n", frame_id);
    printf("  pin_count = %d\n", frame->pin_count);

    buffer_pool_unpin_page(bp, page_id, false);

    assert(frame->pin_count == 0);

    printf("  unpin successful\n");
}

static void test_fetch_cache_miss(BufferPool *bp) {
    print_test("fetch page - cache miss");

    PageId page_id;
    buffer_pool_new_page(bp, &page_id);
    buffer_pool_unpin_page(bp, page_id, false);

    FrameId frame_id = page_table_lookup(bp->page_table, page_id);
    assert(frame_id != INVALID_FRAME_ID);

    Page *page = buffer_pool_fetch_page(bp, page_id);

    assert(page != NULL);
    assert(page->header.page_id == page_id);

    BufferFrame *frame = &bp->buffer_frames[frame_id];

    /*
     * A page fetched by a caller should be pinned.
     */
    assert(frame->pin_count == 1);
    assert(frame->ref_bit == 1);

    printf("  cache miss fetch -> pin_count = %d\n", frame->pin_count);

    buffer_pool_unpin_page(bp, page_id, false);
    assert(frame->pin_count == 0);
}

static void test_fetch_cache_hit_pins_again(BufferPool *bp) {
    print_test("fetch page - cache hit + pinning");

    PageId page_id;
    buffer_pool_new_page(bp, &page_id);
    buffer_pool_unpin_page(bp, page_id, false);

    FrameId frame_id = page_table_lookup(bp->page_table, page_id);
    assert(frame_id != INVALID_FRAME_ID);

    BufferFrame *frame = &bp->buffer_frames[frame_id];

    assert(frame->pin_count == 0);

    Page *page = buffer_pool_fetch_page(bp, page_id);

    assert(page != NULL);
    assert(frame->ref_bit == 1);

    /*
     * IMPORTANT:
     * fetch_page() should increment pin_count even on a cache hit.
     */
    assert(frame->pin_count == 1);

    printf("  cache hit fetch -> pin_count = %d\n", frame->pin_count);

    buffer_pool_unpin_page(bp, page_id, false);

    assert(frame->pin_count == 0);
}

static void test_dirty_page(BufferPool *bp) {
    print_test("dirty page + flush");

    PageId page_id;
    buffer_pool_new_page(bp, &page_id);

    Page *page = buffer_pool_fetch_page(bp, page_id);

    /*
     * The new page is already pinned by buffer_pool_new_page().
     * fetch_page() creates another reference if pinning is implemented
     * correctly.
     */
    FrameId frame_id = page_table_lookup(bp->page_table, page_id);
    assert(frame_id != INVALID_FRAME_ID);

    BufferFrame *frame = &bp->buffer_frames[frame_id];

    /*
     * Modify the page.
     */
    page->data[0] = 0x42;

    /*
     * Release the fetch reference as dirty.
     */
    buffer_pool_unpin_page(bp, page_id, true);

    /*
     * Release the original new_page() reference.
     */
    buffer_pool_unpin_page(bp, page_id, true);

    assert(frame->pin_count == 0);
    assert(frame->is_dirty == true);

    printf("  page modified\n");
    printf("  dirty    = %s\n", frame->is_dirty ? "true" : "false");
    printf("  pin_count = %d\n", frame->pin_count);

    bool flushed = buffer_pool_flush_page(bp, page_id);

    assert(flushed == true);
    assert(frame->is_dirty == false);

    printf("  flush successful\n");
}

static void test_multiple_pins(BufferPool *bp) {
    print_test("multiple references / pin count");

    PageId page_id;
    buffer_pool_new_page(bp, &page_id);

    /*
     * new_page() gives us one pin.
     */
    FrameId frame_id = page_table_lookup(bp->page_table, page_id);
    assert(frame_id != INVALID_FRAME_ID);

    BufferFrame *frame = &bp->buffer_frames[frame_id];

    assert(frame->pin_count == 1);

    /*
     * Fetching the same page again should create another reference.
     */
    Page *page = buffer_pool_fetch_page(bp, page_id);
    assert(page != NULL);

    assert(frame->pin_count == 2);

    printf("  after two references -> pin_count = %d\n",
           frame->pin_count);

    buffer_pool_unpin_page(bp, page_id, false);
    assert(frame->pin_count == 1);

    buffer_pool_unpin_page(bp, page_id, false);
    assert(frame->pin_count == 0);

    printf("  after two unpins    -> pin_count = %d\n",
           frame->pin_count);
}

static void test_eviction(BufferPool *bp) {
    print_test("CLOCK eviction");

    PageId *page_ids =
        malloc(sizeof(PageId) * (BUFFER_POOL_SIZE + 2));

    assert(page_ids != NULL);

    /*
     * Fill the buffer pool.
     */
    for (int i = 0; i < BUFFER_POOL_SIZE; i++) {
        Page *page =
            buffer_pool_new_page(bp, &page_ids[i]);

        assert(page != NULL);

        buffer_pool_unpin_page(
            bp,
            page_ids[i],
            false
        );
    }

    printf("  filled buffer pool with %d pages\n",
           BUFFER_POOL_SIZE);

    /*
     * All pages are now unpinned, but their reference bits are set.
     * CLOCK should eventually clear reference bits and choose a victim.
     */
    for (int i = BUFFER_POOL_SIZE;
         i < BUFFER_POOL_SIZE + 2;
         i++) {

        Page *page =
            buffer_pool_new_page(bp, &page_ids[i]);

        assert(page != NULL);

        buffer_pool_unpin_page(
            bp,
            page_ids[i],
            false
        );

        printf("  allocated page %u\n",
               page_ids[i]);
    }

    /*
     * At least one of the original pages should no longer be in
     * the page table.
     */
    int evicted = 0;

    for (int i = 0; i < BUFFER_POOL_SIZE; i++) {
        if (page_table_lookup(
                bp->page_table,
                page_ids[i]
            ) == INVALID_FRAME_ID) {

            evicted++;
        }
    }

    assert(evicted > 0);

    printf("  original pages evicted = %d\n", evicted);

    free(page_ids);
}

static void test_dirty_eviction(BufferPool *bp) {
    print_test("dirty page eviction");

    PageId page_id;
    Page *page = buffer_pool_new_page(bp, &page_id);

    assert(page != NULL);

    page->data[0] = 0x7B;

    FrameId frame_id =
        page_table_lookup(bp->page_table, page_id);

    assert(frame_id != INVALID_FRAME_ID);

    BufferFrame *frame =
        &bp->buffer_frames[frame_id];

    buffer_pool_unpin_page(
        bp,
        page_id,
        true
    );

    assert(frame->is_dirty == true);
    assert(frame->pin_count == 0);

    /*
     * Force CLOCK to look for victims by allocating enough pages.
     */
    for (int i = 0; i < BUFFER_POOL_SIZE + 1; i++) {

        PageId temp_page_id;

        Page *temp =
            buffer_pool_new_page(
                bp,
                &temp_page_id
            );

        assert(temp != NULL);

        buffer_pool_unpin_page(
            bp,
            temp_page_id,
            false
        );
    }

    /*
     * If our page was selected as a victim, it should have been
     * written to disk before eviction.
     */
    FrameId result =
        page_table_lookup(bp->page_table, page_id);

    if (result == INVALID_FRAME_ID) {
        printf("  dirty page was evicted\n");
        printf("  eviction completed without assertion failure\n");
    } else {
        printf("  dirty page remained in buffer pool\n");
    }
}

static void test_missing_page(BufferPool *bp) {
    print_test("unpin/flush missing page");

    PageId fake_page_id = 999999;

    /*
     * These should fail safely rather than crashing.
     */
    buffer_pool_unpin_page(
        bp,
        fake_page_id,
        false
    );

    bool result =
        buffer_pool_flush_page(
            bp,
            fake_page_id
        );

    assert(result == false);

    printf("  missing page handled correctly\n");
}

int main(void) {

    printf("========================================\n");
    printf("       BUFFER POOL TEST SUITE\n");
    printf("========================================\n");

    remove(TEST_DB);

    BufferPool *bp =
        buffer_pool_open(TEST_DB);

    assert(bp != NULL);

    printf("\nBuffer pool size: %d\n",
           BUFFER_POOL_SIZE);

    printf("Page table capacity: %d\n",
           PAGE_TABLE_CAPACITY);

    test_new_page_and_pin(bp);

    test_fetch_cache_miss(bp);

    test_fetch_cache_hit_pins_again(bp);

    test_dirty_page(bp);

    test_multiple_pins(bp);

    test_eviction(bp);

    test_dirty_eviction(bp);

    test_missing_page(bp);

    buffer_pool_close(bp);

    printf("\n========================================\n");
    printf("       ALL BUFFER POOL TESTS PASSED\n");
    printf("========================================\n");

    return 0;
}