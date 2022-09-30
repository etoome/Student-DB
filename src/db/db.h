#ifndef _DB_H
#define _DB_H

#include "../student/student.h"

typedef struct
{
    int size;        /** The logical size of the list **/
    student_t *data; /** The list of students **/
} database_t;

typedef struct
{
    const char *field;
    const char *value;
} SelectFilter;

typedef struct
{
    int size;
    int *data;
} MatchesIndexes;

typedef struct
{
    database_t *db;
    int start;
    int end;
    SelectFilter filter;
    MatchesIndexes *indexes;
} ArgsSelectRange;

/**
 * Initialise a database_t structure.
 * Typical use:
 * ```
 * database_t db;
 * db_init(&db);
 * ```
 **/
void db_init(database_t *db);

/**
 * Load the content of a database of students from a file.
 **/
void db_load(database_t *db, const char *path);

/**
 * Get students index from the database.
 **/
MatchesIndexes *db_get(database_t *db, const char *field, const char *value);

/** 
 *  Add a student to the database.
 **/
void db_add(database_t *db, student_t s);

/**
 * Delete a student from the database.
 **/
void db_delete(database_t *db, const unsigned int index);

/**
 * Update a student data from the database.
 **/
void db_update(database_t *db, const unsigned int index, const char *field, const char *value);

/**
 * Save the content of a database_t to the specified file.
 **/
void db_save(database_t *db, const char *path);

#endif
