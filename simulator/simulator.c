#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <stdio.h>
#include <string.h>

#include "handheld_ui.h"

enum { WIDTH = 240, HEIGHT = 320 };
static SDL_Texture *texture;
static SDL_Renderer *renderer;
static bool render_ok = true;
static bool dirty;
static unsigned saves;
static bool save_enabled = true;
static struct handheld_preferences saved = {.brightness = 75};

static bool save_preferences(struct handheld_preferences prefs) {
    if (!save_enabled) return false;
    saved = prefs;
    saves++;
    return true;
}

static void flush(lv_disp_drv_t *driver, const lv_area_t *area, lv_color_t *pixels) {
    const SDL_Rect rect = {area->x1, area->y1, area->x2 - area->x1 + 1, area->y2 - area->y1 + 1};
    if (SDL_UpdateTexture(texture, &rect, pixels, rect.w * (int)sizeof(*pixels)) != 0) {
        render_ok = false;
    }
    dirty = true;
    lv_disp_flush_ready(driver);
}

static void pointer_read(lv_indev_drv_t *driver, lv_indev_data_t *data) {
    (void)driver;
    int x, y;
    const Uint32 buttons = SDL_GetMouseState(&x, &y);
    float lx, ly;
    SDL_RenderWindowToLogical(renderer, x, y, &lx, &ly);
    data->point.x = (lv_coord_t)lx;
    data->point.y = (lv_coord_t)ly;
    data->state = buttons & SDL_BUTTON_LMASK ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

static bool frame(void) {
    lv_refr_now(NULL);
    return render_ok && SDL_RenderClear(renderer) == 0 &&
           SDL_RenderCopy(renderer, texture, NULL, NULL) == 0;
}

static bool snapshot(const char *name) {
    if (!frame()) return false;
    if (!name) return true;
    SDL_Surface *surface =
        SDL_CreateRGBSurfaceWithFormat(0, WIDTH, HEIGHT, 32, SDL_PIXELFORMAT_RGBA32);
    if (!surface) return false;
    const bool ok = SDL_RenderReadPixels(renderer, NULL, surface->format->format, surface->pixels,
                                         surface->pitch) == 0 &&
                    SDL_SaveBMP(surface, name) == 0;
    SDL_FreeSurface(surface);
    return ok;
}

static bool has_label(lv_disp_t *display, const char *text) {
    lv_obj_t *screen = lv_disp_get_scr_act(display);
    for (uint32_t i = 0; i < lv_obj_get_child_cnt(screen); i++) {
        lv_obj_t *child = lv_obj_get_child(screen, (int32_t)i);
        if (lv_obj_check_type(child, &lv_label_class) &&
            strcmp(lv_label_get_text(child), text) == 0)
            return true;
    }
    return false;
}

static struct dmx_monitor sample_dmx;
static bool sample_signal = true;

static enum dmx_status sample_dmx_read(struct dmx_snapshot *snapshot) {
    *snapshot = sample_dmx.latest;
    return dmx_snapshot_status(snapshot, SDL_GetTicks());
}

static void sample_dmx_send(uint32_t now) {
    uint8_t frame[DMX_FRAME_BYTES] = {0};
    for (unsigned i = 0; i < DMX_SLOTS; i++)
        frame[i + 1] = (uint8_t)(i * 17 + 32);

    // Model a complete UART packet whose last chunk arrives after the break.
    const size_t chunk_size = 122;
    size_t offset = 0;
    while (sizeof(frame) - offset > chunk_size) {
        dmx_on_data(&sample_dmx, frame + offset, chunk_size, false, now);
        offset += chunk_size;
    }
    dmx_on_break(&sample_dmx, now);
    dmx_on_data(&sample_dmx, frame + offset, sizeof(frame) - offset, true, now);
}

static int smoke(lv_disp_t *display, bool capture) {
#define CHECK(cond)                                                                                \
    do {                                                                                           \
        if (!(cond)) {                                                                             \
            fprintf(stderr, "Smoke check failed: %s\n", #cond);                                    \
            return 1;                                                                              \
        }                                                                                          \
    } while (0)
    CHECK(handheld_ui_page() == PAGE_SETTINGS);
    CHECK(snapshot(capture ? "settings.bmp" : NULL));
    handheld_ui_key(KEY_ENTER, "Enter");
    CHECK(handheld_ui_page() == PAGE_DEVICE);
    handheld_ui_key(KEY_ENTER, "Enter");
    CHECK(saved.brightness == 100 && saves == 1);
    save_enabled = false;
    handheld_ui_key(KEY_ENTER, "Enter");
    CHECK(saved.brightness == 100 && saves == 1);
    CHECK(has_label(display, "Settings could not be saved"));
    save_enabled = true;
    handheld_ui_key(KEY_ENTER, "Enter");
    CHECK(saved.brightness == 25 && saves == 2);
    CHECK(snapshot(capture ? "device.bmp" : NULL));
    handheld_ui_key(KEY_DOWN, "Down");
    handheld_ui_key(KEY_ENTER, "Enter");
    CHECK(saved.contrast && saves == 3);
    handheld_ui_key(KEY_ENTER, "Enter");
    CHECK(!saved.contrast && saves == 4);
    handheld_ui_key(KEY_BACK, "Back");
    CHECK(handheld_ui_page() == PAGE_SETTINGS);
    handheld_ui_key(KEY_DOWN, "Down");
    handheld_ui_key(KEY_ENTER, "Enter");
    CHECK(handheld_ui_page() == PAGE_DEV);
    handheld_ui_key(KEY_OTHER, "Left");
    CHECK(has_label(display, "Left  |  1 press"));
    CHECK(has_label(display, "RTC: 12:34:56 (mock)"));
    CHECK(snapshot(capture ? "dev-menu.bmp" : NULL));
    handheld_ui_status(false, "unavailable");
    CHECK(has_label(display, "Keypad: unavailable"));
    CHECK(has_label(display, "RTC: unavailable"));
    handheld_ui_key(KEY_ENTER, "Enter");
    CHECK(handheld_ui_page() == PAGE_DMX);
    CHECK(has_label(display, "Receiving  |  512 slots"));
    CHECK(has_label(display, "001:032"));
    CHECK(snapshot(capture ? "dmx-monitor.bmp" : NULL));
    handheld_ui_key(KEY_ENTER, "Enter");
    CHECK(has_label(display, "017:048"));
    handheld_ui_key(KEY_UP, "Up");
    handheld_ui_key(KEY_ENTER, "Enter");
    CHECK(has_label(display, "001:032"));
    sample_dmx.latest.last_frame_ms = SDL_GetTicks() - 1000;
    handheld_ui_key(KEY_ENTER, "Enter");
    CHECK(has_label(display, "Signal idle  |  512 slots"));
    CHECK(has_label(display, "497:016"));
    handheld_ui_key(KEY_BACK, "Back");
    CHECK(handheld_ui_page() == PAGE_DEV);
    handheld_ui_key(KEY_BACK, "Back");
    handheld_ui_key(KEY_DOWN, "Down");
    handheld_ui_key(KEY_ENTER, "Enter");
    CHECK(handheld_ui_page() == PAGE_ABOUT && has_label(display, "SDL2 simulator"));
    puts("Settings, persistence callback, diagnostics, DMX monitor, and About passed.");
    return 0;
#undef CHECK
}

static void key(SDL_Keycode code) {
    switch (code) {
    case SDLK_UP:
        handheld_ui_key(KEY_UP, "Up");
        break;
    case SDLK_DOWN:
        handheld_ui_key(KEY_DOWN, "Down");
        break;
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
        handheld_ui_key(KEY_ENTER, "Enter");
        break;
    case SDLK_SPACE:
        if (handheld_ui_page() == PAGE_DMX) {
            sample_signal = !sample_signal;
            break;
        }
        handheld_ui_key(KEY_OTHER, "Space");
        break;
    case SDLK_ESCAPE:
    case SDLK_BACKSPACE:
        handheld_ui_key(KEY_BACK, "Back");
        break;
    default:
        handheld_ui_key(KEY_OTHER, SDL_GetKeyName(code));
        break;
    }
}

int main(int argc, char **argv) {
    const bool capture = argc == 2 && strcmp(argv[1], "--capture") == 0;
    const bool automated = capture || (argc == 2 && strcmp(argv[1], "--smoke") == 0);
    if (argc > 1 && !automated) {
        fprintf(stderr, "Usage: %s [--smoke | --capture]\n", argv[0]);
        return 2;
    }
    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL: %s\n", SDL_GetError());
        return 1;
    }
    const int scale = automated ? 1 : 2;
    SDL_Window *window = SDL_CreateWindow("Handheld Reference", SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED, WIDTH * scale, HEIGHT * scale,
                                          automated ? SDL_WINDOW_HIDDEN : SDL_WINDOW_RESIZABLE);
    int result = 1;
    if (!window) goto done;
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer || SDL_RenderSetLogicalSize(renderer, WIDTH, HEIGHT) != 0) goto done;
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING,
                                WIDTH, HEIGHT);
    if (!texture) goto done;

    lv_init();
    static lv_color_t pixels[WIDTH * 80];
    static lv_disp_draw_buf_t buffer;
    static lv_disp_drv_t driver;
    lv_disp_draw_buf_init(&buffer, pixels, NULL, WIDTH * 80);
    lv_disp_drv_init(&driver);
    driver.hor_res = WIDTH;
    driver.ver_res = HEIGHT;
    driver.draw_buf = &buffer;
    driver.flush_cb = flush;
    lv_disp_t *display = lv_disp_drv_register(&driver);
    static lv_indev_drv_t pointer;
    lv_indev_drv_init(&pointer);
    pointer.type = LV_INDEV_TYPE_POINTER;
    pointer.read_cb = pointer_read;
    lv_indev_drv_register(&pointer);
    static const struct handheld_platform platform = {
        .target = "SDL2 simulator",
        .version = "0.1.0",
        .save = save_preferences,
        .read_dmx = sample_dmx_read,
        .dmx_source = "Simulated input / Space to pause",
    };
    handheld_ui_start(display, &platform, saved);
    handheld_ui_status(true, "12:34:56 (mock)");
    dmx_on_break(&sample_dmx, SDL_GetTicks());
    dmx_finish_break(&sample_dmx, SDL_GetTicks());
    sample_dmx_send(SDL_GetTicks());
    if (automated) {
        result = smoke(display, capture);
        goto done;
    }

    bool running = true;
    Uint32 tick = SDL_GetTicks();
    Uint32 last_sample = tick;
    while (running && render_ok) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT)
                running = false;
            else if (event.type == SDL_WINDOWEVENT)
                dirty = true;
            else if (event.type == SDL_KEYDOWN && !event.key.repeat)
                key(event.key.keysym.sym);
        }
        const Uint32 now = SDL_GetTicks();
        if (sample_signal && now - last_sample >= 25) {
            sample_dmx_send(now);
            last_sample = now;
        }
        lv_tick_inc(now - tick);
        tick = now;
        lv_timer_handler();
        if (dirty) {
            if (SDL_RenderClear(renderer) != 0 ||
                SDL_RenderCopy(renderer, texture, NULL, NULL) != 0) {
                render_ok = false;
            } else {
                SDL_RenderPresent(renderer);
            }
            dirty = false;
        }
        SDL_Delay(5);
    }
    result = render_ok ? 0 : 1;
done:
    if (result && *SDL_GetError()) fprintf(stderr, "SDL: %s\n", SDL_GetError());
    if (texture) SDL_DestroyTexture(texture);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
    return result;
}
