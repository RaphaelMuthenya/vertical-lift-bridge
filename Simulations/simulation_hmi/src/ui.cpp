#include <cstdlib>
#include <lvgl.h>
#include "system_types.h"
#include <stdio.h>

extern SharedStatus_t g_status;

// --- UI Components ---
static lv_obj_t *tv;
static lv_obj_t *pg_home, *pg_ops, *pg_vision, *pg_cw, *pg_settings;
static lv_obj_t *header, *lbl_state, *lbl_pos, *bar_pos;
static lv_obj_t *led_road_r, *led_road_y, *led_road_g;
static lv_obj_t *meter_motor;
static lv_meter_indicator_t *indic_motor;

// Vision Components
static lv_obj_t *radar_bg;
static lv_obj_t *target_box;
static lv_obj_t *bar_confidence;
static lv_obj_t *lbl_target;

// CW Components
static lv_obj_t *bar_cw_l, *bar_cw_r;
static lv_obj_t *lbl_cw_l_stat, *lbl_cw_r_stat;

// --- Styles (Light Mode) ---
static lv_style_t style_card;
static lv_style_t style_btn_modern;

void init_styles() {
    lv_style_init(&style_card);
    lv_style_set_bg_color(&style_card, lv_color_white());
    lv_style_set_radius(&style_card, 8);
    lv_style_set_border_width(&style_card, 2);
    lv_style_set_border_color(&style_card, lv_palette_lighten(LV_PALETTE_GREY, 2));
    lv_style_set_shadow_width(&style_card, 10);
    lv_style_set_shadow_color(&style_card, lv_palette_main(LV_PALETTE_GREY));
    lv_style_set_shadow_opa(&style_card, LV_OPA_20);

    lv_style_init(&style_btn_modern);
    lv_style_set_radius(&style_btn_modern, 4);
    lv_style_set_bg_color(&style_btn_modern, lv_palette_main(LV_PALETTE_BLUE));
    lv_style_set_text_color(&style_btn_modern, lv_color_white());
    lv_style_set_shadow_width(&style_btn_modern, 4);
    lv_style_set_shadow_color(&style_btn_modern, lv_palette_main(LV_PALETTE_GREY));
}

// --- Callbacks ---
static void btn_event_cb(lv_event_t * e) {
    lv_obj_t * btn = lv_event_get_target(e);
    const char * txt = lv_label_get_text(lv_obj_get_child(btn, 0));
    
    if(strcmp(txt, "RAISE") == 0) g_status.state = STATE_ROAD_CLEARING;
    if(strcmp(txt, "LOWER") == 0) g_status.state = STATE_LOWERING;
    if(strcmp(txt, "HOLD") == 0) g_status.state = STATE_IDLE; 
}

static void popup_timer_cb(lv_timer_t * t) {
    lv_obj_t * mbox = (lv_obj_t *)t->user_data;
    lv_msgbox_close(mbox);
}

static void header_click_cb(lv_event_t * e) {
    char buf[64];
    sprintf(buf, "Instantaneous Motor Draw:\n%d mA", g_status.motor_current_ma);
    
    // Create popup
    static const char * btns[] = {""};
    lv_obj_t * mbox = lv_msgbox_create(NULL, "Motor Telemetry", buf, btns, false);
    lv_obj_center(mbox);
    
    // Auto-close after 4 seconds
    lv_timer_t * timer = lv_timer_create(popup_timer_cb, 4000, mbox);
    lv_timer_set_repeat_count(timer, 1);
}

