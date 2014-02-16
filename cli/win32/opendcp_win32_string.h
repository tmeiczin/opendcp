#include <stddef.h>

char *strndup (const char *s, size_t n) {
    char *c;
    size_t len = strlen(s);

    if (n < len) {
        len = n;
    }

    c = (char *) malloc (len + 1);

    if (c == NULL) {
        return 0;
    }

    c[len] = '\0';

    return (char *)memcpy (c, s, len);
}
