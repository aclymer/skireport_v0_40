#include "pebble.h"
	
static Window *window;
static Window *settings;
ActionBarLayer *settings_action_bar;
static TextLayer *settings_US_text_layer;
static TextLayer *settings_METRIC_text_layer;
static TextLayer *weather_layer;
static TextLayer *wind_layer;
static TextLayer *temps_layer;
static TextLayer *snowfall_layer;
static TextLayer *update_layer;
static TextLayer *area_name_layer;
static BitmapLayer *button_bar_layer;
static GBitmap *bmp_button_bar;
static GBitmap *up_arrow_icon;
static GBitmap *down_arrow_icon;
static GBitmap *check_mark_icon;

int units = 0;
int index = 0;
int refresh_interval = 0;
static AppSync sync;
static uint8_t sync_buffer[1024];
// char *unit_length = "in";

enum key {
	INDEX_KEY = 0X0,			// TUPLE_INTEGER
	AREA_NAME_KEY = 0x1,		// TUPLE_CSTRING
	WIND_KEY = 0x2,				// TUPLE_CSTRING
	AREA_TEMPS_KEY = 0x3,		// TUPLE_CSTRING
	AREA_SNOWFALL_KEY = 0x4,	// TUPLE_CSTRING
	UPDATE_TIME_KEY = 0x5,		// TUPLE_CSTRING
	WEATHER_DESC_KEY = 0x6,		// TUPLE_CSTRING
	UNITS_KEY = 0x7				// TUPLE_INTEGER
};

static void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {
	switch (key) {
		
		case AREA_NAME_KEY:
		text_layer_set_text(area_name_layer, new_tuple->value->cstring);
		break;
		
		case WIND_KEY:
		text_layer_set_text(wind_layer, new_tuple->value->cstring);
		break;
		
		case AREA_TEMPS_KEY:
		text_layer_set_text(temps_layer, new_tuple->value->cstring);
		break;
		
		case AREA_SNOWFALL_KEY:
		text_layer_set_text(snowfall_layer, new_tuple->value->cstring);
		break;
		
		case UPDATE_TIME_KEY:
		text_layer_set_text(update_layer, new_tuple->value->cstring);
		break;	  
		
		case WEATHER_DESC_KEY:
		text_layer_set_text(weather_layer, new_tuple->value->cstring);
		break;
	}
	refresh_interval = 1;
}

static void send_cmd(void) {
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	
	if (iter == NULL) {
		return;
	}
	
	Tuplet value = TupletInteger(0, index);	
	dict_write_tuplet(iter, &value);
	Tuplet load = TupletCString(1, "Loading...");
	dict_write_tuplet(iter, &load);
	Tuplet snow = TupletCString(4, "Snow(24h): -- in");
	dict_write_tuplet(iter, &snow);
	Tuplet unit = TupletInteger(7, units);
	dict_write_tuplet(iter, &unit);
	dict_write_end(iter);
	
	app_message_outbox_send();
}

void settings_up_single_click_handler(ClickRecognizerRef recognizer, void *context) {
	if (units == 1) {
		text_layer_set_font(settings_US_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28));
		text_layer_set_font(settings_METRIC_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
	}
	units = 0;
}

void settings_down_single_click_handler(ClickRecognizerRef recognizer, void *context) {
	if (units == 0) {
		text_layer_set_font(settings_US_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
		text_layer_set_font(settings_METRIC_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28));
	}
	units = 1;
}

void settings_select_single_click_handler(ClickRecognizerRef recognizer, void *context) {
	// For Select Feature - Set Settings & return to main Window
	send_cmd();
	window_stack_pop(true);
	window_stack_push(window, true);
}

void up_single_click_handler(ClickRecognizerRef recognizer, void *context) {
	if (units > 0) {
		units += 1;
	}
	else {units = 9;}
	send_cmd();
}

void down_single_click_handler(ClickRecognizerRef recognizer, void *context) {
	if (index < 9) {
		index += 1;
	}
	else {index = 0;}
	send_cmd();
}
void select_long_click_handler(ClickRecognizerRef recognizer, void *context) {
	// Push settings window onto stack
	window_stack_push(settings, true);
}

void select_single_click_handler(ClickRecognizerRef recognizer, void *context) {
	// For Select Feature
	if (refresh_interval == 0) {
		send_cmd();
		refresh_interval = 1;
	}
	
}

static void sync_error_callback(DictionaryResult dict_error, AppMessageResult app_message_error, void *context) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Sync Error: %d", app_message_error);
	if (app_message_error == 64) {send_cmd();}
}