// --- Page Builders ---
void build_home(lv_obj_t *parent) {
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, 140, 150);
    lv_obj_align(card, LV_ALIGN_LEFT_MID, 5, -10);
    lv_obj_add_style(card, &style_card, 0);

    lv_obj_t *lbl = lv_label_create(card);
    lv_label_set_text(lbl, "DECK HEIGHT");
    lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, 0);

    bar_pos = lv_bar_create(card);
    lv_obj_set_size(bar_pos, 25, 90);
    lv_obj_align(bar_pos, LV_ALIGN_CENTER, 0, 5);
    lv_bar_set_range(bar_pos, 0, DECK_HEIGHT_MAX_MM);
    lv_obj_set_style_bg_color(bar_pos, lv_palette_main(LV_PALETTE_CYAN), LV_PART_INDICATOR);

    lbl_pos = lv_label_create(card);
    lv_obj_align(lbl_pos, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_text_font(lbl_pos, &lv_font_montserrat_18, 0);

    lv_obj_t *card_lights = lv_obj_create(parent);
    lv_obj_set_size(card_lights, 140, 150);
    lv_obj_align(card_lights, LV_ALIGN_RIGHT_MID, -5, -10);
    lv_obj_add_style(card_lights, &style_card, 0);

    lv_obj_t *lbl2 = lv_label_create(card_lights);
    lv_label_set_text(lbl2, "ROAD TRAFFIC");
    lv_obj_align(lbl2, LV_ALIGN_TOP_MID, 0, 0);

    led_road_g = lv_led_create(card_lights);
    lv_obj_set_size(led_road_g, 24, 24);
    lv_obj_align(led_road_g, LV_ALIGN_CENTER, 0, -30);
    lv_led_set_color(led_road_g, lv_palette_main(LV_PALETTE_GREEN));

    led_road_y = lv_led_create(card_lights);
    lv_obj_set_size(led_road_y, 24, 24);
    lv_obj_align(led_road_y, LV_ALIGN_CENTER, 0, 0);
    lv_led_set_color(led_road_y, lv_palette_main(LV_PALETTE_AMBER));

    led_road_r = lv_led_create(card_lights);
    lv_obj_set_size(led_road_r, 24, 24);
    lv_obj_align(led_road_r, LV_ALIGN_CENTER, 0, 30);
    lv_led_set_color(led_road_r, lv_palette_main(LV_PALETTE_RED));
}

