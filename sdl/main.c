// create by wuyulun, 2011.12.24

#pragma comment(lib, "SDL.lib")
#pragma comment(lib, "SDLmain.lib")
#pragma comment(lib, "SDL_ttf.lib")
#pragma comment(lib, "SDL_net.lib")
#pragma comment(lib, "SDL_image.lib")
#pragma comment(lib, "zlib.lib")

#include "nbk_conf.h"
#include "../stdc/inc/config.h"
#include <stdio.h>
#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_net.h>
#include <SDL_image.h>
#include "nbk.h"
#include "runtime.h"
#include "ini.h"

#ifdef PLATFORM_WEBOS
	#include "PDL.h"
	#include "glutil.h"
#endif

static SDL_Surface* dev = NULL;

void test(NBK_core* nbk)
{
}

static void event_loop(NBK_core* nbk)
{
	nbool over = N_FALSE;
	SDL_Event evt;

	while (!over) {

		if (SDL_PollEvent(&evt)) {
			if (evt.type == SDL_QUIT)
				over = N_TRUE;
			else
				nbk_handleEvent(nbk, evt);
        }
        else {
            timerMgr_run(nbk->timerMgr);
        }

		nbk_display(nbk, dev);
        
        SDL_Delay(20);
	}
}

int main(int argc, char** argv)
{
	NBK_core* nbk;
	SDL_Surface* canvas; // 绘制页面

	ini_init();

	if (!nbk_pathExist(ini_getString(NEINI_TTF_FONT))) {
		fprintf(stderr, "Font: %s not found!", ini_getString(NEINI_TTF_FONT));
		ini_end();
		return 1;
	}
    else
        fprintf(stderr, "Use font: %s\n", ini_getString(NEINI_TTF_FONT));

	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
    SDL_EnableUNICODE(1);
	TTF_Init();
	SDLNet_Init();

#ifdef PLATFORM_WEBOS
	PDL_Init(0);
#else
	IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);
#endif

	runtime_init();

#ifdef PLATFORM_WEBOS
	dev = gl_init();
	canvas = SDL_CreateRGBSurface(SDL_SWSURFACE, dev->w, dev->h, 32, 0xff, 0xff00, 0xff0000, 0xff000000);
	dev = SDL_CreateRGBSurface(SDL_SWSURFACE, canvas->w, canvas->h, 32, 0xff, 0xff00, 0xff0000, 0xff000000);
#else
	dev = SDL_SetVideoMode(ini_getInt(NEINI_WIDTH), ini_getInt(NEINI_HEIGHT), 0, SDL_HWSURFACE);
	canvas = SDL_CreateRGBSurface(SDL_HWSURFACE, dev->w, dev->h, 32, 0xff, 0xff00, 0xff0000, 0xff000000);
#endif

	nbk = nbk_create();
	nbk_setScreen(nbk, canvas);

	nbk_updateCaption(nbk);
	nbk_init(nbk);
	nbk_loadUrl(nbk, ini_getString(NEINI_HOMEPAGE));
    //test(nbk);
	event_loop(nbk);

	nbk_delete(&nbk);
	SDL_FreeSurface(canvas);

#ifdef PLATFORM_WEBOS
    SDL_FreeSurface(dev);
	gl_end();
	PDL_Quit();
#else
	IMG_Quit();
#endif
	SDLNet_Quit();
	TTF_Quit();
    SDL_Quit();

	ini_end();
	runtime_end();

	return 0;
}
