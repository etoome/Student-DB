#define __USE_XOPEN
#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/time.h>

#include "./db/db.h"
#include "./utils/parsing.h"

database_t db;

struct timespec timer_now, timer_start, timer_stop;

void startScreen()
{
    printf("\n");
    printf("   ███████ ████████ ██    ██ ██████  ███████ ███    ██ ████████       ██████  ██████  \n");
    printf("   ██         ██    ██    ██ ██   ██ ██      ████   ██    ██          ██   ██ ██   ██ \n");
    printf("   ███████    ██    ██    ██ ██   ██ █████   ██ ██  ██    ██    █████ ██   ██ ██████  \n");
    printf("        ██    ██    ██    ██ ██   ██ ██      ██  ██ ██    ██          ██   ██ ██   ██ \n");
    printf("   ███████    ██     ██████  ██████  ███████ ██   ████    ██          ██████  ██████  \n");
    printf("\n");
}

void intHandler(int sig)
{
    fclose(stdin);
}

void logs(char *query, MatchesIndexes *results)
{
    struct stat st = {0};

    if (stat("./logs", &st) == -1)
    {
        mkdir("./logs", 0700);
    }

    char *command = strtok_r(query, " ", &query);

    if (clock_gettime(CLOCK_REALTIME, &timer_now) != 0)
    {
        fprintf(stderr, "Cannot get time");
        exit(EXIT_FAILURE);
    }
    long long int timestamp = timer_now.tv_sec * 1000000 + timer_now.tv_nsec / 1000;

    char path[256];
    sprintf(path, "./logs/%lld-%s.txt", timestamp, command);

    FILE *fptr;
    fptr = fopen(path, "w");
    if (fptr == NULL)
    {
        fprintf(stderr, "Open %s failed", path);
        exit(EXIT_FAILURE);
    }

    clock_gettime(CLOCK_MONOTONIC_RAW, &timer_stop);

    long long int delta_us = (timer_stop.tv_sec - timer_start.tv_sec) * 1000000 + (timer_stop.tv_nsec - timer_start.tv_nsec) / 1000;
    long int delta_ms = (timer_stop.tv_sec - timer_start.tv_sec) * 1000 + (timer_stop.tv_nsec - timer_start.tv_nsec) / 1000000;

    fprintf(fptr, "Query \"%s %s\" completed in %lld µs (~%ld ms) with %d results\n", command, query, delta_us, delta_ms, results->size);
    for (int i = 0; i < results->size; i++)
    {
        char buffer[256];
        student_to_str(buffer, &db.data[*(results->data + i)]);
        fprintf(fptr, "%s\n", buffer);
    }

    fclose(fptr);
}

char *getDbPath(int argc, char *argv)
{
    char *db_path;
    if (argc == 1)
    {
        db_path = "./students.bin";
    }
    else if (argc == 2)
    {
        db_path = argv;
    }
    else
    {
        fprintf(stderr, "Too many arguments");
        exit(EXIT_FAILURE);
    }

    return db_path;
}

bool validField(char *str)
{
    return (strcasecmp(str, "id") == 0 || strcasecmp(str, "fname") == 0 || strcasecmp(str, "lname") == 0 || strcasecmp(str, "section") == 0 || strcasecmp(str, "birthdate") == 0);
}

