#define _XOPEN_SOURCE // strptime support

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <pthread.h>
#include <unistd.h>
#include <strings.h>

#include <time.h>

#include "db.h"
#include "../student/student.h"

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void db_init(database_t *db)
{
    db->data = NULL;
    db->size = 0;
}

void db_load(database_t *db, const char *path)
{

    if (access(path, F_OK) == 0)
    {
        if (access(path, R_OK) != 0)
        {
            fprintf(stderr, "Cannot read the database (%s)\n", path);
            exit(EXIT_FAILURE);
        }

        if (access(path, W_OK) != 0)
        {
            fprintf(stderr, "Cannot edit the database (%s)\n", path);
            exit(EXIT_FAILURE);
        }

        printf("Loading database.. (%s)\n", path);

        FILE *fptr;
        fptr = fopen(path, "rb");

        struct stat st;
        stat(path, &st);
        int number_of_students = st.st_size / sizeof(student_t);

        student_t *students = malloc(sizeof(student_t) * number_of_students);
        if (students == NULL)
        {
            fprintf(stderr, "Process run out of memory");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < number_of_students; i++)
        {
            fread(&students[i], sizeof(student_t), 1, fptr);
        }

        db->size = number_of_students;
        db->data = &students[0];

        printf("%d student%s loaded\n", db->size, db->size > 1 ? "s" : "");
    }
}

void *thread_select_range(void *arguments)
{
    ArgsSelectRange *args = (ArgsSelectRange *)arguments;

    for (int i = args->start; i < args->end; i++)
    {
        student_t *student = (args->db->data + i);
        char birthdate[11];
        sprintf(birthdate, "%d/%d/%d", student->birthdate.tm_mday, student->birthdate.tm_mon + 1, student->birthdate.tm_year + 1900);

        if ((strcasecmp(args->filter.field, "id") == 0 && student->id == atoi(args->filter.value)) ||
            (strcasecmp(args->filter.field, "fname") == 0 && strcasecmp(student->fname, args->filter.value) == 0) ||
            (strcasecmp(args->filter.field, "lname") == 0 && strcasecmp(student->lname, args->filter.value) == 0) ||
            (strcasecmp(args->filter.field, "section") == 0 && strcasecmp(student->section, args->filter.value) == 0) ||
            (strcasecmp(args->filter.field, "birthdate") == 0 && strcasecmp(birthdate, args->filter.value) == 0))
        {
            pthread_mutex_lock(&mutex);
            args->indexes->size += 1;
            args->indexes->data = realloc(args->indexes->data, args->indexes->size * sizeof(int));
            if (args->indexes->data == NULL)
            {
                fprintf(stderr, "Process run out of memory");
                exit(EXIT_FAILURE);
            }
            args->indexes->data[args->indexes->size - 1] = i;
            pthread_mutex_unlock(&mutex);
        }
    }

    return NULL;
}

MatchesIndexes *db_get(database_t *db, const char *field, const char *value)
{
    int SEARCHING_THREAD = ((int)db->size / 20000) + 1; // Best results with 50 threads for 1000000 students

    pthread_t search_threads[SEARCHING_THREAD];

    ArgsSelectRange args[SEARCHING_THREAD];

    SelectFilter filter = {.field = field, .value = value};

    MatchesIndexes *indexes = malloc(sizeof(MatchesIndexes));
    if (indexes == NULL)
    {
        fprintf(stderr, "Process run out of memory");
        exit(EXIT_FAILURE);
    }
    indexes->size = 0;
    indexes->data = NULL;

    for (int i = 0; i < SEARCHING_THREAD; i++)
    {
        int portion = db->size / SEARCHING_THREAD;

        args[i].db = db;
        args[i].start = portion * i;
        args[i].end = i == SEARCHING_THREAD - 1 ? db->size : (portion * i) + portion;
        args[i].filter = filter;
        args[i].indexes = indexes;

        if (pthread_create(&search_threads[i], NULL, thread_select_range, (void *)&args[i]) != 0)
        {
            fprintf(stderr, "Error when creating a thread\n");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < SEARCHING_THREAD; i++)
    {
        pthread_join(search_threads[i], NULL);
    }

    return indexes;
}

void db_add(database_t *db, student_t student)
{
    db->size += 1;
    db->data = realloc(db->data, db->size * sizeof(student_t));
    if (db->data == NULL)
    {
        fprintf(stderr, "Process run out of memory");
        exit(EXIT_FAILURE);
    }

    db->data[db->size - 1] = student;
}

void db_delete(database_t *db, const unsigned int index)
{
    db->data[index] = db->data[db->size - 1];

    db->size -= 1;
    db->data = realloc(db->data, db->size * sizeof(student_t));
    if (db->data == NULL)
    {
        fprintf(stderr, "Process run out of memory");
        exit(EXIT_FAILURE);
    }
}

void db_update(database_t *db, const unsigned int index, const char *field, const char *value)
{
    if (strcasecmp(field, "id") == 0)
    {
        (db->data + index)->id = atoi(value);
    }
    else if (strcasecmp(field, "fname") == 0)
    {
        strcpy((db->data + index)->fname, value);
    }
    else if (strcasecmp(field, "lname") == 0)
    {
        strcpy((db->data + index)->lname, value);
    }
    else if (strcasecmp(field, "section") == 0)
    {
        strcpy((db->data + index)->section, value);
    }
    else if (strcasecmp(field, "birthdate") == 0)
    {
        strptime(value, "%d/%m/%y", &((db->data + index)->birthdate));
    }
}

void db_save(database_t *db, const char *path)
{
    printf("Saving database... (%s)\n", path);

    FILE *fptr;
    fptr = fopen(path, "wb");
    if (fptr == NULL)
    {
        fprintf(stderr, "Open %s failed", path);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < db->size; i++)
    {
        fwrite(&db->data[i], sizeof(student_t), 1, fptr);
    }

    fclose(fptr);

    printf("%d student%s saved\n", db->size, db->size > 1 ? "s" : "");

    free(db->data);
}
