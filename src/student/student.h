#ifndef _STUDENT_H
#define _STUDENT_H

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

/**
 * Student structure type.
 **/
typedef struct {
    unsigned id;         /** Unique ID **/
    char fname[64];      /** Firstname **/
    char lname[64];      /** Lastname **/
    char section[64];    /** Section **/
    struct tm birthdate; /** Birth date **/
} student_t;

/**
 * Convert a student to a human-readlable string.
 **/
void student_to_str(char* buffer, student_t* s);

/**
 * Return whether two students are equal or not.
 * Two students are equal when all their fields are equal.
 **/
int student_equals(student_t* s1, student_t* s2);

#endif