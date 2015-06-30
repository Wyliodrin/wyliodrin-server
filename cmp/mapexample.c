#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmp.h"

#define STORAGESIZE 1024
#define SBUFSIZE    32

static bool string_reader(cmp_ctx_t *ctx, void *data, size_t limit) {
    static uint32_t offset = 0;

    strncpy((char *)data, (const char *)ctx->buf + offset, limit);
    offset += limit;

    if (offset > STORAGESIZE) {
        return false;
    } else {
        return true;
    }
}

static size_t string_writer(cmp_ctx_t *ctx, const void *data, size_t count) {
    if (strlen((char *)ctx->buf) + count >= STORAGESIZE) {
        fprintf(stderr, "STORAGESIZE REACHED\n");
        return 0;
    }

    strncat((char *)ctx->buf, (const char *)data, count);
    return count;
}

void error_and_exit(const char *msg) {
    fprintf(stderr, "%s\n\n", msg);
    exit(EXIT_FAILURE);
}

int main(void) {
    int i;
    cmp_ctx_t cmp;
    uint32_t map_size = 0;
    uint32_t string_size = 0;
    char sbuf[SBUFSIZE] = {0};
    char storage[STORAGESIZE] = {0};

    cmp_init(&cmp, storage, string_reader, string_writer);

    /* Writing */

    if (!cmp_write_map(&cmp, 2))
        error_and_exit(cmp_strerror(&cmp));

    if (!cmp_write_str(&cmp, "Greeting", 8))
        error_and_exit(cmp_strerror(&cmp));

    if (!cmp_write_str(&cmp, "Hello", 5))
        error_and_exit(cmp_strerror(&cmp));

    if (!cmp_write_str(&cmp, "Name", 4))
        error_and_exit(cmp_strerror(&cmp));

    if (!cmp_write_str(&cmp, "Linus", 5))
        error_and_exit(cmp_strerror(&cmp));

    /* Reading */

    if (!cmp_read_map(&cmp, &map_size))
        error_and_exit(cmp_strerror(&cmp));

    for (i = 0; i < map_size; i++) {
        string_size = sizeof(sbuf);
        if (!cmp_read_str(&cmp, sbuf, &string_size))
            error_and_exit(cmp_strerror(&cmp));

        printf("key = %s\n", sbuf);

        string_size = sizeof(sbuf);
        if (!cmp_read_str(&cmp, sbuf, &string_size))
            error_and_exit(cmp_strerror(&cmp));

        printf("value = %s\n", sbuf);
    }

    return EXIT_SUCCESS;
}
