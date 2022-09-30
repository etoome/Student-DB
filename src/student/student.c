#include "student.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

void student_to_str(char *buffer, student_t *s)
{
    int day = s->birthdate.tm_mday;
    int month = s->birthdate.tm_mon;
    int year = s->birthdate.tm_year;
    sprintf(buffer, "%u: %s %s in section %s, born on the %d/%d/%d", s->id, s->fname, s->lname, s->section, day, month + 1, year + 1900);
}

int student_equals(student_t *s1, student_t *s2)
{
    return s1->id == s2->id && strcmp(s1->fname, s2->fname) == 0 && strcmp(s1->lname, s2->lname) == 0 && strcmp(s1->section, s2->section) == 0 && s1->birthdate.tm_mday == s2->birthdate.tm_mday && s1->birthdate.tm_mon == s2->birthdate.tm_mon && s1->birthdate.tm_year == s2->birthdate.tm_year;
}
