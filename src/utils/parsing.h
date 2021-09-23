#ifndef _PARSING_H
#define _PARSING_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE /* Required to work with strptime, which is OS-dependent */
#endif

#include <stdbool.h>

#include "../student/student.h"

/** 
 * Parse the arguments of an update query.
 * Example:
 * ```
 * char* query = "update id=864030 set fname=Trucmuche"
 * char *command = strtok_r(query, " ", &query);
 * parse_update(query, field_filter, value_filter, field_to_update, update_value);
 * ```
 * @return: false if the parsing was successful, true otherwise
 **/
bool parse_update(char *query_arguments, char *field_filter, char *value_filter, char *field_to_update, char *update_value);

/**
 * Parse the arguments of an insert query.
 * An insert query must specify all the fields as follows:
 * ```
 * char* query = "insert Yannick Molinghen 393354 info 14/02/1995"
 * char* command = strtok_r(query, " ", &query);
 * parse_insert(query, fname, lname, &id, section, &birthdate);
 * ```
 * @return:false if the parsing was successful, true otherwise
 **/
bool parse_insert(char *query_arguments, unsigned *id, char *fname, char *lname, char *section, struct tm *birthdate);

/** 
 * Parse field=value selectors.
 **/
bool parse_selectors(char *query, char *field, char *value);

#endif
