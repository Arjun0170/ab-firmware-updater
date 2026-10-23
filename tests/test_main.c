#include "flash_model.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void fill(uint8_t *out, size_t length, uint8_t seed) {
    for (size_t i = 0; i < length; ++i) out[i] = (uint8_t)(seed + i * 17u);
}

int main(void) {
    assert(update_crc32((const uint8_t *)"123456789", 9) == 0xCBF43926u);
    uint8_t v1[180], v2[300], v3[100];
    fill(v1, sizeof(v1), 1);
    fill(v2, sizeof(v2), 2);
    fill(v3, sizeof(v3), 3);
    flash_model_t baseline;
    flash_model_init(&baseline);
    updater_t updater;
    updater_init(&updater, flash_model_interface(&baseline));
    boot_image_t image;
    assert(!updater_select_boot(&updater, &image));
    assert(updater_install(&updater, 1u, v1, sizeof(v1)) == UPDATE_OK);
    assert(updater_select_boot(&updater, &image) && image.slot == 0u &&
           image.version == 1u && image.length == sizeof(v1));
    assert(updater_install(&updater, 1u, v2, sizeof(v2)) == UPDATE_BAD_VERSION);
    assert(updater_install(&updater, 2u, v2, 0) == UPDATE_BAD_ARGUMENT);
    assert(updater_install(&updater, 2u, v2, UPDATE_MAX_IMAGE + 1u) ==
           UPDATE_BAD_ARGUMENT);

    /* Exhaustively interrupt every byte of the image and metadata writes. */
    for (int budget = 0; budget < (int)sizeof(v2) + 24; ++budget) {
        flash_model_t model = baseline;
        model.write_budget = budget;
        updater_init(&updater, flash_model_interface(&model));
        assert(updater_install(&updater, 2u, v2, sizeof(v2)) == UPDATE_FLASH_ERROR);
        updater_init(&updater, flash_model_interface(&model)); /* simulated reset */
        assert(updater_select_boot(&updater, &image));
        assert(image.slot == 0u && image.version == 1u);
    }
    flash_model_t model = baseline;
    updater_init(&updater, flash_model_interface(&model));
    assert(updater_install(&updater, 2u, v2, sizeof(v2)) == UPDATE_OK);
    updater_init(&updater, flash_model_interface(&model));
    assert(updater_select_boot(&updater, &image) && image.slot == 1u &&
           image.version == 2u);
    model.slot[1][UPDATE_IMAGE_OFFSET + 50u] ^= 1u;
    assert(updater_select_boot(&updater, &image) && image.slot == 0u &&
           image.version == 1u);
    model = baseline;
    updater_init(&updater, flash_model_interface(&model));
    assert(updater_install(&updater, 2u, v2, sizeof(v2)) == UPDATE_OK);
    model.slot[1][4] ^= 1u; /* version metadata corruption */
    assert(updater_select_boot(&updater, &image) && image.slot == 0u &&
           image.version == 1u);
    model = baseline;
    updater_init(&updater, flash_model_interface(&model));
    assert(updater_install(&updater, 2u, v2, sizeof(v2)) == UPDATE_OK);
    assert(updater_install(&updater, 3u, v3, sizeof(v3)) == UPDATE_OK);
    assert(updater_select_boot(&updater, &image) && image.slot == 0u &&
           image.version == 3u);
    puts("updater tests passed (324 power-loss boundaries)");
    return 0;
}
