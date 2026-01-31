#include <SDL3/SDL.h>
#include <fstream>
#include <vector>
#include "tinymsx.h"

uint8_t pad1 = 0;
uint8_t pad2 = 0;

std::vector < uint8_t > loadFile (const std::string & path)
{
  std::ifstream file (path, std::ios::binary);
  return std::vector < uint8_t > (std::istreambuf_iterator < char >(file),
				  {
				  }
  );
}

int
main (int argc, char *argv[])
{
  const int SCREEN_WIDTH = 284;
  const int SCREEN_HEIGHT = 240;
  const int SCALE = 1;

  if (SDL_Init (SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD) < 0)
    {
      SDL_Log ("SDL_Init failed: %s", SDL_GetError ());
      return 1;
    }

  SDL_Window *window = nullptr;
  SDL_Renderer *renderer = nullptr;

  if (!SDL_CreateWindowAndRenderer ("tinymsx-emu",
				    SCREEN_WIDTH * SCALE,
				    SCREEN_HEIGHT * SCALE,
				    SDL_WINDOW_FULLSCREEN,
				    &window, &renderer))
    {
      SDL_Log ("CreateWindowAndRenderer failed: %s", SDL_GetError ());
      return 1;
    }

  SDL_Texture *texture = SDL_CreateTexture (renderer,
					    SDL_PIXELFORMAT_XRGB1555,
					    SDL_TEXTUREACCESS_STREAMING,
					    SCREEN_WIDTH,
					    SCREEN_HEIGHT);

  SDL_AudioSpec spec
  {
  };
  spec.freq = 44100;
  spec.format = SDL_AUDIO_S16;
  spec.channels = 2;

  SDL_AudioDeviceID audioDevice =
    SDL_OpenAudioDevice (SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec);

  if (!audioDevice)
    {
      SDL_Log ("Audio open failed: %s", SDL_GetError ());
      return 1;
    }

  SDL_AudioStream *audioStream = SDL_CreateAudioStream (&spec, &spec);

  SDL_BindAudioStream (audioDevice, audioStream);
  SDL_ResumeAudioDevice (audioDevice);

  auto bios = loadFile ("bios/msx1.rom");
  auto rom = loadFile ("roms/demo.rom");

  TinyMSX msx (TINYMSX_TYPE_MSX1_ASC8,
	       rom.data (), rom.size (), 0x8000, TINYMSX_COLOR_MODE_RGB555);

  msx.loadBiosFromMemory (bios.data (), bios.size ());
  msx.reset ();

  std::vector < SDL_Gamepad * >controllers;

  int padCount = 0;
  SDL_JoystickID *padIDs = SDL_GetGamepads (&padCount);

  for (int i = 0; i < padCount; ++i)
    {
      SDL_Gamepad *pad = SDL_OpenGamepad (padIDs[i]);
      if (pad)
	controllers.push_back (pad);
    }

  SDL_free (padIDs);

  bool running = true;
  SDL_Event event;

  while (running)
    {
      while (SDL_PollEvent (&event))
	{
	  if (event.type == SDL_EVENT_QUIT)
	    running = false;
	}

      pad1 = 0;

      bool up = false, down = false, left = false, right = false;
      bool t1 = false, t2 = false, s1 = false, s2 = false;

      const bool *state = SDL_GetKeyboardState (nullptr);

      up |= state[SDL_SCANCODE_UP];
      down |= state[SDL_SCANCODE_DOWN];
      left |= state[SDL_SCANCODE_LEFT];
      right |= state[SDL_SCANCODE_RIGHT];
      t1 |= state[SDL_SCANCODE_X];
      t2 |= state[SDL_SCANCODE_Z];
      s1 |= state[SDL_SCANCODE_RETURN];
      s2 |= state[SDL_SCANCODE_BACKSPACE];

      if (!controllers.empty ())
	{
	  SDL_Gamepad *c = controllers[0];

	  up |= SDL_GetGamepadButton (c, SDL_GAMEPAD_BUTTON_DPAD_UP);
	  down |= SDL_GetGamepadButton (c, SDL_GAMEPAD_BUTTON_DPAD_DOWN);
	  left |= SDL_GetGamepadButton (c, SDL_GAMEPAD_BUTTON_DPAD_LEFT);
	  right |= SDL_GetGamepadButton (c, SDL_GAMEPAD_BUTTON_DPAD_RIGHT);
	  t1 |= SDL_GetGamepadButton (c, SDL_GAMEPAD_BUTTON_SOUTH);
	  t2 |= SDL_GetGamepadButton (c, SDL_GAMEPAD_BUTTON_EAST);
	  s1 |= SDL_GetGamepadButton (c, SDL_GAMEPAD_BUTTON_START);
	  s2 |= SDL_GetGamepadButton (c, SDL_GAMEPAD_BUTTON_BACK);
	}

      if (up)
	pad1 |= TINYMSX_JOY_UP;
      if (down)
	pad1 |= TINYMSX_JOY_DW;
      if (left)
	pad1 |= TINYMSX_JOY_LE;
      if (right)
	pad1 |= TINYMSX_JOY_RI;
      if (t1)
	pad1 |= TINYMSX_JOY_T1;
      if (t2)
	pad1 |= TINYMSX_JOY_T2;
      if (s1)
	pad1 |= TINYMSX_JOY_S1;
      if (s2)
	pad1 |= TINYMSX_JOY_S2;

      msx.tick (pad1, pad2);

      unsigned short *video = msx.getDisplayBuffer ();

      SDL_UpdateTexture (texture,
			 nullptr, video, SCREEN_WIDTH * sizeof (uint16_t));

      SDL_RenderClear (renderer);
      SDL_RenderTexture (renderer, texture, nullptr, nullptr);
      SDL_RenderPresent (renderer);

      size_t audioSize = 0;
      void *audioBuffer = msx.getSoundBuffer (&audioSize);

      if (audioSize > 0)
	SDL_PutAudioStreamData (audioStream, audioBuffer, audioSize);

      SDL_Delay (16);
    }

  SDL_DestroyAudioStream (audioStream);
  SDL_CloseAudioDevice (audioDevice);

for (auto c:controllers)
    SDL_CloseGamepad (c);

  SDL_DestroyTexture (texture);
  SDL_DestroyRenderer (renderer);
  SDL_DestroyWindow (window);
  SDL_Quit ();

  return 0;
}