void settings_click_config_provider(void *context) {
	
	//	Single Click Up Action
	window_single_click_subscribe(BUTTON_ID_UP, settings_up_single_click_handler);
	
	//	Single Click Down
	window_single_click_subscribe(BUTTON_ID_DOWN, settings_down_single_click_handler);
	
	//	Single Click Select
	window_single_click_subscribe(BUTTON_ID_SELECT, settings_select_single_click_handler);
}

static void settings_load(Window *settings) {
	Layer *settings_layer = window_get_root_layer(settings);
	
	//Set up Action Bar
	up_arrow_icon = gbitmap_create_with_resource(RESOURCE_ID_ICON_UP_ARROW);
	down_arrow_icon = gbitmap_create_with_resource(RESOURCE_ID_ICON_DOWN_ARROW);
	check_mark_icon = gbitmap_create_with_resource(RESOURCE_ID_ICON_OK);
	settings_action_bar = action_bar_layer_create();
	action_bar_layer_add_to_window(settings_action_bar, settings);
	action_bar_layer_set_background_color(settings_action_bar, GColorWhite);
	action_bar_layer_set_click_config_provider(settings_action_bar, settings_click_config_provider);
	action_bar_layer_set_icon(settings_action_bar, BUTTON_ID_UP, up_arrow_icon);
	action_bar_layer_set_icon(settings_action_bar, BUTTON_ID_DOWN, down_arrow_icon);
	action_bar_layer_set_icon(settings_action_bar, BUTTON_ID_SELECT, check_mark_icon);
	
	// Settings US Text Layer
	settings_US_text_layer = text_layer_create(GRect(20, 42, 80, 30));
	text_layer_set_text_color(settings_US_text_layer, GColorWhite);
	text_layer_set_background_color(settings_US_text_layer, GColorClear);
	text_layer_set_font(settings_US_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28));
	text_layer_set_text_alignment(settings_US_text_layer, GTextAlignmentCenter);
	text_layer_set_text(settings_US_text_layer, "US");
	layer_add_child(settings_layer, text_layer_get_layer(settings_US_text_layer));
	
	// Settings Metric Text Layer
	settings_METRIC_text_layer = text_layer_create(GRect(20, 72, 80, 30));
	text_layer_set_text_color(settings_METRIC_text_layer, GColorWhite);
	text_layer_set_background_color(settings_METRIC_text_layer, GColorClear);
	text_layer_set_font(settings_METRIC_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
	text_layer_set_text_alignment(settings_METRIC_text_layer, GTextAlignmentCenter);
	text_layer_set_text(settings_METRIC_text_layer, "METRIC");
	layer_add_child(settings_layer, text_layer_get_layer(settings_METRIC_text_layer));
}

