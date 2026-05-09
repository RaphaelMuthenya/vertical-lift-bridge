#include <SDL2/SDL.h>
#include <lvgl.h>
#include <unistd.h>
#include <stdio.h>
#include "system_types.h"

// --- Mock Shared Status ---
SharedStatus_t g_status = {};

static SDL_Window * window = NULL;
static SDL_Renderer * renderer = NULL;
static SDL_Texture * texture = NULL;

static void hal_init(void);
static void sdl_flush_cb(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p);
static void sdl_mouse_read_cb(lv_indev_drv_t * indev_drv, lv_indev_data_t * data);

void ui_init(void);
void ui_tick(void);

int main(int argc, char** argv) {
    lv_init();
    hal_init();

    g_status.state = STATE_IDLE;
    g_status.hmi_brightness = 80;
    g_status.deck_position_mm = 0;
    g_status.motor_current_ma = 0;

    ui_init();

    bool quit = false;
    SDL_Event event;

    while (!quit) {
        while(SDL_PollEvent(&event)) {
            if(event.type == SDL_QUIT) quit = true;
        }
        
        lv_timer_handler();
        ui_tick();
        usleep(5000); // 5ms
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

static void hal_init(void) {
    SDL_Init(SDL_INIT_VIDEO);
    window = SDL_CreateWindow("VLB Simulation", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 320, 240, 0);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING, 320, 240);

    static lv_disp_draw_buf_t draw_buf;
    static lv_color_t buf[320 * 240 / 10];
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, 320 * 240 / 10);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.draw_buf = &draw_buf;
    disp_drv.flush_cb = sdl_flush_cb;
    disp_drv.hor_res = 320;
    disp_drv.ver_res = 240;
    lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = sdl_mouse_read_cb;
    lv_indev_drv_register(&indev_drv);
}

static void sdl_flush_cb(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p) {
    SDL_Rect rect;
    rect.x = area->x1;
    rect.y = area->y1;
    rect.w = area->x2 - area->x1 + 1;
    rect.h = area->y2 - area->y1 + 1;
    SDL_UpdateTexture(texture, &rect, color_p, rect.w * sizeof(lv_color_t));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
    lv_disp_flush_ready(disp_drv);
}

static void sdl_mouse_read_cb(lv_indev_drv_t * indev_drv, lv_indev_data_t * data) {
    int x, y;
    uint32_t buttons = SDL_GetMouseState(&x, &y);
    data->point.x = x;
    data->point.y = y;
    data->state = (buttons & SDL_BUTTON_LMASK) ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
}

