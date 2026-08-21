//
// benchmark.c
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../include/storage/table_api.h"
#include "../headers/data/student.h"

#define PAGE_DATABASE_PATH "./databases/page_database.db"
#define NORMAL_DATABASE_PATH "./databases/normal_database.db"

#define NUMBER_OF_STUDENTS 1000

/* ===========================================================
                        NAME DATABASE
   =========================================================== */

static const char *FIRST_NAMES[] = {
    "Aarav","Vivaan","Aditya","Arjun","Krishna",
    "Rahul","Rohan","Karan","Akash","Vikram",
    "Amit","Ankit","Harsh","Yash","Dev",
    "Kabir","Ishaan","Aryan","Siddharth","Raghav",
    "Neha","Priya","Ananya","Diya","Kavya",
    "Sneha","Pooja","Aditi","Riya","Meera",
    "Sanya","Nisha","Isha","Tanvi","Khushi",
    "Raj","Ravi","Manish","Suresh","Mahesh",
    "Om","Shiv","Parth","Dhruv","Ved",
    "Laksh","Reyansh","Madhav","Nirav","Tanish"
};

static const char *LAST_NAMES[] = {
    "Sharma","Patel","Singh","Gupta","Verma",
    "Yadav","Joshi","Mishra","Agarwal","Malhotra",
    "Kapoor","Bhat","Kulkarni","Deshmukh","Pawar",
    "Naik","Nair","Reddy","Iyer","Menon",
    "Choudhary","Saxena","Pandey","Tripathi","Dubey",
    "Jain","Mehta","Chauhan","Thakur","Rana",
    "Kamble","Patil","Ghosh","Bose","Roy",
    "Das","Pillai","Shetty","Fernandes","D'Souza",
    "Chavan","Sawant","Bhatt","Chopra","Sinha",
    "Bansal","Arora","Goel","Kohli","Chaudhari"
};

#define FIRST_NAME_COUNT (sizeof(FIRST_NAMES)/sizeof(FIRST_NAMES[0]))
#define LAST_NAME_COUNT  (sizeof(LAST_NAMES)/sizeof(LAST_NAMES[0]))

/* ===========================================================
                        SHUFFLE
   =========================================================== */

void shuffle(int *arr, int n)
{
    for (int i = n - 1; i > 0; i--)
    {
        int j = rand() % (i + 1);

        int t = arr[i];
        arr[i] = arr[j];
        arr[j] = t;
    }
}

/* ===========================================================
                    STUDENT GENERATOR
   =========================================================== */

Student **prepare_bulk_students()
{
    Student **students =
            malloc(sizeof(Student *) * NUMBER_OF_STUDENTS);

    for (int i = 0; i < NUMBER_OF_STUDENTS; i++)
    {
        students[i] = malloc(sizeof(Student));

        students[i]->id = i + 1;

        students[i]->age = 18 + rand() % 43;

        const char *first =
                FIRST_NAMES[rand() % FIRST_NAME_COUNT];

        const char *last =
                LAST_NAMES[rand() % LAST_NAME_COUNT];

        size_t len =
                strlen(first) + strlen(last) + 2;

        students[i]->name = malloc(len);

        snprintf(
                students[i]->name,
                len,
                "%s %s",
                first,
                last
        );
    }

    return students;
}

void destroy_bulk_students(Student **students)
{
    for (int i = 0; i < NUMBER_OF_STUDENTS; i++)
    {
        free(students[i]->name);
        free(students[i]);
    }

    free(students);
}

/* ===========================================================
                    TIMER
   =========================================================== */

double now()
{
    return (double)clock() / CLOCKS_PER_SEC;
}

/* ===========================================================
                NON PAGING INSERT
   =========================================================== */

double benchmark_insert_non_paging(Student **students)
{
    FILE *fp =
            fopen(NORMAL_DATABASE_PATH, "wb");

    if (!fp)
    {
        printf("Unable to open normal database.\n");
        exit(EXIT_FAILURE);
    }

    double start = now();

    for (int i = 0; i < NUMBER_OF_STUDENTS; i++)
    {
        fwrite(
                students[i],
                sizeof(Student),
                1,
                fp
        );
    }

    fflush(fp);

    double end = now();

    fclose(fp);

    return end - start;
}

