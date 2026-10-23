#ifndef FLASH_MODEL_H
#define FLASH_MODEL_H

#include "updater.h"

typedef struct {
    uint8_t slot[2][UPDATE_SLOT_SIZE];
    int32_t write_budget; /* -1: unlimited; 0: next byte write fails */
} flash_model_t;

void flash_model_init(flash_model_t *model);
update_flash_t flash_model_interface(flash_model_t *model);

#endif