void ui_tick() {
    static uint32_t last_tick = 0;
    if (SDL_GetTicks() - last_tick < 50) return;
    last_tick = SDL_GetTicks();

    g_status.uptime_ms = last_tick;

    // Simulate vehicle presence randomly, but hold it so it's visible
    static uint32_t last_vision_change = 0;
    if (last_tick - last_vision_change > 2500) {
        g_status.vision.vehicle_present = (rand() % 100 > 70); // 30% chance of vehicle
        last_vision_change = last_tick;
    }

    if(g_status.vision.vehicle_present) {
        g_status.vision.confidence = 80 + (rand() % 20);
    } else {
        g_status.vision.confidence = 0;
    }

    // Ultrasonic Simulation (Quad Sensor Direction Inference)
    // We mock a boat passing by every ~15 seconds
    static uint32_t last_boat_time = 0;
    static bool boat_in_zone = false;
    static VehicleDirection_t mock_direction = DIR_NONE;

    if (!boat_in_zone && last_tick - last_boat_time > 15000) {
        boat_in_zone = true;
        mock_direction = (rand() % 2 == 0) ? DIR_APPROACHING : DIR_LEAVING;
        g_status.ultrasonic.upstream_direction = mock_direction;
        g_status.ultrasonic.downstream_direction = DIR_NONE;
    }

    if (boat_in_zone) {
        g_status.ultrasonic.distance_us1_cm = 50 + (rand() % 5);
        g_status.ultrasonic.distance_us2_cm = 50 + (rand() % 5);
        g_status.ultrasonic.vessel_approaching = (mock_direction == DIR_APPROACHING);
        
        // Boat leaves zone after 5 seconds
        if (last_tick - last_boat_time > 20000) {
            boat_in_zone = false;
            last_boat_time = last_tick;
            g_status.ultrasonic.upstream_direction = DIR_NONE;
            g_status.ultrasonic.vessel_approaching = false;
            g_status.ultrasonic.distance_us1_cm = 400;
            g_status.ultrasonic.distance_us2_cm = 400;
        }
    } else {
        g_status.ultrasonic.distance_us1_cm = 400;
        g_status.ultrasonic.distance_us2_cm = 400;
        g_status.ultrasonic.vessel_approaching = false;
    }

    // Automated Mode Logic (Triggers on approaching vessel)
    if (g_status.auto_mode_active && g_status.state == STATE_IDLE) {
        if (g_status.ultrasonic.vessel_approaching) {
            g_status.state = STATE_ROAD_CLEARING;
        }
    }

    // Road Clearing Logic (Wait for no vehicles before raising)
    if (g_status.state == STATE_ROAD_CLEARING) {
        if (!g_status.vision.vehicle_present) {
            g_status.state = STATE_RAISING;
        }
    } else if (g_status.state == STATE_RAISING) {
        if (g_status.deck_position_mm < DECK_HEIGHT_MAX_MM) {
            g_status.deck_position_mm += 2;
            
            // Inject random high current spikes for demo purposes
            int rand_spike = rand() % 100;
            if (rand_spike > 98) g_status.motor_current_ma = 2100; // CRITICAL
            else if (rand_spike > 90) g_status.motor_current_ma = 1700; // WARNING
            else g_status.motor_current_ma = 650 + (rand() % 50); // OK

            // Mock pump active while raising
            g_status.counterweight.left.pump_active = true;
            g_status.counterweight.right.pump_active = true;
            g_status.counterweight.left.drain_open = false;
            g_status.counterweight.right.drain_open = false;
            if(g_status.counterweight.left.water_level_ml < CW_TANK_CAPACITY_ML) g_status.counterweight.left.water_level_ml += 1.0f;
            if(g_status.counterweight.right.water_level_ml < CW_TANK_CAPACITY_ML) g_status.counterweight.right.water_level_ml += 1.0f;

        } else {
            g_status.state = STATE_RAISED_HOLD;
            g_status.top_limit_hit = true;
            g_status.motor_current_ma = 0;
            g_status.counterweight.left.pump_active = false;
            g_status.counterweight.right.pump_active = false;
            g_status.state_entered_ms = last_tick; // Record entry time for timeout
        }
    } else if (g_status.state == STATE_RAISED_HOLD) {
        // Automatically timeout after 8 seconds (HOLD_TIMEOUT_MS)
        if (last_tick - g_status.state_entered_ms > 8000) {
            g_status.state = STATE_LOWERING;
        }
    } else if (g_status.state == STATE_LOWERING) {
        if (g_status.deck_position_mm > 0) {
            g_status.deck_position_mm -= 3;
            if (g_status.deck_position_mm < 0) g_status.deck_position_mm = 0;
            g_status.motor_current_ma = 400 + (rand() % 30);

            // Mock drain open while lowering
            g_status.counterweight.left.pump_active = false;
            g_status.counterweight.right.pump_active = false;
            g_status.counterweight.left.drain_open = true;
            g_status.counterweight.right.drain_open = true;
            if(g_status.counterweight.left.water_level_ml > 0.0f) g_status.counterweight.left.water_level_ml -= 1.5f;
            if(g_status.counterweight.right.water_level_ml > 0.0f) g_status.counterweight.right.water_level_ml -= 1.5f;

        } else {
            g_status.state = STATE_ROAD_OPENING; // Bridge fully down, opening barriers
            g_status.bottom_limit_hit = true;
            g_status.motor_current_ma = 0;
            g_status.counterweight.left.drain_open = false;
            g_status.counterweight.right.drain_open = false;
            // Hacky timer for mock
            g_status.state_entered_ms = last_tick;
        }
    } else if (g_status.state == STATE_ROAD_OPENING) {
        if (last_tick - g_status.state_entered_ms > 2000) {
            g_status.state = STATE_IDLE; // Reset to idle (Green light)
        }
    } else {
        g_status.motor_current_ma = 0;
        g_status.counterweight.left.pump_active = false;
        g_status.counterweight.right.pump_active = false;
        g_status.counterweight.left.drain_open = false;
        g_status.counterweight.right.drain_open = false;
    }
}