#include "tinymsx_wrapper.h"
#include "../src/tinymsx.h"

#include <cstdint>
#include <cstring>
#include <string>


static TinyMSX* msx = nullptr;
static std::string bios_path;

extern "C" {

void tinymsx_set_bios_path(const char* path)
{
    if (path)
        bios_path = path;
}

void tinymsx_create(const void* rom, size_t romSize,
                    const void* bios, size_t biosSize,
                    size_t ramSize,
                    int machine_type)
{
    if (msx)
    {
        delete msx;
        msx = nullptr;
    }

    msx = new TinyMSX(
        machine_type,
        rom ? (const uint8_t*)rom : nullptr,
        romSize,
        ramSize,
        TINYMSX_COLOR_MODE_RGB555
    );

    if (bios && biosSize > 0)
        msx->loadBiosFromMemory((void*)bios, biosSize);

    msx->reset();
}

void tinymsx_destroy(void)
{
    if (msx)
    {
        delete msx;
        msx = nullptr;
    }
}

void tinymsx_run_frame(uint8_t pad1, uint8_t pad2)
{
    if (msx)
        msx->tick(pad1, pad2);
}

const uint16_t* tinymsx_video(void)
{
    return msx ? msx->getDisplayBuffer() : nullptr;
}

const void* tinymsx_audio(size_t* size)
{
    if (!msx)
    {
        if (size) *size = 0;
        return nullptr;
    }

    return msx->getSoundBuffer(size);
}

size_t tinymsx_save_state(void *buffer, size_t *size)
{
    if (!size)
        return 0;

    const void* state_ptr = msx->saveState(size);
    if (!buffer)
    {

        return *size;
    }


    memcpy(buffer, state_ptr, *size);
    return *size;
}

bool tinymsx_load_state(const void *buffer, size_t size)
{
    if (!buffer || size == 0)
        return false;

    msx->loadState(buffer, size);
    return true;
}


}
