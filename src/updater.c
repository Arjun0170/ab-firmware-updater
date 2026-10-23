#include "updater.h"

#include <string.h>

#define HEADER_MAGIC 0x46575550u /* FWUP */
#define COMMIT_MARKER 0x434F4D54u /* COMT, written last */

static void store_u32(uint8_t *p, uint32_t v) {
    for (unsigned i = 0; i < 4u; ++i) p[i] = (uint8_t)(v >> (8u * i));
}
static uint32_t load_u32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static uint32_t crc32_next(uint32_t crc, uint8_t byte) {
    crc ^= byte;
    for (unsigned bit = 0; bit < 8u; ++bit)
        crc = (crc & 1u) ? (crc >> 1) ^ 0xEDB88320u : crc >> 1;
    return crc;
}
uint32_t update_crc32(const uint8_t *data, size_t length) {
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < length; ++i) crc = crc32_next(crc, data[i]);
    return ~crc;
}

void updater_init(updater_t *updater, update_flash_t flash) {
    memset(updater, 0, sizeof(*updater));
    updater->flash = flash;
}

static bool slot_valid(const updater_t *u, unsigned slot, boot_image_t *image) {
    uint8_t header[24];
    if (u->flash.read == 0 || !u->flash.read(u->flash.ctx, slot, 0u,
                                             header, sizeof(header))) return false;
    uint32_t length = load_u32(header + 8);
    if (load_u32(header) != HEADER_MAGIC ||
        load_u32(header + 20) != COMMIT_MARKER ||
        length == 0u || length > UPDATE_MAX_IMAGE ||
        load_u32(header + 16) != update_crc32(header, 16u)) return false;
    uint32_t crc = 0xFFFFFFFFu;
    for (uint32_t i = 0; i < length; ++i) {
        uint8_t byte;
        if (!u->flash.read(u->flash.ctx, slot, UPDATE_IMAGE_OFFSET + i,
                           &byte, 1u)) return false;
        crc = crc32_next(crc, byte);
    }
    if (~crc != load_u32(header + 12)) return false;
    image->slot = slot;
    image->version = load_u32(header + 4);
    image->length = length;
    return true;
}

bool updater_select_boot(const updater_t *u, boot_image_t *image) {
    if (u == 0 || image == 0) return false;
    bool found = false;
    boot_image_t candidate;
    for (unsigned slot = 0; slot < 2u; ++slot) {
        if (slot_valid(u, slot, &candidate) &&
            (!found || candidate.version > image->version)) {
            *image = candidate;
            found = true;
        }
    }
    return found;
}

update_result_t updater_install(updater_t *u, uint32_t version,
                                const uint8_t *image, size_t length) {
    if (u == 0 || u->flash.read == 0 || u->flash.write == 0 ||
        u->flash.erase == 0 || image == 0 || length == 0u ||
        length > UPDATE_MAX_IMAGE)
        return UPDATE_BAD_ARGUMENT;
    boot_image_t active;
    bool has_active = updater_select_boot(u, &active);
    if (has_active && version <= active.version) return UPDATE_BAD_VERSION;
    unsigned target = has_active ? 1u - active.slot : 0u;
    if (!u->flash.erase(u->flash.ctx, target)) return UPDATE_FLASH_ERROR;
    for (size_t offset = 0; offset < length; offset += UPDATE_PROGRAM_BLOCK) {
        size_t block = length - offset;
        if (block > UPDATE_PROGRAM_BLOCK) block = UPDATE_PROGRAM_BLOCK;
        if (!u->flash.write(u->flash.ctx, target,
                            UPDATE_IMAGE_OFFSET + (uint32_t)offset,
                            image + offset, block)) return UPDATE_FLASH_ERROR;
    }
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < length; ++i) {
        uint8_t byte;
        if (!u->flash.read(u->flash.ctx, target,
                           UPDATE_IMAGE_OFFSET + (uint32_t)i, &byte, 1u)) {
            return UPDATE_FLASH_ERROR;
        }
        crc = crc32_next(crc, byte);
    }
    if (~crc != update_crc32(image, length)) return UPDATE_FLASH_ERROR;
    uint8_t header[20] = {0};
    uint8_t marker[4];
    store_u32(header, HEADER_MAGIC);
    store_u32(header + 4, version);
    store_u32(header + 8, (uint32_t)length);
    store_u32(header + 12, ~crc);
    store_u32(header + 16, update_crc32(header, 16u));
    store_u32(marker, COMMIT_MARKER);
    if (!u->flash.write(u->flash.ctx, target, 0u,
                        header, sizeof(header)) ||
        !u->flash.write(u->flash.ctx, target, 20u,
                         marker, sizeof(marker))) {
        return UPDATE_FLASH_ERROR;
    }
    return UPDATE_OK;
}
