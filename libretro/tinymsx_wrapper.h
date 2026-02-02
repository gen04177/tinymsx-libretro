#pragma once
#include <stddef.h>
#include <stdint.h>
#include "../src/tinymsx_def.h"

#ifdef __cplusplus
extern "C" {
#endif

void tinymsx_create(const void* rom, size_t romSize,
                    const void* bios, size_t biosSize,
                    size_t ramSize,
                    int machine_type);
void tinymsx_destroy(void);

void tinymsx_run_frame(uint8_t pad1, uint8_t pad2);

const uint16_t* tinymsx_video(void);
const void* tinymsx_audio(size_t* size);
size_t tinymsx_save_state(void *buffer, size_t *size);
bool   tinymsx_load_state(const void *buffer, size_t size);

#ifdef __cplusplus
}
#endif
