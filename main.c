#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "include/storage/table_api.h"
#include "include/data/student.h"

#define N 10

TableAPI *initialize() {
    return table_api_open(DATABASE_FILE_PATH);
}

void terminate(TableAPI *api) {
    table_api_close(api);
}

int main(void)
{
    TableAPI *api = initialize();

    Student students[N];
    Student temp;
    TID tids[N];

    char *names[N] = {
        "Student1",
        "Student2",
        "Student3",
        "Student4",
        "Student5",
        "Student6",
        "Student7",
        "Student8",
        "Student9",
        "Student10"
    };

    printf("\n========== STORAGE ENGINE TEST ==========\n");

    /* -------------------------------------------------- */
    /* 1. Insert 10 students                              */
    /* -------------------------------------------------- */

    printf("\n[1] Inserting 10 students...\n");

    for (int i = 0; i < N; i++) {

        students[i].id = i + 1;
        students[i].age = 18 + i;
        students[i].name = names[i];

        tids[i] = insert_data(api, &students[i], sizeof(Student));
    }

    printf("PASS\n");

    /* -------------------------------------------------- */
    /* 2. Read all students                               */
    /* -------------------------------------------------- */

    printf("\n[2] Reading all students...\n");

    for (int i = 0; i < N; i++) {

        assert(read_data(api, tids[i], &temp, sizeof(Student)) == 1);
        assert(strcmp(temp.name, names[i]) == 0);
    }

    printf("PASS\n");

    /* -------------------------------------------------- */
    /* 3. Delete students 2,5,8                           */
    /* -------------------------------------------------- */

    printf("\n[3] Deleting students 2,5,8...\n");

    delete_data(api, tids[1]);
    delete_data(api, tids[4]);
    delete_data(api, tids[7]);

    printf("PASS\n");

    /* -------------------------------------------------- */
    /* 4. Verify deleted                                  */
    /* -------------------------------------------------- */

    printf("\n[4] Reading deleted students...\n");

    assert(read_data(api, tids[1], &temp, sizeof(Student)) == 0);
    assert(read_data(api, tids[4], &temp, sizeof(Student)) == 0);
    assert(read_data(api, tids[7], &temp, sizeof(Student)) == 0);

    printf("PASS\n");

    /* -------------------------------------------------- */
    /* 5. Update student 3                                */
    /* -------------------------------------------------- */

    printf("\n[5] Updating student 3...\n");

    Student s3 = {
        .id = 3,
        .age = 99,
        .name = "Student3_UPDATED_WITH_A_LONGER_NAME"
    };

    TID tid3_new =
        update_data(api, tids[2], &s3, sizeof(Student));

    printf("PASS\n");

    /* -------------------------------------------------- */
    /* 6. Update student 7                                */
    /* -------------------------------------------------- */

    printf("\n[6] Updating student 7...\n");

    Student s7 = {
        .id = 7,
        .age = 77,
        .name = "Student7"
    };

    TID tid7_new =
        update_data(api, tids[6], &s7, sizeof(Student));

    printf("PASS\n");

    /* -------------------------------------------------- */
    /* 7. Read updated students                           */
    /* -------------------------------------------------- */

    printf("\n[7] Reading updated students...\n");

    assert(read_data(api, tid3_new, &temp, sizeof(Student)) == 1);
    assert(temp.age == 99);

    assert(read_data(api, tid7_new, &temp, sizeof(Student)) == 1);
    assert(temp.age == 77);

    printf("PASS\n");

    /* -------------------------------------------------- */
    /* 8. Insert 3 new students                           */
    /* -------------------------------------------------- */

    printf("\n[8] Inserting 3 new students...\n");

    Student n1 = {30, "New11", 11};
    Student n2 = {31, "New12", 12};
    Student n3 = {32, "New13", 13};

    TID tid11 = insert_data(api, &n1, sizeof(Student));
    TID tid12 = insert_data(api, &n2, sizeof(Student));
    TID tid13 = insert_data(api, &n3, sizeof(Student));

    (void)tid11;
    (void)tid12;
    (void)tid13;

    printf("PASS\n");

    /* -------------------------------------------------- */
    /* 9. Read all living students                        */
    /* -------------------------------------------------- */

    printf("\n[9] Reading all living students...\n");

    for (int i = 0; i < N; i++) {

        if (i == 1 || i == 4 || i == 7)
            continue;

        TID tid = tids[i];

        if (i == 2)
            tid = tid3_new;

        if (i == 6)
            tid = tid7_new;

        assert(read_data(api, tid, &temp, sizeof(Student)) == 1);
    }

    assert(read_data(api, tid11, &temp, sizeof(Student)) == 1);
    assert(read_data(api, tid12, &temp, sizeof(Student)) == 1);
    assert(read_data(api, tid13, &temp, sizeof(Student)) == 1);

    printf("PASS\n");

    /* -------------------------------------------------- */
    /* 10. Deleted TIDs remain invalid                    */
    /* -------------------------------------------------- */

    printf("\n[10] Verifying deleted TIDs...\n");

    assert(read_data(api, tids[1], &temp, sizeof(Student)) == 0);
    assert(read_data(api, tids[4], &temp, sizeof(Student)) == 0);
    assert(read_data(api, tids[7], &temp, sizeof(Student)) == 0);

    printf("PASS\n");

    printf("\n========== ALL TESTS PASSED ==========\n");

    terminate(api);

    return 0;
}