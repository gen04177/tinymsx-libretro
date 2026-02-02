#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#if defined(_WIN32) && !defined(_XBOX)
#include <windows.h>
#endif

#include "libretro.h"
#include "tinymsx_wrapper.h"


#define VIDEO_WIDTH   284
#define VIDEO_HEIGHT  240
#define VIDEO_PITCH   (VIDEO_WIDTH * sizeof(uint16_t))

static const struct retro_variable vars[] = {
    {
        "tinymsx_machine_type",
        "Machine Type; MSX1|MSX1 ASC8|MSX1 ASC8X"
    },
    { NULL, NULL }
};


static struct retro_log_callback logging;
static retro_log_printf_t log_cb;

static retro_environment_t environ_cb;
static retro_video_refresh_t video_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;

static void *rom_data = NULL;
static size_t rom_size = 0;

char retro_base_directory[4096];
char retro_game_path[4096];

static void update_core_options(void)
{
    struct retro_variable var = {0};

    var.key = "tinymsx_machine_type";

    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
    {
        if (!strcmp(var.value, "MSX1 ASC8"))
            tinymsx_type = TINYMSX_TYPE_MSX1_ASC8;
        else if (!strcmp(var.value, "MSX1 ASC8X"))
            tinymsx_type = TINYMSX_TYPE_MSX1_ASC8X;
        else
            tinymsx_type = TINYMSX_TYPE_MSX1;
    }
}


static void
fallback_log (enum retro_log_level level, const char *fmt, ...)
{
  (void) level;
  va_list va;
  va_start (va, fmt);
  vfprintf (stderr, fmt, va);
  va_end (va);
}


unsigned
retro_api_version (void)
{
  return RETRO_API_VERSION;
}

void
retro_set_environment (retro_environment_t cb)
{
  environ_cb = cb;

  if (cb (RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &logging))
    log_cb = logging.log;
  else
    log_cb = fallback_log;

  static const struct retro_controller_description controllers[] = {
    {"RetroPad", RETRO_DEVICE_JOYPAD},
  };

  static const struct retro_controller_info ports[] = {
    {controllers, 1},
    {NULL, 0},
  };

  cb (RETRO_ENVIRONMENT_SET_CONTROLLER_INFO, (void *) ports);
  cb(RETRO_ENVIRONMENT_SET_VARIABLES, (void*)vars);
}

void
retro_init (void)
{
  const char *dir = NULL;
  if (environ_cb (RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &dir) && dir)
    snprintf (retro_base_directory, sizeof (retro_base_directory), "%s", dir);
}

void
retro_deinit (void)
{
}

void
retro_set_controller_port_device (unsigned port, unsigned device)
{
  log_cb (RETRO_LOG_INFO, "Port %u device %u\n", port, device);
}

void
retro_get_system_info (struct retro_system_info *info)
{
  memset (info, 0, sizeof (*info));
  info->library_name = "TinyMSX (MSX)";
  info->library_version = "0.2";
  info->need_fullpath = false;
  info->valid_extensions = "rom";
}

void
retro_get_system_av_info (struct retro_system_av_info *info)
{
  info->timing.fps = 60.0;
  info->timing.sample_rate = 44100.0;

  info->geometry.base_width = VIDEO_WIDTH;
  info->geometry.base_height = VIDEO_HEIGHT;
  info->geometry.max_width = VIDEO_WIDTH;
  info->geometry.max_height = VIDEO_HEIGHT;
  info->geometry.aspect_ratio = 4.0f / 3.0f;
}

void
retro_set_video_refresh (retro_video_refresh_t cb)
{
  video_cb = cb;
}

void
retro_set_audio_sample (retro_audio_sample_t cb)
{
  (void) cb;
}

void
retro_set_audio_sample_batch (retro_audio_sample_batch_t cb)
{
  audio_batch_cb = cb;
}

void
retro_set_input_poll (retro_input_poll_t cb)
{
  input_poll_cb = cb;
}

void
retro_set_input_state (retro_input_state_t cb)
{
  input_state_cb = cb;
}

void
retro_reset (void)
{
  tinymsx_destroy ();
}

