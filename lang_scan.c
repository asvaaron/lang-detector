/*
 * lang_scan: scans PATH for known language interpreters/compilers
 * and reports which ones are installed, along with their version.
 */

#include "lang_scan.h"

#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Matches a dotted version number, e.g. "14.2.0" inside "gcc ... 14.2.0". */
#define VERSION_NUMBER_PATTERN "[0-9]+(\\.[0-9]+)+"

typedef struct {
    const char *display_name;
    const char *exec_name;
    const char *version_cmd; /* run through the shell, stdout+stderr merged */
} Language;

static const Language languages[] = {
    {"C (GCC)",           "gcc",      "gcc --version"},
    {"C (Clang)",         "clang",    "clang --version"},
    {"C++ (G++)",         "g++",      "g++ --version"},
    {"Python 3",          "python3",  "python3 --version"},
    {"Python 2",          "python2",  "python2 --version"},
    {"Node.js",           "node",     "node --version"},
    {"Java",              "java",     "java -version"},
    {"Go",                "go",       "go version"},
    {"Rust",              "rustc",    "rustc --version"},
    {"Ruby",              "ruby",     "ruby --version"},
    {"PHP",               "php",      "php --version"},
    {"Perl",              "perl",     "perl --version"},
    {"Lua",               "lua",      "lua -v"},
    {"R",                 "R",        "R --version"},
    {"Julia",             "julia",    "julia --version"},
    {".NET",              "dotnet",   "dotnet --version"},
    {"Haskell (GHC)",     "ghc",      "ghc --version"},
    {"Erlang",            "erl",      "erl -version"},
    {"Elixir",            "elixir",   "elixir --version"},
    {"Swift",             "swift",    "swift --version"},
    {"Kotlin",            "kotlin",   "kotlin -version"},
    {"Scala",             "scala",    "scala -version"},
    {"TypeScript",        "tsc",      "tsc --version"},
    {"Deno",              "deno",     "deno --version"},
    {"Bun",               "bun",      "bun --version"},
    {"Bash",              "bash",     "bash --version"},
    {"Zig",               "zig",      "zig version"},
    {"Nim",               "nim",      "nim --version"},
    {"Crystal",           "crystal",  "crystal --version"},
    {"Dart",              "dart",     "dart --version"},
    {"Fortran (gfortran)","gfortran", "gfortran --version"},
    {"COBOL (GnuCOBOL)",  "cobc",     "cobc --version"},
    {"Assembly (NASM)",   "nasm",     "nasm -v"},
};

#define NUM_LANGUAGES (sizeof(languages) / sizeof(languages[0]))

/* Search PATH for exec_name; return 1 if an executable file is found. */
static int is_in_path(const char *exec_name) {
    const char *path_env = getenv("PATH");
    if (!path_env || !*path_env)
        return 0;

    char *path_copy = strdup(path_env);
    if (!path_copy)
        return 0;

    int found = 0;
    char *saveptr = NULL;
    char *dir = strtok_r(path_copy, ":", &saveptr);
    while (dir) {
        char full_path[4096];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir, exec_name);
        if (access(full_path, X_OK) == 0) {
            found = 1;
            break;
        }
        dir = strtok_r(NULL, ":", &saveptr);
    }

    free(path_copy);
    return found;
}

/* Run version_cmd and capture the first non-empty line of its output. */
static void get_version(const char *version_cmd, char *out, size_t out_size) {
    out[0] = '\0';

    char full_cmd[512];
    snprintf(full_cmd, sizeof(full_cmd), "%s 2>&1", version_cmd);

    FILE *fp = popen(full_cmd, "r");
    if (!fp)
        return;

    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        size_t len = strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
            line[--len] = '\0';
        if (len > 0) {
            snprintf(out, out_size, "%s", line);
            break;
        }
    }

    pclose(fp);
}

typedef struct {
    const Language *lang;
    int found;
    char version[512];
    char version_number[64];
} LangResult;

/* Extract the first dotted version number (e.g. "14.2.0") out of version
 * via VERSION_NUMBER_PATTERN. Leaves out empty if compiled_regex is NULL
 * or no match is found. */
static void extract_version_number(const regex_t *compiled_regex, const char *version,
                                    char *out, size_t out_size) {
    out[0] = '\0';
    if (!compiled_regex || !version || !version[0])
        return;

    regmatch_t match;
    if (regexec(compiled_regex, version, 1, &match, 0) != 0)
        return;

    size_t len = (size_t)(match.rm_eo - match.rm_so);
    if (len >= out_size)
        len = out_size - 1;
    memcpy(out, version + match.rm_so, len);
    out[len] = '\0';
}

