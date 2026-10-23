#include "flash_model.h"

#include <stdio.h>

static void show_boot(updater_t *updater, const char *phase) {
    boot_image_t image;
    if (updater_select_boot(updater, &image))
        printf("%s: boot slot=%u version=%lu length=%lu\n", phase,
               image.slot, (unsigned long)image.version,
               (unsigned long)image.length);
    else
        printf("%s: no bootable image\n", phase);
}

int main(void) {
    flash_model_t model;
    flash_model_init(&model);
    updater_t updater;
    updater_init(&updater, flash_model_interface(&model));
    uint8_t first[256], second[384];
    for (unsigned i = 0; i < sizeof(first); ++i) first[i] = (uint8_t)(i * 3u);
    for (unsigned i = 0; i < sizeof(second); ++i) second[i] = (uint8_t)(i ^ 0xA5u);
    show_boot(&updater, "empty flash");
    if (updater_install(&updater, 1u, first, sizeof(first)) != UPDATE_OK) return 1;
    show_boot(&updater, "after version 1");
    model.write_budget = 140; /* simulate loss of power during image programming */
    if (updater_install(&updater, 2u, second, sizeof(second)) != UPDATE_FLASH_ERROR)
        return 1;
    updater_init(&updater, flash_model_interface(&model)); /* reset, RAM state lost */
    show_boot(&updater, "after interrupted version 2");
    model.write_budget = -1;
    if (updater_install(&updater, 2u, second, sizeof(second)) != UPDATE_OK) return 1;
    updater_init(&updater, flash_model_interface(&model));
    show_boot(&updater, "after completed version 2");
    model.slot[1][UPDATE_IMAGE_OFFSET] ^= 1u; /* simulated bit corruption */
    show_boot(&updater, "after corruption");
    return 0;
}
