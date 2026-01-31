#include <SDL2/SDL.h>
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
				  });
}

int
main (int argc, char *argv[])
{
  const int SCREEN_WIDTH = 284;
  const int SCREEN_HEIGHT = 240;
  const int SCALE = 2;

  SDL_Init (SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER);
  SDL_Window *window = SDL_CreateWindow ("tinymsx-emu",
					 SDL_WINDOWPOS_CENTERED,
					 SDL_WINDOWPOS_CENTERED,
					 SCREEN_WIDTH * SCALE,
					 SCREEN_HEIGHT * SCALE,
					 SDL_WINDOW_FULLSCREEN);

  SDL_Renderer *renderer =
    SDL_CreateRenderer (window, -1, SDL_RENDERER_ACCELERATED);
  SDL_Texture *texture = SDL_CreateTexture (renderer,
					    SDL_PIXELFORMAT_RGB555,
					    SDL_TEXTUREACCESS_STREAMING,
					    SCREEN_WIDTH, SCREEN_HEIGHT);

  SDL_AudioSpec want = { }, have;
  want.freq = 44100;
  want.format = AUDIO_S16;
  want.channels = 2;
  want.samples = 1024;
  want.callback = nullptr;

  SDL_AudioDeviceID audioDevice =
    SDL_OpenAudioDevice (nullptr, 0, &want, &have, 0);
  SDL_PauseAudioDevice (audioDevice, 0);

  auto bios = loadFile ("bios/msx1.rom");
  auto rom = loadFile ("roms/demo.rom");

  TinyMSX msx (TINYMSX_TYPE_MSX1, rom.data (), rom.size (), 0x8000,
	       TINYMSX_COLOR_MODE_RGB555);
  msx.loadBiosFromMemory (bios.data (), bios.size ());
  msx.reset ();

  std::vector < SDL_GameController * >controllers;
  for (int i = 0; i < SDL_NumJoysticks (); ++i)
    if (SDL_IsGameController (i))
      controllers.push_back (SDL_GameControllerOpen (i));

  bool running = true;
  SDL_Event event;

  while (running)
    {
      while (SDL_PollEvent (&event))
	{
	  if (event.type == SDL_QUIT)
	    running = false;
	}

      pad1 = 0;

      bool up = false, down = false, left = false, right = false;
      bool t1 = false, t2 = false, s1 = false, s2 = false;

      const Uint8 *state = SDL_GetKeyboardState (nullptr);

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
	  SDL_GameController *c = controllers[0];

	  up |=
	    SDL_GameControllerGetButton (c, SDL_CONTROLLER_BUTTON_DPAD_UP);
	  down |=
	    SDL_GameControllerGetButton (c, SDL_CONTROLLER_BUTTON_DPAD_DOWN);
	  left |=
	    SDL_GameControllerGetButton (c, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
	  right |=
	    SDL_GameControllerGetButton (c, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
	  t1 |= SDL_GameControllerGetButton (c, SDL_CONTROLLER_BUTTON_A);
	  t2 |= SDL_GameControllerGetButton (c, SDL_CONTROLLER_BUTTON_B);
	  s1 |= SDL_GameControllerGetButton (c, SDL_CONTROLLER_BUTTON_START);
	  s2 |= SDL_GameControllerGetButton (c, SDL_CONTROLLER_BUTTON_BACK);
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
      SDL_UpdateTexture (texture, nullptr, video,
			 SCREEN_WIDTH * sizeof (uint16_t));
      SDL_RenderClear (renderer);
      SDL_RenderCopy (renderer, texture, nullptr, nullptr);
      SDL_RenderPresent (renderer);

      size_t audioSize = 0;
      void *audioBuffer = msx.getSoundBuffer (&audioSize);
      if (audioSize > 0)
	{
	  SDL_QueueAudio (audioDevice, audioBuffer, audioSize);
	}

      SDL_Delay (16);
    }

  SDL_CloseAudioDevice (audioDevice);
  for (auto c:controllers)
    SDL_GameControllerClose (c);
  SDL_DestroyTexture (texture);
  SDL_DestroyRenderer (renderer);
  SDL_DestroyWindow (window);
  SDL_Quit ();
  return 0;
}
