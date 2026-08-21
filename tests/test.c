/*
 * test.c
 *
 * Automated regression test suite for the Table API storage engine.
 *
 * This file does NOT manipulate records directly from main(). Instead,
 * main() acts as a test orchestrator that runs a series of independent
 * test_*() functions, each of which validates one area of behavior.
 *
 * As the storage engine grows (page compaction, relocation, free-page
 * management, iterators, indexes, transactions, WAL, recovery, ...),
 * simply add new test_*() functions below and register them in main().
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/storage/table_api.h"
#include "../include/data/student.h"

/* ------------------------------------------------------------------ */
/* Globals                                                             */
/* ------------------------------------------------------------------ */

static TableAPI *g_api = NULL;

static int g_tests_executed = 0;
static int g_tests_passed   = 0;
static int g_tests_failed   = 0;

/* Set by fail() so the current test function can stop early. */
static int g_current_test_failed = 0;

/* ------------------------------------------------------------------ */
/* Setup / Teardown                                                    */
/* ------------------------------------------------------------------ */

static void initialize(void)
{
    g_api = table_api_open(DATABASE_FILE_PATH);

    if (!g_api)
    {
        fprintf(stderr, "FATAL: could not open storage engine at %s\n",
                DATABASE_FILE_PATH);
        exit(EXIT_FAILURE);
    }
}

static void terminate(void)
{
    if (g_api)
    {
        table_api_close(g_api);
        g_api = NULL;
    }
}

/* ------------------------------------------------------------------ */
/* Pretty-printing helpers                                             */
/* ------------------------------------------------------------------ */

static void print_banner(void)
{
    printf("========================================================\n");
    printf("\nStorage Engine Test Suite\n\n");
    printf("========================================================\n\n");
}

static void print_test_start(int index, int total, const char *title,
                              const char *objective)
{
    printf("========================================================\n");
    printf("\nRunning Test %d / %d\n\n", index, total);
    printf("%s\n", title);
    printf("========================================================\n\n");
    printf("Objective\n\n");
    printf("%s\n\n", objective);
    printf("--------------------------------------------------------\n\n");

    g_current_test_failed = 0;
}

static void print_step(int step_number, const char *description)
{
    printf("Step %d\n\n", step_number);
    printf("%s\n\n", description);
}

static void pass(void)
{
    printf("\xE2\x9C\x94 PASS\n\n");
    printf("--------------------------------------------------------\n\n");
}

static void fail(const char *reason, const char *expected, const char *actual)
{
    printf("FAIL\n\n");
    printf("Reason\n\n%s\n\n", reason);
    printf("Expected\n\n%s\n\n", expected);
    printf("Actual\n\n%s\n\n", actual);
    printf("Stopping current test...\n\n");
    printf("--------------------------------------------------------\n\n");

    g_current_test_failed = 1;
}

static void print_test_result(void)
{
    printf("Result\n\n");
    printf("%s\n\n", g_current_test_failed ? "TEST FAILED" : "TEST PASSED");

    g_tests_executed++;
    if (g_current_test_failed)
        g_tests_failed++;
    else
        g_tests_passed++;
}

/* ------------------------------------------------------------------ */
/* Test 1 - TID Stability                                              */
/* ------------------------------------------------------------------ */
/*
 * Verifies that deleted records never "come back":
 *   - deletion actually removes the record
 *   - reads correctly detect a deleted slot
 *   - a new insertion never reuses a still-referenced, unrelated TID
 */
