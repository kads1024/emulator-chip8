#pragma once
#include <cstdint>

#include "SDL.h"

class app
{
public:
    app(char const *title, int windowWidth, int windowHeight, int textureWidth, int textureHeight)
    {
        SDL_Init(SDL_INIT_VIDEO);

        window = SDL_CreateWindow(title, windowWidth, windowHeight, 0);

        renderer = SDL_CreateRenderer(window, nullptr);

        texture = SDL_CreateTexture(
            renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, textureWidth, textureHeight);

        SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
    }

    ~app()
    {
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }

    void Update(void const *buffer, int pitch)
    {
        SDL_UpdateTexture(texture, nullptr, buffer, pitch);
        SDL_RenderClear(renderer);
        SDL_RenderTexture(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);
    }

    bool ProcessInput(uint8_t *keys)
    {
        bool quit = false;

        SDL_Event event;

        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_EVENT_QUIT:
                quit = true;
                break;

            case SDL_EVENT_KEY_DOWN:
            case SDL_EVENT_KEY_UP:
            {
                if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE)
                {
                    quit = true;
                    break;
                }

                uint8_t value = (event.type == SDL_EVENT_KEY_DOWN) ? 1 : 0;

                switch (event.key.key)
                {
                case SDLK_X:
                    keys[0x0] = value;
                    break;
                case SDLK_1:
                    keys[0x1] = value;
                    break;
                case SDLK_2:
                    keys[0x2] = value;
                    break;
                case SDLK_3:
                    keys[0x3] = value;
                    break;
                case SDLK_Q:
                    keys[0x4] = value;
                    break;
                case SDLK_W:
                    keys[0x5] = value;
                    break;
                case SDLK_E:
                    keys[0x6] = value;
                    break;
                case SDLK_A:
                    keys[0x7] = value;
                    break;
                case SDLK_S:
                    keys[0x8] = value;
                    break;
                case SDLK_D:
                    keys[0x9] = value;
                    break;
                case SDLK_Z:
                    keys[0xA] = value;
                    break;
                case SDLK_C:
                    keys[0xB] = value;
                    break;
                case SDLK_4:
                    keys[0xC] = value;
                    break;
                case SDLK_R:
                    keys[0xD] = value;
                    break;
                case SDLK_F:
                    keys[0xE] = value;
                    break;
                case SDLK_V:
                    keys[0xF] = value;
                    break;
                default:
                    break;
                }
            }
            break;

            default:
                break;
            }
        }

        return quit;
    }

private:
    SDL_Window *window{};
    SDL_Renderer *renderer{};
    SDL_Texture *texture{};
};