static uint8_t pad1 = 0;
static uint8_t pad2 = 0;

static void
update_input (void)
{
  input_poll_cb ();

  pad1 = 0;
  pad2 = 0;

  if (input_state_cb (0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP))
    pad1 |= TINYMSX_JOY_UP;

  if (input_state_cb (0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN))
    pad1 |= TINYMSX_JOY_DW;

  if (input_state_cb (0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT))
    pad1 |= TINYMSX_JOY_LE;

  if (input_state_cb
      (0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT))
    pad1 |= TINYMSX_JOY_RI;

  if (input_state_cb (0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A))
    pad1 |= TINYMSX_JOY_T1;

  if (input_state_cb (0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B))
    pad1 |= TINYMSX_JOY_T2;

  if (input_state_cb
      (0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START))
    pad1 |= TINYMSX_JOY_S1;

  if (input_state_cb
      (0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT))
    pad1 |= TINYMSX_JOY_S2;
}


void
retro_run (void)
{
  update_input ();

  tinymsx_run_frame (pad1, pad2);

  video_cb (tinymsx_video (), VIDEO_WIDTH, VIDEO_HEIGHT, VIDEO_PITCH);

  size_t audio_size = 0;
  const int16_t *audio = (const int16_t *) tinymsx_audio (&audio_size);

  if (audio && audio_size)
    audio_batch_cb (audio, audio_size / 4);
}


bool
retro_load_game (const struct retro_game_info *info)
{
  enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_0RGB1555;
  if (!environ_cb (RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
    return false;

  rom_size = info->size;
  rom_data = malloc (rom_size);
  memcpy (rom_data, info->data, rom_size);

  char bios_path[4096];
  snprintf (bios_path, sizeof (bios_path), "%s/msx1.rom",
	    retro_base_directory);

  FILE *f = fopen (bios_path, "rb");
  if (!f)
    {
      log_cb (RETRO_LOG_ERROR, "Failed to load BIOS: %s\n", bios_path);
      return false;
    }
  fseek (f, 0, SEEK_END);
  size_t bios_size = ftell (f);
  fseek (f, 0, SEEK_SET);
  void *bios_data = malloc (bios_size);
  fread (bios_data, 1, bios_size, f);
  fclose (f);

tinymsx_create(
    rom_data,
    rom_size,
    bios_data,
    bios_size,
    0x8000,
    tinymsx_type
);
	
  free (bios_data);

  static const struct retro_input_descriptor desc[] = {
    {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP, "Up"},
    {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN, "Down"},
    {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT, "Left"},
    {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "Right"},
    {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A, "Button 1"},
    {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B, "Button 2"},
    {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "Start"},
    {0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Select"},
    {0},
  };
  environ_cb (RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, (void *) desc);

  return true;
}

void
retro_unload_game (void)
{
  tinymsx_destroy ();
  free (rom_data);
  rom_data = NULL;
  rom_size = 0;
}


unsigned
retro_get_region (void)
{
  return RETRO_REGION_NTSC;
}

bool
retro_load_game_special (unsigned type,
			 const struct retro_game_info *info, size_t num)
{
  (void) type;
  (void) info;
  (void) num;
  return false;
}


size_t retro_serialize_size(void)
{
    if (!rom_data)
        return 0;

    size_t size = 0;
    tinymsx_save_state(NULL, &size);
    return size;
}

bool retro_serialize(void *data, size_t size)
{
    if (!rom_data || !data || size == 0)
        return false;

    size_t needed_size = 0;
    tinymsx_save_state(data, &needed_size);
    return needed_size == size;
}

bool retro_unserialize(const void *data, size_t size)
{
    if (!rom_data || !data || size == 0)
        return false;

    return tinymsx_load_state(data, size);
}


void *
retro_get_memory_data (unsigned id)
{
  (void) id;
  return NULL;
}

size_t
retro_get_memory_size (unsigned id)
{
  (void) id;
  return 0;
}

void
retro_cheat_reset (void)
{
}

void
retro_cheat_set (unsigned index, bool enabled, const char *code)
{
  (void) index;
  (void) enabled;
  (void) code;
}