void build_ops(lv_obj_t *parent) {
    lv_obj_t *btn_raise = lv_btn_create(parent);
    lv_obj_set_size(btn_raise, 100, 40);
    lv_obj_align(btn_raise, LV_ALIGN_TOP_LEFT, 10, 0);
    lv_obj_add_style(btn_raise, &style_btn_modern, 0);
    lv_obj_t *l1 = lv_label_create(btn_raise);
    lv_label_set_text(l1, "RAISE");
    lv_obj_center(l1);
    lv_obj_add_event_cb(btn_raise, btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_lower = lv_btn_create(parent);
    lv_obj_set_size(btn_lower, 100, 40);
    lv_obj_align(btn_lower, LV_ALIGN_TOP_LEFT, 10, 50);
    lv_obj_add_style(btn_lower, &style_btn_modern, 0);
    lv_obj_t *l2 = lv_label_create(btn_lower);
    lv_label_set_text(l2, "LOWER");
    lv_obj_center(l2);
    lv_obj_add_event_cb(btn_lower, btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_hold = lv_btn_create(parent);
    lv_obj_set_size(btn_hold, 100, 40);
    lv_obj_align(btn_hold, LV_ALIGN_TOP_LEFT, 10, 100);
    lv_obj_set_style_bg_color(btn_hold, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_t *l3 = lv_label_create(btn_hold);
    lv_label_set_text(l3, "HOLD");
    lv_obj_center(l3);
    lv_obj_add_event_cb(btn_hold, btn_event_cb, LV_EVENT_CLICKED, NULL);

    meter_motor = lv_meter_create(parent);
    lv_obj_set_size(meter_motor, 150, 150);
    lv_obj_align(meter_motor, LV_ALIGN_RIGHT_MID, -10, -10);

    lv_meter_scale_t * scale = lv_meter_add_scale(meter_motor);
    lv_meter_set_scale_ticks(meter_motor, scale, 41, 2, 10, lv_palette_main(LV_PALETTE_GREY));
    lv_meter_set_scale_range(meter_motor, scale, 0, 2500, 270, 135);

    indic_motor = lv_meter_add_needle_line(meter_motor, scale, 4, lv_palette_main(LV_PALETTE_CYAN), -10);
}

void build_vision(lv_obj_t *parent) {
    // High-tech radar visualizer
    radar_bg = lv_obj_create(parent);
    lv_obj_set_size(radar_bg, 200, 120);
    lv_obj_align(radar_bg, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(radar_bg, lv_palette_darken(LV_PALETTE_BLUE_GREY, 4), 0); // Keep radar dark even in light mode
    
    target_box = lv_obj_create(radar_bg);
    lv_obj_set_size(target_box, 40, 40);
    lv_obj_align(target_box, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_opa(target_box, 0, 0);
    lv_obj_set_style_border_width(target_box, 2, 0);
    lv_obj_set_style_border_color(target_box, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_add_flag(target_box, LV_OBJ_FLAG_HIDDEN); // Hidden initially

    lbl_target = lv_label_create(parent);
    lv_label_set_text(lbl_target, "SCANNING...");
    lv_obj_align(lbl_target, LV_ALIGN_TOP_MID, 0, 125);
    lv_obj_set_style_text_color(lbl_target, lv_palette_main(LV_PALETTE_GREY), 0);

    bar_confidence = lv_bar_create(parent);
    lv_obj_set_size(bar_confidence, 200, 15);
    lv_obj_align(bar_confidence, LV_ALIGN_TOP_MID, 0, 145);
    lv_bar_set_range(bar_confidence, 0, 100);
}

void build_cw(lv_obj_t *parent) {
    // Left Tank
    lv_obj_t *card_l = lv_obj_create(parent);
    lv_obj_set_size(card_l, 130, 150);
    lv_obj_align(card_l, LV_ALIGN_LEFT_MID, 10, -10);
    lv_obj_add_style(card_l, &style_card, 0);

    lv_obj_t *l1 = lv_label_create(card_l);
    lv_label_set_text(l1, "LEFT TANK");
    lv_obj_align(l1, LV_ALIGN_TOP_MID, 0, 0);

    bar_cw_l = lv_bar_create(card_l);
    lv_obj_set_size(bar_cw_l, 30, 80);
    lv_obj_align(bar_cw_l, LV_ALIGN_CENTER, 0, 0);
    lv_bar_set_range(bar_cw_l, 0, 150);

    lbl_cw_l_stat = lv_label_create(card_l);
    lv_label_set_text(lbl_cw_l_stat, "IDLE");
    lv_obj_align(lbl_cw_l_stat, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_text_font(lbl_cw_l_stat, &lv_font_montserrat_12, 0);

    // Right Tank
    lv_obj_t *card_r = lv_obj_create(parent);
    lv_obj_set_size(card_r, 130, 150);
    lv_obj_align(card_r, LV_ALIGN_RIGHT_MID, -10, -10);
    lv_obj_add_style(card_r, &style_card, 0);

    lv_obj_t *l2 = lv_label_create(card_r);
    lv_label_set_text(l2, "RIGHT TANK");
    lv_obj_align(l2, LV_ALIGN_TOP_MID, 0, 0);

    bar_cw_r = lv_bar_create(card_r);
    lv_obj_set_size(bar_cw_r, 30, 80);
    lv_obj_align(bar_cw_r, LV_ALIGN_CENTER, 0, 0);
    lv_bar_set_range(bar_cw_r, 0, 150);

    lbl_cw_r_stat = lv_label_create(card_r);
    lv_label_set_text(lbl_cw_r_stat, "IDLE");
    lv_obj_align(lbl_cw_r_stat, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_text_font(lbl_cw_r_stat, &lv_font_montserrat_12, 0);
}

void build_settings(lv_obj_t *parent) {
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, 280, 150);
    lv_obj_align(card, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_add_style(card, &style_card, 0);

    lv_obj_t *lbl_title = lv_label_create(card);
    lv_label_set_text(lbl_title, "SYSTEM SETTINGS");
    lv_obj_align(lbl_title, LV_ALIGN_TOP_MID, 0, -5);

    lv_obj_t *lbl_auto = lv_label_create(card);
    lv_label_set_text(lbl_auto, "Auto Mode:");
    lv_obj_align(lbl_auto, LV_ALIGN_LEFT_MID, 10, -35);

    lv_obj_t *sw_auto = lv_switch_create(card);
    lv_obj_align(sw_auto, LV_ALIGN_RIGHT_MID, -10, -35);
    if(g_status.auto_mode_active) lv_obj_add_state(sw_auto, LV_STATE_CHECKED);
    lv_obj_add_event_cb(sw_auto, [](lv_event_t *e){
        lv_obj_t * sw = lv_event_get_target(e);
        g_status.auto_mode_active = lv_obj_has_state(sw, LV_STATE_CHECKED);
    }, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t *lbl_theme = lv_label_create(card);
    lv_label_set_text(lbl_theme, "Dark Mode:");
    lv_obj_align(lbl_theme, LV_ALIGN_LEFT_MID, 10, 0);

    lv_obj_t *sw_theme = lv_switch_create(card);
    lv_obj_align(sw_theme, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_add_event_cb(sw_theme, [](lv_event_t *e){
        lv_obj_t * sw = lv_event_get_target(e);
        bool is_dark = lv_obj_has_state(sw, LV_STATE_CHECKED);
        lv_disp_t * disp = lv_disp_get_default();
        lv_theme_t * theme = lv_theme_default_init(disp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), is_dark, LV_FONT_DEFAULT);
        lv_disp_set_theme(disp, theme);
        
        if(is_dark) {
            lv_style_set_bg_color(&style_card, lv_palette_darken(LV_PALETTE_GREY, 4));
            lv_style_set_border_color(&style_card, lv_palette_darken(LV_PALETTE_GREY, 3));
            lv_style_set_text_color(&style_btn_modern, lv_color_white());
        } else {
            lv_style_set_bg_color(&style_card, lv_color_white());
            lv_style_set_border_color(&style_card, lv_palette_lighten(LV_PALETTE_GREY, 2));
            lv_style_set_text_color(&style_btn_modern, lv_color_white());
        }
        lv_obj_report_style_change(&style_card);
    }, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t *lbl_br = lv_label_create(card);
    lv_label_set_text(lbl_br, "HMI Brightness:");
    lv_obj_align(lbl_br, LV_ALIGN_LEFT_MID, 10, 35);

    lv_obj_t *slider_br = lv_slider_create(card);
    lv_obj_set_size(slider_br, 100, 10);
    lv_obj_align(slider_br, LV_ALIGN_RIGHT_MID, -10, 35);
    lv_slider_set_range(slider_br, 10, 100);
    lv_slider_set_value(slider_br, g_status.hmi_brightness, LV_ANIM_OFF);
    lv_obj_add_event_cb(slider_br, [](lv_event_t *e) {
        lv_obj_t * slider = lv_event_get_target(e);
        g_status.hmi_brightness = lv_slider_get_value(slider);
    }, LV_EVENT_VALUE_CHANGED, NULL);
}

void ui_init() {
    init_styles();

    // Top Bar (Clickable for Telemetry)
    header = lv_obj_create(lv_scr_act());
    lv_obj_set_size(header, 320, 35);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(header, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_radius(header, 0, 0);
    lv_obj_add_flag(header, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(header, header_click_cb, LV_EVENT_CLICKED, NULL);

    lbl_state = lv_label_create(header);
    lv_label_set_recolor(lbl_state, true);
    lv_obj_set_style_bg_color(lbl_state, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(lbl_state, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(lbl_state, 2, 0);
    lv_obj_set_style_radius(lbl_state, 4, 0);
    lv_label_set_text(lbl_state, "#FFFFFF SYS: IDLE# | #FFFFFF MOTOR: IDLE#");
    lv_obj_center(lbl_state);

    // Main Tabview
    tv = lv_tabview_create(lv_scr_act(), LV_DIR_BOTTOM, 40);
    lv_obj_align(tv, LV_ALIGN_TOP_MID, 0, 35);
    lv_obj_set_size(tv, 320, 205);
    lv_obj_set_style_bg_color(tv, lv_palette_lighten(LV_PALETTE_GREY, 4), 0);

    pg_home = lv_tabview_add_tab(tv, "HOME");
    pg_ops = lv_tabview_add_tab(tv, "OPS");
    pg_vision = lv_tabview_add_tab(tv, "VISION");
    pg_cw = lv_tabview_add_tab(tv, "CW");
    pg_settings = lv_tabview_add_tab(tv, "SET");

    build_home(pg_home);
    build_ops(pg_ops);
    build_vision(pg_vision);
    build_cw(pg_cw);
    build_settings(pg_settings);
    
    // Timer to update UI from g_status
    lv_timer_create([](lv_timer_t * t) {
        // Update State
        const char* state_names[] = {"IDLE", "CLEARING", "RAISING", "HOLD", "LOWERING", "OPENING", "FAULT", "ESTOP", "INIT"};
        const char* sys_col = "#00FF00"; // Green
        if(g_status.state == STATE_FAULT || g_status.state == STATE_ESTOP) sys_col = "#FF0000"; // Red
        else if(g_status.state == STATE_RAISED_HOLD) sys_col = "#FFA500"; // Amber
        else if(g_status.state != STATE_IDLE) sys_col = "#00FFFF"; // Cyan

        const char* mot_stat = "IDLE";
        const char* mot_col = "#FFFFFF"; // White
        if (g_status.motor_current_ma == 0) {
            mot_stat = "IDLE";
            mot_col = "#FFFFFF";
        } else if (g_status.motor_current_ma < 1500) {
            mot_stat = "OK";
            mot_col = "#00FF00"; // Green
        } else if (g_status.motor_current_ma <= 1980) {
            mot_stat = "WARNING";
            mot_col = "#FFFF00"; // Yellow
        } else {
            mot_stat = "CRITICAL";
            mot_col = (g_status.uptime_ms % 500 < 250) ? "#FF0000" : "#FFFFFF"; // Flashing Red
        }

        char buf_s[128];
        const char* mode_str = g_status.auto_mode_active ? "#FF00FF [AUTO]#" : "#AAAAAA [MAN] #";
        sprintf(buf_s, "%s SYS:%s# %s| %s MOT:%s#", sys_col, state_names[g_status.state], mode_str, mot_col, mot_stat);
        lv_label_set_text(lbl_state, buf_s);

        // Update Position
        char buf_p[16];
        sprintf(buf_p, "%d mm", g_status.deck_position_mm);
        lv_label_set_text(lbl_pos, buf_p);
        lv_bar_set_value(bar_pos, g_status.deck_position_mm, LV_ANIM_ON);

        // Update Meter
        lv_meter_set_indicator_value(meter_motor, indic_motor, g_status.motor_current_ma);

        // Update Traffic LEDs
        lv_led_off(led_road_g); lv_led_off(led_road_y); lv_led_off(led_road_r);
        if(g_status.state == STATE_IDLE) lv_led_on(led_road_g);
        else if(g_status.state == STATE_ROAD_CLEARING) lv_led_on(led_road_y);
        else lv_led_on(led_road_r);

        // Update Vision Radar
        if(g_status.vision.vehicle_present) {
            lv_obj_clear_flag(target_box, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(lbl_target, "TARGET ACQUIRED");
            lv_obj_set_style_text_color(lbl_target, lv_palette_main(LV_PALETTE_RED), 0);
            
            // Add a little random jitter to the box for "radar" effect
            lv_obj_align(target_box, LV_ALIGN_CENTER, (rand()%10)-5, (rand()%10)-5);
        } else {
            lv_obj_add_flag(target_box, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(lbl_target, "SCANNING...");
            lv_obj_set_style_text_color(lbl_target, lv_palette_main(LV_PALETTE_GREY), 0);
        }
        lv_bar_set_value(bar_confidence, g_status.vision.confidence, LV_ANIM_ON);

        // Update Counterweights
        lv_bar_set_value(bar_cw_l, (int)g_status.counterweight.left.water_level_ml, LV_ANIM_ON);
        lv_bar_set_value(bar_cw_r, (int)g_status.counterweight.right.water_level_ml, LV_ANIM_ON);

        char cw_stat_l[32];
        if(g_status.counterweight.left.pump_active) sprintf(cw_stat_l, "PUMPING\n%d ML", (int)g_status.counterweight.left.water_level_ml);
        else if(g_status.counterweight.left.drain_open) sprintf(cw_stat_l, "DRAINING\n%d ML", (int)g_status.counterweight.left.water_level_ml);
        else sprintf(cw_stat_l, "IDLE\n%d ML", (int)g_status.counterweight.left.water_level_ml);
        lv_label_set_text(lbl_cw_l_stat, cw_stat_l);

        char cw_stat_r[32];
        if(g_status.counterweight.right.pump_active) sprintf(cw_stat_r, "PUMPING\n%d ML", (int)g_status.counterweight.right.water_level_ml);
        else if(g_status.counterweight.right.drain_open) sprintf(cw_stat_r, "DRAINING\n%d ML", (int)g_status.counterweight.right.water_level_ml);
        else sprintf(cw_stat_r, "IDLE\n%d ML", (int)g_status.counterweight.right.water_level_ml);
        lv_label_set_text(lbl_cw_r_stat, cw_stat_r);

    }, 100, NULL);
}
