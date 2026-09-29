/*
 * os_info: reports the host operating system's name and version.
 */

#ifndef OS_INFO_H
#define OS_INFO_H

/* Prints a single line identifying the OS name/version and kernel to
 * stdout. Independent of any table/json/csv output format. */
void print_os_info(void);

#endif /* OS_INFO_H */
