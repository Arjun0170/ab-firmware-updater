#include "flash_model.h"

#include <string.h>

static bool within(unsigned slot, uint32_t offset, size_t length) {
    return slot < 2u && offset <= UPDATE_SLOT_SIZE &&
           length <= UPDATE_SLOT_SIZE - offset;
}
static bool read_flash(void *ctx, unsigned slot, uint32_t offset,
                       void *out, size_t length) {
    flash_model_t *m = ctx;
    if (out == 0 || !within(slot, offset, length)) return false;
    memcpy(out, m->slot[slot] + offset, length);
    return true;
}
static bool write_flash(void *ctx, unsigned slot, uint32_t offset,
                        const void *data, size_t length) {
    flash_model_t *m = ctx;
    const uint8_t *bytes = data;
    if (data == 0 || !within(slot, offset, length)) return false;
    for (size_t i = 0; i < length; ++i) {
        if (m->write_budget == 0) return false;
        if ((m->slot[slot][offset + i] & bytes[i]) != bytes[i]) return false;
        m->slot[slot][offset + i] &= bytes[i];
        if (m->write_budget > 0) --m->write_budget;
    }
    return true;
}
static bool erase_flash(void *ctx, unsigned slot) {
    flash_model_t *m = ctx;
    if (slot >= 2u) return false;
    memset(m->slot[slot], 0xFF, UPDATE_SLOT_SIZE);
    return true;
}
void flash_model_init(flash_model_t *model) {
    memset(model->slot, 0xFF, sizeof(model->slot));
    model->write_budget = -1;
}
update_flash_t flash_model_interface(flash_model_t *model) {
    update_flash_t flash = {read_flash, write_flash, erase_flash, model};
    return flash;
}