static void test_tid_stability(void)
{
    print_test_start(1, 3, "TID STABILITY VALIDATION",
                      "Verify that deleted records never magically come back.");

    Student alice = { .id = 1, .age = 20, .name = "Alice" };
    Student bob   = { .id = 2, .age = 22, .name = "Bob" };
    Student out;

    /* Step 1: Insert Alice */
    print_step(1, "Insert Alice");
    TID alice_tid = insert_data(g_api, &alice, sizeof(Student));
    if (alice_tid.page_id == INVALID_PAGE_ID || alice_tid.slot_id == INVALID_SLOT_ID)
        fail("insert_data returned an invalid TID",
             "A valid, non-zero TID",
             "TID{page=0, slot=0}");
    else
        pass();
    if (g_current_test_failed) { print_test_result(); return; }

    /* Step 2: Delete Alice */
    print_step(2, "Delete Alice");
    delete_data(g_api, alice_tid);
    pass();

    /* Step 3: Insert Bob */
    print_step(3, "Insert Bob");
    TID bob_tid = insert_data(g_api, &bob, sizeof(Student));
    if (bob_tid.page_id == INVALID_PAGE_ID || bob_tid.slot_id == INVALID_SLOT_ID)  // RIGHT
        fail("insert_data returned an invalid TID",
             "A valid, non-zero TID",
             "TID{page=0, slot=0}");
    else
        pass();
    if (g_current_test_failed) { print_test_result(); return; }

    /* Step 4: Read Alice (must fail, she was deleted) */
    print_step(4, "Read Alice (expected to fail)");
    if (read_data(g_api, alice_tid, &out, sizeof(Student)))
        fail("Read succeeded on a deleted TID",
             "Read should fail (return 0)",
             "Read returned 1");
    else
        pass();
    if (g_current_test_failed) { print_test_result(); return; }

    /* Step 5: Read Bob (must succeed) */
    print_step(5, "Read Bob (expected to succeed)");
    if (!read_data(g_api, bob_tid, &out, sizeof(Student)))
    {
        fail("Read failed for an existing record",
             "Record should exist",
             "Record not found");
    }
    else if (strcmp(out.name, "Bob") != 0 || out.id != 2 || out.age != 22)
    {
        fail("Read returned corrupted data for Bob",
             "id=2, age=22, name=Bob",
             "Mismatched fields");
    }
    else
    {
        pass();
    }
    if (g_current_test_failed) { print_test_result(); return; }

    /* Step 6: Compare TIDs (Bob must not reuse Alice's TID silently) */
    print_step(6, "Compare Alice TID and Bob TID");
    if (alice_tid.page_id == bob_tid.page_id &&
        alice_tid.slot_id == bob_tid.slot_id)
    {
        fail("Bob was assigned the exact same TID as the deleted Alice record",
             "A distinguishable TID (new slot or reused slot with updated identity)",
             "Identical TID reused without distinction");
    }
    else
    {
        pass();
    }

    print_test_result();
}

/* ------------------------------------------------------------------ */
/* Test 2 - Update Engine                                              */
/* ------------------------------------------------------------------ */
/*
 * Verifies update logic:
 *   - in-place update (same size)
 *   - relocation update (returns a new TID)
 *   - deleted records remain unreadable
 */
static void test_updates(void)
{
    print_test_start(2, 3, "UPDATE ENGINE VALIDATION",
                      "Verify update logic.");

    Student alice = { .id = 1, .age = 20, .name = "Alice" };
    Student bob   = { .id = 2, .age = 22, .name = "Bob" };
    Student out;

    /* Step 1: Insert Alice */
    print_step(1, "Insert Alice");
    TID alice_tid = insert_data(g_api, &alice, sizeof(Student));
    pass();

    /* Step 2: Insert Bob */
    print_step(2, "Insert Bob");
    TID bob_tid = insert_data(g_api, &bob, sizeof(Student));
    pass();

    /* Step 3: Read Alice */
    print_step(3, "Read Alice");
    if (!read_data(g_api, alice_tid, &out, sizeof(Student)))
        fail("Read returned 0", "Student should exist", "Record not found");
    else
        pass();
    if (g_current_test_failed) { print_test_result(); return; }

    /* Step 4: Update Alice (same size -> in-place update) */
    print_step(4, "Update Alice (same size, expect in-place update)");
    alice.age = 21;
    TID alice_new_tid = update_data(g_api, alice_tid, &alice, sizeof(Student));
    if (alice_new_tid.page_id == 0 && alice_new_tid.slot_id == 0)
        fail("update_data returned an invalid TID",
             "A valid TID (same or relocated)",
             "TID{page=0, slot=0}");
    else
        pass();
    if (g_current_test_failed) { print_test_result(); return; }

    /* Step 5: Read Updated Alice */
    print_step(5, "Read Updated Alice");
    if (!read_data(g_api, alice_new_tid, &out, sizeof(Student)))
    {
        fail("Read returned 0 after update",
             "Updated student should exist",
             "Record not found");
    }
    else if (out.age != 21)
    {
        fail("Updated field was not persisted",
             "age = 21",
             "age = (unexpected value)");
    }
    else
    {
        pass();
    }
    if (g_current_test_failed) { print_test_result(); return; }

    /* Step 6: Update Bob */
    print_step(6, "Update Bob");
    bob.age = 23;
    TID bob_new_tid = update_data(g_api, bob_tid, &bob, sizeof(Student));
    pass();

    /* Step 7: Read Bob */
    print_step(7, "Read Updated Bob");
    if (!read_data(g_api, bob_new_tid, &out, sizeof(Student)))
        fail("Read returned 0 after update", "Bob should exist", "Record not found");
    else if (out.age != 23)
        fail("Updated field was not persisted", "age = 23", "age = (unexpected value)");
    else
        pass();
    if (g_current_test_failed) { print_test_result(); return; }

    /* Step 8: Delete Alice */
    print_step(8, "Delete Alice");
    delete_data(g_api, alice_new_tid);
    pass();

    /* Step 9: Read Alice (must fail) */
    print_step(9, "Read Alice (expected to fail)");
    if (read_data(g_api, alice_new_tid, &out, sizeof(Student)))
        fail("Read succeeded on a deleted TID",
             "Read should fail (return 0)",
             "Read returned 1");
    else
        pass();
    if (g_current_test_failed) { print_test_result(); return; }

    /* Step 10: Verify Bob TID still valid */
    print_step(10, "Verify Bob TID is still valid");
    if (!read_data(g_api, bob_new_tid, &out, sizeof(Student)))
        fail("Bob became unreadable after deleting Alice",
             "Bob should remain readable",
             "Record not found");
    else
        pass();

    print_test_result();
}