int main(int argc, char *argv[])
{
    startScreen();

    signal(SIGINT, intHandler);

    char *db_path = getDbPath(argc, (char *)argv[1]);

    db_init(&db);
    db_load(&db, db_path);

    printf("> ");
    char buffer[256];
    while (fgets(buffer, 256, stdin))
    {
        clock_gettime(CLOCK_MONOTONIC_RAW, &timer_start);

        int len = strlen(buffer);
        if (strcmp(&buffer[len - 1], "\n") == 0)
        {
            buffer[len - 1] = '\0';
        }

        if (!isatty(fileno(stdin))) // stdin is a file or a pipe
        {
            printf("Running query '%s'\n", buffer);
        }

        char query[256];
        strcpy(query, buffer);

        char *tmp_query = buffer;
        char *command = strtok_r(tmp_query, " ", &tmp_query);

        if (strcasecmp(command, "select") == 0)
        {
            char field_filter[10]; // max field lenght : "birthdate" (char[10])
            char value_filter[64]; // max value lenght (char[64]) or sizeof(struct tm) == 56

            if (!parse_selectors(tmp_query, field_filter, value_filter))
            {
                fprintf(stderr, "Query invalid");
            }
            else
            {

                if (!validField(field_filter))
                {
                    fprintf(stderr, "Query filter invalid\n");
                }
                else
                {
                    MatchesIndexes *indexes = db_get(&db, field_filter, value_filter);

                    printf("%d match%s\n", indexes->size, indexes->size > 1 ? "es" : "");

                    for (int i = 0; i < indexes->size; i++)
                    {
                        char s_buffer[256];
                        student_to_str(s_buffer, &db.data[*(indexes->data + i)]);
                        printf("%s\n", s_buffer);
                    }

                    logs(query, indexes);

                    free(indexes->data);
                    free(indexes);
                }
            }
        }
        else if (strcasecmp(command, "insert") == 0)
        {
            unsigned id;
            char fname[64];
            char lname[64];
            char section[64];
            struct tm birthdate;

            if (!parse_insert(tmp_query, &id, fname, lname, section, &birthdate))
            {
                fprintf(stderr, "Query invalid");
            }
            else
            {
                char strid[12];
                sprintf(strid, "%d", id);
                MatchesIndexes *indexes = db_get(&db, "id", strid);

                if (indexes->size > 0)
                {
                    fprintf(stderr, "Student with this id already exist\n");
                }
                else
                {

                    student_t student;
                    student.id = id;
                    strcpy(student.fname, fname);
                    strcpy(student.lname, lname);
                    strcpy(student.section, section);
                    student.birthdate = birthdate;

                    db_add(&db, student);

                    char i_buffer[256];
                    student_to_str(i_buffer, &student);
                    printf("Added %s\n", i_buffer);

                    int last = db.size - 1;
                    MatchesIndexes indexes = {.size = 1, .data = &last};

                    logs(query, &indexes);
                }
            }
        }
        else if (strcasecmp(command, "delete") == 0)
        {
            char field_filter[10];
            char value_filter[64];

            if (!parse_selectors(tmp_query, field_filter, value_filter))
            {
                fprintf(stderr, "Query invalid");
            }
            else
            {
                if (!validField(field_filter))
                {
                    fprintf(stderr, "Query filter invalid\n");
                }
                else
                {
                    MatchesIndexes *indexes = db_get(&db, field_filter, value_filter);

                    printf("%d match%s\n", indexes->size, indexes->size > 1 ? "es" : "");

                    logs(query, indexes);

                    for (int i = 0; i < indexes->size; i++)
                    {
                        int id = *(indexes->data + i);

                        int offset = 0;
                        for (int j = 0; j < i; j++) // count deleted before current index
                        {
                            if (*(indexes->data + j) < id)
                            {
                                offset += 1;
                            }
                        }
                        id -= offset;

                        db_delete(&db, id);
                    }

                    free(indexes->data);
                    free(indexes);
                }
            }
        }
        else if (strcasecmp(command, "update") == 0)
        {
            char field_filter[10];
            char value_filter[64];
            char field_to_update[10];
            char update_value[64];

            if (!parse_update(tmp_query, field_filter, value_filter, field_to_update, update_value))
            {
                fprintf(stderr, "Query invalid\n");
            }
            else
            {
                if (!validField(field_filter))
                {
                    fprintf(stderr, "Query filter invalid\n");
                }
                else
                {
                    MatchesIndexes *indexes = db_get(&db, field_filter, value_filter);

                    printf("%d match%s\n", indexes->size, indexes->size > 1 ? "es" : "");

                    for (int i = 0; i < indexes->size; i++)
                    {
                        int id = *(indexes->data + i);

                        char before_buffer[256];
                        student_to_str(before_buffer, &db.data[*(indexes->data + i)]);
                        printf("%s => ", before_buffer);

                        db_update(&db, id, field_to_update, update_value);

                        char after_buffer[256];
                        student_to_str(after_buffer, &db.data[*(indexes->data + i)]);
                        printf("%s\n", after_buffer);
                    }

                    logs(query, indexes);

                    free(indexes->data);
                    free(indexes);
                }
            }
        }
        else if (strcasecmp(command, "list") == 0)
        {
            printf("%d students in the database\n", db.size);
            for (int i = 0; i < db.size; i++)
            {
                char buffer[256];
                student_to_str(buffer, (db.data + i));
                printf("%s\n", buffer);
            }
        }
        else if (strcasecmp(command, "help") == 0)
        {
            printf(" - select <field>=<value>: returns the list of students that match the single filter <field>=<value>\n");
            printf(" - insert <fname> <lname> <id> <section> <birthdate>: inserts a new student at the end of the database\n");
            printf(" - delete <field>=<value>: deletes all students that match the given filter\n");
            printf(" - update <filter>=<value> set <modify_field>=<modify_value>: modifies all students corresponding to the filter <filter>=<value>, giving the value <modify_value> to the field <modify_field>\n");
            printf(" - list : list all the students in the database\n");
            printf("To close the program please press Ctrl+D or Ctrl+C\n");
        }
        else
        {
            fprintf(stderr, "Command invalid (type 'help' for valid commands)\n");
        }

        printf("> ");
    }

    printf("\nClosing...\n");

    db_save(&db, db_path);

    return EXIT_SUCCESS;
}
