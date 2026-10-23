#ifndef UPDATER_H
#define UPDATER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define UPDATE_SLOT_SIZE 4096u
#define UPDATE_IMAGE_OFFSET 32u
#define UPDATE_MAX_IMAGE (UPDATE_SLOT_SIZE - UPDATE_IMAGE_OFFSET)
#define UPDATE_PROGRAM_BLOCK 128u

typedef struct {
    bool (*read)(void *ctx, unsigned slot, uint32_t offset, void *out, size_t len);
    bool (*write)(void *ctx, unsigned slot, uint32_t offset,
                  const void *data, size_t len);
    bool (*erase)(void *ctx, unsigned slot);
    void *ctx;
} update_flash_t;

typedef enum {
    UPDATE_OK = 0,
    UPDATE_BAD_ARGUMENT,
    UPDATE_BAD_VERSION,
    UPDATE_FLASH_ERROR
} update_result_t;

typedef struct {
    update_flash_t flash;
} updater_t;

typedef struct { unsigned slot; uint32_t version; uint32_t length; } boot_image_t;

uint32_t update_crc32(const uint8_t *data, size_t length);
void updater_init(updater_t *updater, update_flash_t flash);
/* Selects highest-version image whose commit marker and readback CRC are valid. */
bool updater_select_boot(const updater_t *updater, boot_image_t *image);
/* Installs a local image into inactive simulated flash, committing only after readback. */
update_result_t updater_install(updater_t *updater, uint32_t version,
                                const uint8_t *image, size_t length);

#endif