/* Print str to stdout as a JSON string literal, including the quotes. */
static void print_json_string(const char *str) {
    putchar('"');
    for (const unsigned char *p = (const unsigned char *)str; *p; p++) {
        switch (*p) {
            case '"':  fputs("\\\"", stdout); break;
            case '\\': fputs("\\\\", stdout); break;
            case '\n': fputs("\\n", stdout); break;
            case '\r': fputs("\\r", stdout); break;
            case '\t': fputs("\\t", stdout); break;
            default:
                if (*p < 0x20)
                    printf("\\u%04x", *p);
                else
                    putchar(*p);
        }
    }
    putchar('"');
}

/* Print str to stdout as a CSV field, quoting it if necessary. */
static void print_csv_field(const char *str) {
    int needs_quotes = 0;
    for (const char *p = str; *p; p++) {
        if (*p == ',' || *p == '"' || *p == '\n' || *p == '\r') {
            needs_quotes = 1;
            break;
        }
    }

    if (!needs_quotes) {
        fputs(str, stdout);
        return;
    }

    putchar('"');
    for (const char *p = str; *p; p++) {
        if (*p == '"')
            putchar('"');
        putchar(*p);
    }
    putchar('"');
}

static void print_table(const LangResult *results, size_t count, int installed_count) {
    printf("Scanning system for installed programming languages...\n\n");
    printf("%-22s %-10s %-14s %s\n", "LANGUAGE", "STATUS", "VER#", "VERSION");
    printf("------------------------------------------------------------------------------------\n");

    for (size_t i = 0; i < count; i++) {
        const LangResult *r = &results[i];
        if (!r->found) {
            printf("%-22s %-10s %-14s -\n", r->lang->display_name, "missing", "-");
            continue;
        }
        printf("%-22s %-10s %-14s %s\n", r->lang->display_name, "found",
               r->version_number[0] ? r->version_number : "-",
               r->version[0] ? r->version : "(version unknown)");
    }

    printf("------------------------------------------------------------------------------------\n");
    printf("%d of %zu checked languages appear to be installed.\n",
           installed_count, count);
}

static void print_json(const LangResult *results, size_t count, int installed_count) {
    printf("{\n  \"installed_count\": %d,\n  \"total_count\": %zu,\n  \"languages\": [\n",
           installed_count, count);

    for (size_t i = 0; i < count; i++) {
        const LangResult *r = &results[i];
        printf("    {\"name\": ");
        print_json_string(r->lang->display_name);
        printf(", \"exec\": ");
        print_json_string(r->lang->exec_name);
        printf(", \"found\": %s, \"version\": ", r->found ? "true" : "false");
        print_json_string(r->found && r->version[0] ? r->version : "");
        printf(", \"version_number\": ");
        print_json_string(r->version_number);
        printf("}%s\n", i + 1 < count ? "," : "");
    }

    printf("  ]\n}\n");
}

static void print_csv(const LangResult *results, size_t count) {
    printf("language,exec,status,version,version_number\n");

    for (size_t i = 0; i < count; i++) {
        const LangResult *r = &results[i];
        print_csv_field(r->lang->display_name);
        putchar(',');
        print_csv_field(r->lang->exec_name);
        putchar(',');
        fputs(r->found ? "found" : "missing", stdout);
        putchar(',');
        print_csv_field(r->found ? r->version : "");
        putchar(',');
        print_csv_field(r->version_number);
        putchar('\n');
    }
}

int scan_languages(LangFormat format) {
    LangResult results[NUM_LANGUAGES];
    int installed_count = 0;

    regex_t version_regex;
    int have_regex = (regcomp(&version_regex, VERSION_NUMBER_PATTERN, REG_EXTENDED) == 0);

    for (size_t i = 0; i < NUM_LANGUAGES; i++) {
        LangResult *r = &results[i];
        r->lang = &languages[i];
        r->version[0] = '\0';
        r->version_number[0] = '\0';
        r->found = is_in_path(r->lang->exec_name);

        if (r->found) {
            get_version(r->lang->version_cmd, r->version, sizeof(r->version));
            extract_version_number(have_regex ? &version_regex : NULL, r->version,
                                    r->version_number, sizeof(r->version_number));
            installed_count++;
        }
    }

    if (have_regex)
        regfree(&version_regex);

    switch (format) {
        case LANG_FORMAT_JSON:
            print_json(results, NUM_LANGUAGES, installed_count);
            break;
        case LANG_FORMAT_CSV:
            print_csv(results, NUM_LANGUAGES);
            break;
        case LANG_FORMAT_TABLE:
        default:
            print_table(results, NUM_LANGUAGES, installed_count);
            break;
    }

    return installed_count;
}