static void window_load(Window *window) {
	Layer *window_layer = window_get_root_layer(window);
	static int row_height = 22;
	int it = 2;
	const int button_bar_width = 14;
	const int text_layer_width = 144 - button_bar_width;
	
	// Button Bar Layer
	bmp_button_bar = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BUTTON_BAR);
	button_bar_layer = bitmap_layer_create(GRect(text_layer_width, 0, button_bar_width, 152));
	bitmap_layer_set_bitmap(button_bar_layer, bmp_button_bar);
	layer_add_child(window_layer, bitmap_layer_get_layer(button_bar_layer));
	
	// Area Name Layer	
	area_name_layer = text_layer_create(GRect(0, -2, text_layer_width, 48));
	text_layer_set_text_color(area_name_layer, GColorWhite);
	text_layer_set_background_color(area_name_layer, GColorClear);
	text_layer_set_font(area_name_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	text_layer_set_text_alignment(area_name_layer, GTextAlignmentCenter);
	text_layer_set_overflow_mode(area_name_layer, GTextOverflowModeWordWrap);
	layer_add_child(window_layer, text_layer_get_layer(area_name_layer));
	
	// Weather Description Layer
	weather_layer = text_layer_create(GRect(0, it++ * row_height, text_layer_width, row_height));
	text_layer_set_text_color(weather_layer, GColorWhite);
	text_layer_set_background_color(weather_layer, GColorClear);
	text_layer_set_font(weather_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
	text_layer_set_text_alignment(weather_layer, GTextAlignmentCenter);
	layer_add_child(window_layer, text_layer_get_layer(weather_layer));
	
	// Snowfall Layer	
	snowfall_layer = text_layer_create(GRect(0, it++ * row_height, text_layer_width, row_height));
	text_layer_set_text_color(snowfall_layer, GColorWhite);
	text_layer_set_background_color(snowfall_layer, GColorClear);
	text_layer_set_font(snowfall_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
	text_layer_set_text_alignment(snowfall_layer, GTextAlignmentCenter);
	layer_add_child(window_layer, text_layer_get_layer(snowfall_layer));
	
	// Temperature Layer
	temps_layer = text_layer_create(GRect(0, it++ * row_height, text_layer_width, row_height));
	text_layer_set_text_color(temps_layer, GColorWhite);
	text_layer_set_background_color(temps_layer, GColorClear);
	text_layer_set_font(temps_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
	text_layer_set_text_alignment(temps_layer, GTextAlignmentCenter);
	layer_add_child(window_layer, text_layer_get_layer(temps_layer));
	
	// Wind Layer	
	wind_layer = text_layer_create(GRect(0, it++ * row_height, text_layer_width, row_height));
	text_layer_set_text_color(wind_layer, GColorWhite);
	text_layer_set_background_color(wind_layer, GColorClear);
	text_layer_set_font(wind_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
	text_layer_set_text_alignment(wind_layer, GTextAlignmentCenter);
	layer_add_child(window_layer, text_layer_get_layer(wind_layer));
	
	// Update Time Layer
	update_layer = text_layer_create(GRect(0, it++ * row_height + 4, text_layer_width, row_height));
	text_layer_set_text_color(update_layer, GColorWhite);
	text_layer_set_background_color(update_layer, GColorClear);
	text_layer_set_font(update_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
	text_layer_set_text_alignment(update_layer, GTextAlignmentCenter);
	layer_add_child(window_layer, text_layer_get_layer(update_layer));
	
	Tuplet initial_values[] = {
		TupletInteger(INDEX_KEY, (uint8_t) 0),
		TupletCString(AREA_NAME_KEY, "Loading..."),
		TupletCString(WEATHER_DESC_KEY, "Loading..."),
		TupletCString(WIND_KEY, "Wind: --- @ -- ---"),
		TupletCString(AREA_TEMPS_KEY, "Temp(--): -- to --"),
		TupletCString(AREA_SNOWFALL_KEY, "Snow(24h): ---"),
		TupletCString(UPDATE_TIME_KEY, "Updated @ --:--"),
		TupletInteger(UNITS_KEY, (uint8_t) 0),
	};
	
	app_sync_init(&sync, sync_buffer, sizeof(sync_buffer), initial_values, ARRAY_LENGTH(initial_values),
				  sync_tuple_changed_callback, sync_error_callback, NULL);
	
	send_cmd();
}

static void settings_unload(Window *settings) {
	text_layer_destroy(settings_US_text_layer);
	text_layer_destroy(settings_METRIC_text_layer);
	window_stack_pop(true);
}

static void window_unload(Window *window) {
	app_sync_deinit(&sync);
	text_layer_destroy(wind_layer);
	text_layer_destroy(weather_layer);
	text_layer_destroy(area_name_layer);
	text_layer_destroy(temps_layer);
	text_layer_destroy(snowfall_layer);
	text_layer_destroy(update_layer);
	gbitmap_destroy(bmp_button_bar);
	bitmap_layer_destroy(button_bar_layer);
}

void click_config_provider(Window *window) {
	
	//	Single Click Up Action
	window_single_click_subscribe(BUTTON_ID_UP, up_single_click_handler);
	
	//	Single Click Down
	window_single_click_subscribe(BUTTON_ID_DOWN, down_single_click_handler);
	
	//	Single Click Select
	window_single_click_subscribe(BUTTON_ID_SELECT, select_single_click_handler);
	
	//	Long Click Select
	window_long_click_subscribe(BUTTON_ID_SELECT, (uint16_t) 500, NULL, select_long_click_handler);
}

static void init(void) {	
	// Create Main Window & Load
	window = window_create();
	window_set_click_config_provider(window, (ClickConfigProvider) click_config_provider);
	APP_LOG(APP_LOG_LEVEL_DEBUG_VERBOSE, "Done initializing, pushed window: %p", window);
	window_set_background_color(window, GColorBlack);
	window_set_window_handlers(window, (WindowHandlers) {
		.load = window_load,
		.unload = window_unload
	});
	
	// Create Settings Window & Load
	settings = window_create();
	window_set_click_config_provider(settings, (ClickConfigProvider) settings_click_config_provider);
	APP_LOG(APP_LOG_LEVEL_DEBUG_VERBOSE, "Pushing window: %p", settings);
	window_set_background_color(settings, GColorBlack);
	window_set_window_handlers(settings, (WindowHandlers) {
		.load = settings_load,
		.unload = settings_unload
	});
	
	const int inbound_size = 512;
	const int outbound_size = 512;
	app_message_open(inbound_size, outbound_size);
	
	window_stack_push(window, true);
}

static void deinit(void) {
	window_destroy(window);
	window_destroy(settings);
}

int main(void) {
	init();
	app_event_loop();
	deinit();
}