

#include <SDL2/SDL.h>

#include "esplayer-datasrc.h"

#include "ss4s.h"

static SS4S_Player *player = NULL;

int videoPreroll(int width, int height, int framerate) {
    (void) framerate;
    SS4S_VideoInfo info = {
            .codec = SS4S_VIDEO_H264,
            .width = width,
            .height = height,
    };
    SS4S_VideoOpenResult result = SS4S_PlayerVideoOpen(player, &info);
    if (result == SS4S_VIDEO_OPEN_OK) {
        SS4S_VideoRect src = {0, 0, width, 840 * height / 1080};
        SS4S_VideoRect dst = {0, 0, 1920, 840};
        SS4S_PlayerVideoSetDisplayArea(player, &src, &dst);
    }
    return result;
}

int videoSample(const void *data, size_t size, int flags) {
    SS4S_VideoFeedFlags vflags = 0;
    if (flags & VIDEO_FLAG_FRAME_START) {
        vflags |= SS4S_VIDEO_FEED_DATA_FRAME_START;
    }
    if (flags & VIDEO_FLAG_FRAME_END) {
        vflags |= SS4S_VIDEO_FEED_DATA_FRAME_END;
    }
    if (flags & VIDEO_FLAG_FRAME_KEYFRAME) {
        vflags |= SS4S_VIDEO_FEED_DATA_KEYFRAME;
    }
    return SS4S_PlayerVideoFeed(player, data, size, vflags);
}

void videoEos() {
    SS4S_PlayerVideoClose(player);
}

int audioPreroll(int channels, int sampleFreq) {
    SS4S_AudioInfo info = {
            .codec = SS4S_AUDIO_PCM_S16LE,
            .numOfChannels = channels,
            .sampleRate = sampleFreq,
            .samplesPerFrame = 240,
    };
    return SS4S_PlayerAudioOpen(player, &info);
}

int audioSample(const void *data, size_t size) {
    return SS4S_PlayerAudioFeed(player, data, size);
}

void audioEos() {
    SS4S_PlayerAudioClose(player);
}

void pipelineQuit(int error) {
    SDL_Event quit = {SDL_QUIT};
    SDL_PushEvent(&quit);
}

int main(int argc, char *argv[]) {
    datasrc_init(argc, argv);
    SS4S_Config config = {
            .audioDriver = "ndl-webos4",
            .videoDriver = "ndl-webos4",
            .loggingFunction = SS4S_DefaultLoggingFunction(),
    };

    SDL_Init(SDL_INIT_VIDEO);

    if (SS4S_Init(argc, argv, &config) != 0) {
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("SS4S", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1920, 1080,
                                          SDL_WINDOW_FULLSCREEN);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    SS4S_PostInit(argc, argv);
    player = SS4S_PlayerOpen();
    struct DATASRC_CALLBACKS dscb = {
            .videoPreroll = videoPreroll,
            .videoSample = videoSample,
            .videoEos = videoEos,
            .audioPreroll = audioPreroll,
            .audioSample = audioSample,
            .audioEos = audioEos,
            .pipelineQuit = pipelineQuit,
    };
    datasrc_start(&dscb);
    while (!SDL_QuitRequested()) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_MOUSEBUTTONDOWN) {
                datasrc_stop();
                continue;
            }
        }
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    SS4S_PlayerClose(player);
    player = NULL;

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    datasrc_destroy();

    SS4S_Quit();

    SDL_Quit();

}