/* ===========================================================
                NON PAGING RANDOM READ
   =========================================================== */

double benchmark_read_non_paging(int ids[])
{
    FILE *fp =
            fopen(NORMAL_DATABASE_PATH, "rb");

    if (!fp)
    {
        printf("Unable to open normal database.\n");
        exit(EXIT_FAILURE);
    }

    Student s;

    double start = now();

    for (int i = 0; i < NUMBER_OF_STUDENTS; i++)
    {
        int row = ids[i] - 1;

        fseek(
                fp,
                row * sizeof(Student),
                SEEK_SET
        );

        fread(
                &s,
                sizeof(Student),
                1,
                fp
        );
    }

    double end = now();

    fclose(fp);

    return end - start;
}

/* ===========================================================
                    PAGING INSERT
   =========================================================== */

double benchmark_insert_paging(Student **students, TID tids[])
{
    TableAPI *api = table_api_open(PAGE_DATABASE_PATH);

    double start = now();

    for (int i = 0; i < NUMBER_OF_STUDENTS; i++)
    {
        tids[i] = insert_data(
                api,
                students[i],
                sizeof(Student)
        );
    }

    double end = now();

    table_api_close(api);

    return end - start;
}

/* ===========================================================
                    PAGING RANDOM READ
   =========================================================== */

double benchmark_read_paging(TID tids[], int ids[])
{
    TableAPI *api = table_api_open(PAGE_DATABASE_PATH);

    Student temp;

    double start = now();

    for (int i = 0; i < NUMBER_OF_STUDENTS; i++)
    {
        read_data(
            api,
            tids[ids[i] - 1],
            &temp,
            sizeof(Student)
        );
    }

    double end = now();

    table_api_close(api);

    return end - start;
}

/* ===========================================================
                    REPORT
   =========================================================== */

void print_separator()
{
    printf("=============================================================\n");
}

void print_results(
        double page_insert,
        double normal_insert,
        double page_read,
        double normal_read)
{
    print_separator();
    printf("              STORAGE ENGINE BENCHMARK\n");
    print_separator();

    printf("\nDataset\n");
    printf("Students : %d\n", NUMBER_OF_STUDENTS);

    printf("\n---------------- INSERTION ----------------\n");

    printf("%-20s %.6f sec\n",
           "Paging",
           page_insert);

    printf("%-20s %.6f sec\n",
           "Non Paging",
           normal_insert);

    printf("\n---------------- RANDOM READS ----------------\n");

    printf("%-20s %.6f sec\n",
           "Paging",
           page_read);

    printf("%-20s %.6f sec\n",
           "Non Paging",
           normal_read);

    printf("\n");

    if (page_insert < normal_insert)
        printf("Insertion Winner : Paging\n");
    else
        printf("Insertion Winner : Non Paging\n");

    if (page_read < normal_read)
        printf("Read Winner      : Paging\n");
    else
        printf("Read Winner      : Non Paging\n");

    print_separator();
}

/* ===========================================================
                    MAIN
   =========================================================== */

int main()
{
    srand(time(NULL));

    printf("\nPreparing %d students...\n\n",
           NUMBER_OF_STUDENTS);

    Student **students =
            prepare_bulk_students();

    TID tids[NUMBER_OF_STUDENTS];

    int ids[NUMBER_OF_STUDENTS];

    for (int i = 0; i < NUMBER_OF_STUDENTS; i++)
        ids[i] = i + 1;

    shuffle(ids, NUMBER_OF_STUDENTS);

    printf("Running insertion benchmark (Paging)...\n");

    double page_insert =
            benchmark_insert_paging(
                    students,
                    tids);

    printf("Running insertion benchmark (Non Paging)...\n");

    double normal_insert =
            benchmark_insert_non_paging(
                    students);

    printf("Running random read benchmark (Paging)...\n");

    double page_read =
            benchmark_read_paging(
                    tids,
                    ids);

    printf("Running random read benchmark (Non Paging)...\n");

    double normal_read =
            benchmark_read_non_paging(
                    ids);

    print_results(
            page_insert,
            normal_insert,
            page_read,
            normal_read);

    destroy_bulk_students(students);

    return 0;
}