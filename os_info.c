/*
 * os_info: reports the host operating system's name and version.
 */

#include "os_info.h"

#include <stdio.h>
#include <string.h>
#include <sys/utsname.h>

/* Reads PRETTY_NAME out of /etc/os-release into out. Leaves out empty if
 * the file or field isn't found. */
static void read_os_release_name(char *out, size_t out_size) {
    out[0] = '\0';

    FILE *fp = fopen("/etc/os-release", "r");
    if (!fp)
        return;

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "PRETTY_NAME=", 12) != 0)
            continue;

        char *value = line + 12;
        size_t len = strlen(value);
        while (len > 0 && (value[len - 1] == '\n' || value[len - 1] == '\r'))
            value[--len] = '\0';

        if (len >= 2 && value[0] == '"' && value[len - 1] == '"') {
            value[len - 1] = '\0';
            value++;
        }

        snprintf(out, out_size, "%s", value);
        break;
    }

    fclose(fp);
}

void print_os_info(void) {
    char pretty_name[256];
    read_os_release_name(pretty_name, sizeof(pretty_name));

    struct utsname uts;
    int have_uname = (uname(&uts) == 0);

    printf("Operating System: %s",
           pretty_name[0] ? pretty_name : (have_uname ? uts.sysname : "Unknown"));

    if (have_uname)
        printf(" (Kernel %s, %s)", uts.release, uts.machine);

    printf("\n");
}
