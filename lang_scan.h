/*
 * lang_scan: scans PATH for known language interpreters/compilers
 * and reports which ones are installed, along with their version.
 */

#ifndef LANG_SCAN_H
#define LANG_SCAN_H

typedef enum {
    LANG_FORMAT_TABLE,
    LANG_FORMAT_JSON,
    LANG_FORMAT_CSV
} LangFormat;

/* Scans PATH for all known languages and prints a report to stdout in the
 * requested format. Returns the number of languages found installed. */
int scan_languages(LangFormat format);

#endif /* LANG_SCAN_H */