/* ------------------------------------------------------------------ */
/* Test 3 - Bulk Record Management                                     */
/* ------------------------------------------------------------------ */
/*
 * Stress test:
 *   - many insertions
 *   - multiple deletions
 *   - multiple updates
 *   - free space reuse / free page list correctness
 */
#define BULK_COUNT 10

static void test_bulk_records(void)
{
    print_test_start(3, 3, "BULK RECORD MANAGEMENT VALIDATION",
                      "Stress test insert, delete, update, and free-space reuse.");

    TID tids[BULK_COUNT];
    Student students[BULK_COUNT];
    char student_names[BULK_COUNT][16];
    Student out;
    char step_desc[128];

    /* Step 1: Insert 10 students */
    print_step(1, "Insert 10 students");
    for (int i = 0; i < BULK_COUNT; i++)
    {
        students[i].id = i + 1;
        students[i].age = 18 + i;
        snprintf(student_names[i], sizeof(student_names[i]), "Stu%d", i + 1);
        students[i].name = student_names[i];

        tids[i] = insert_data(g_api, &students[i], sizeof(Student));

        if (tids[i].page_id == 0 && tids[i].slot_id == 0)
        {
            snprintf(step_desc, sizeof(step_desc),
                     "insert_data returned an invalid TID for student %d", i + 1);
            fail(step_desc, "A valid TID", "TID{page=0, slot=0}");
            print_test_result();
            return;
        }
    }
    pass();

    /* Step 2: Read all */
    print_step(2, "Read all 10 students");
    {
        int ok = 1;
        for (int i = 0; i < BULK_COUNT; i++)
        {
            if (!read_data(g_api, tids[i], &out, sizeof(Student)) ||
                out.id != students[i].id)
            {
                ok = 0;
                break;
            }
        }
        if (!ok)
            fail("One or more inserted students could not be read back correctly",
                 "All 10 students readable with matching fields",
                 "Mismatch or missing record");
        else
            pass();
    }
    if (g_current_test_failed) { print_test_result(); return; }

    /* Step 3: Delete records 2, 5, 8 (1-indexed) */
    print_step(3, "Delete students 2, 5, and 8");
    int delete_indexes[3] = { 1, 4, 7 }; /* 0-indexed */
    for (int i = 0; i < 3; i++)
        delete_data(g_api, tids[delete_indexes[i]]);
    pass();

    /* Step 4: Verify deletion */
    print_step(4, "Verify students 2, 5, and 8 are no longer readable");
    {
        int ok = 1;
        for (int i = 0; i < 3; i++)
        {
            if (read_data(g_api, tids[delete_indexes[i]], &out, sizeof(Student)))
            {
                ok = 0;
                break;
            }
        }
        if (!ok)
            fail("A deleted record was still readable",
                 "Deleted records should be unreadable",
                 "Read returned 1 for a deleted TID");
        else
            pass();
    }
    if (g_current_test_failed) { print_test_result(); return; }

    /* Step 5: Update student 3 */
    print_step(5, "Update student 3");
    students[2].age = 99;
    tids[2] = update_data(g_api, tids[2], &students[2], sizeof(Student));
    pass();

    /* Step 6: Update student 7 */
    print_step(6, "Update student 7");
    students[6].age = 77;
    tids[6] = update_data(g_api, tids[6], &students[6], sizeof(Student));
    pass();

    /* Step 7: Insert 3 more students (should be able to reuse freed space) */
    print_step(7, "Insert 3 more students");
    TID new_tids[3];
    Student new_students[3];
    char new_student_names[3][16];
    for (int i = 0; i < 3; i++)
    {
        new_students[i].id = 100 + i;
        new_students[i].age = 30 + i;
        snprintf(new_student_names[i], sizeof(new_student_names[i]),
                 "New%d", i + 1);
        new_students[i].name = new_student_names[i];

        new_tids[i] = insert_data(g_api, &new_students[i], sizeof(Student));

        if (new_tids[i].page_id == 0 && new_tids[i].slot_id == 0)
        {
            fail("insert_data returned an invalid TID for a new student",
                 "A valid TID", "TID{page=0, slot=0}");
            print_test_result();
            return;
        }
    }
    pass();

    /* Step 8: Read every remaining record */
    print_step(8, "Read every remaining record");
    {
        int ok = 1;

        /* Original students, excluding the deleted ones */
        for (int i = 0; i < BULK_COUNT && ok; i++)
        {
            int is_deleted = (i == delete_indexes[0] ||
                               i == delete_indexes[1] ||
                               i == delete_indexes[2]);
            if (is_deleted)
                continue;

            if (!read_data(g_api, tids[i], &out, sizeof(Student)) ||
                out.id != students[i].id ||
                out.age != students[i].age)
            {
                ok = 0;
            }
        }

        /* Newly inserted students */
        for (int i = 0; i < 3 && ok; i++)
        {
            if (!read_data(g_api, new_tids[i], &out, sizeof(Student)) ||
                out.id != new_students[i].id)
            {
                ok = 0;
            }
        }

        if (!ok)
            fail("One or more remaining records failed verification",
                 "All non-deleted and newly inserted records readable and correct",
                 "Mismatch or missing record");
        else
            pass();
    }
    if (g_current_test_failed) { print_test_result(); return; }

    /* Step 9: Verify deleted TIDs still fail */
    print_step(9, "Verify deleted TIDs still fail to read");
    {
        int ok = 1;
        for (int i = 0; i < 3; i++)
        {
            if (read_data(g_api, tids[delete_indexes[i]], &out, sizeof(Student)))
            {
                ok = 0;
                break;
            }
        }
        if (!ok)
            fail("A previously deleted TID became readable again",
                 "Deleted TIDs should remain permanently unreadable",
                 "Read returned 1 for a deleted TID");
        else
            pass();
    }

    print_test_result();
}

/* ------------------------------------------------------------------ */
/* Final Summary                                                       */
/* ------------------------------------------------------------------ */

static void print_summary(void)
{
    printf("========================================================\n\n");
    printf("Storage Engine Regression Suite\n\n");
    printf("========================================================\n\n");
    printf("Tests Executed\n\n%d\n\n", g_tests_executed);
    printf("Passed\n\n%d\n\n", g_tests_passed);
    printf("Failed\n\n%d\n\n", g_tests_failed);
    printf("Overall\n\n%s\n\n", g_tests_failed == 0 ? "SUCCESS" : "FAILED");
    printf("========================================================\n");
}

/* ------------------------------------------------------------------ */
/* Test Orchestrator                                                    */
/* ------------------------------------------------------------------ */

int main(void)
{
    print_banner();

    initialize();

    test_tid_stability();
    test_updates();
    test_bulk_records();

    print_summary();

    terminate();

    return (g_tests_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}