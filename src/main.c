#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"
#include "http.h"
#include "httpcapture.h"
#include "mini-printf.h"
#include "config.h"
#include "pbl-math.h"
#include "sunmoon.h"
#include "util.h"

/* 
Because of the way that httpebble works, a different UUID is needed for Android and iOS compatibility. 
If you are building this to use with Android, then leave the #define ANDROID line alone, and if 
you're building for iOS then remove or comment out that line.
*/
 
#define MY_UUID { 0x91, 0x41, 0xB6, 0x28, 0xBC, 0x89, 0x49, 0x8E, 0xB1, 0x47, 0x04, 0x9F, 0x49, 0xC0, 0x99, 0xAD }
 
#define HTTP_COOKIE 4888
#define FG_COLOR GColorWhite
#define BG_COLOR GColorBlack
#define ERROR_CODE "unknown"
#define WEATHER_KEY_LATITUDE 1
#define WEATHER_KEY_LONGITUDE 2

#define MAKE_SCREENSHOT 1

PBL_APP_INFO(MY_UUID, "Abe's Weather", "VIwebworks", 1, 0,  RESOURCE_ID_IMAGE_MENU_ICON, APP_INFO_WATCH_FACE);
 
 
void handle_init(AppContextRef ctx);
void http_success(int32_t request_id, int http_status, DictionaryIterator* received, void* context);
void http_failure(int32_t request_id, int http_status, void* context);
void window_appear(Window* me);
void httpebble_error(int error_code);
 
Window window;
TextLayer layer_textTemp;
TextLayer layer_textImage;
TextLayer layer_textSRise;
TextLayer layer_textSSet;
TextLayer layer_textError;
TextLayer layerUpdated;
TextLayer layerConditions;
TextLayer day_layer;
TextLayer date_layer;
TextLayer time_layer;
TextLayer secs_layer;
TextLayer ampm_layer;
TextLayer moonPercent;
Layer line_layer;
GFont font_date;
GFont font_time;
GFont font_temp;
GFont font_cond;
GFont font_times;
GFont font_moon;
char temp_str[5];
static bool havess = false;


static int our_latitude, our_longitude;
static bool located;

BmpContainer icon_layer;

void request_weather();
void handle_timer(AppContextRef app_ctx, AppTimerHandle handle, uint32_t cookie);
 


void set_icon(char* icon)
{
	//text_layer_set_text(&layerUpdated, itoa(icon));
	//text_layer_set_text(&layer_textTemp, icon);
	//if (icon != currenticon)
	//{
		//unload the icon that's there
		//if (currenticon != 99)
		//{
			//need to remove before setting
			layer_remove_from_parent(&icon_layer.layer.layer);
			bmp_deinit_container(&icon_layer);
		//}
		//then load one up
		//bmp_init_container(WEATHER_ICONS[icon], &icon_layer);
	if (strcmp(icon, "0") == 0){
			bmp_init_container(RESOURCE_ID_ICON_CHANCEFLURRIES_BLACK, &icon_layer);
			}
			if (strcmp(icon, "1") == 0 ){
			bmp_init_container(RESOURCE_ID_ICON_CHANCESNOW_BLACK, &icon_layer);
			}
			if (strcmp(icon, "2") == 0 ){
			bmp_init_container(RESOURCE_ID_ICON_CHANCETSTORMS_BLACK, &icon_layer);
			}
			if (strcmp(icon, "3") == 0 ){
			bmp_init_container(RESOURCE_ID_ICON_CLOUDY_BLACK, &icon_layer);
			}
			if (strcmp(icon, "4") == 0 ){
			bmp_init_container(RESOURCE_ID_ICON_FLURRIES_BLACK, &icon_layer);
			}
			if (strcmp(icon, "5") == 0 ){
			bmp_init_container(RESOURCE_ID_ICON_NT_CHANCEFLURRIES_BLACK, &icon_layer);
			}
			if (strcmp(icon, "6") == 0 ){
			bmp_init_container(RESOURCE_ID_ICON_NT_CHANCESNOW_BLACK, &icon_layer);
			}
			if (strcmp(icon, "7") == 0 ){
			bmp_init_container(RESOURCE_ID_ICON_NT_PARTLYCLOUDY_BLACK, &icon_layer);
			}
			if (strcmp(icon, "8") == 0 ){
			bmp_init_container(RESOURCE_ID_ICON_NT_SUNNY_BLACK, &icon_layer);
			}
			if (strcmp(icon, "9") == 0 ){
			bmp_init_container(RESOURCE_ID_ICON_PARTLYCLOUDY_BLACK, &icon_layer);
			}
			if (strcmp(icon, "10") == 0 ){
			bmp_init_container(RESOURCE_ID_ICON_RAIN_BLACK, &icon_layer);
			}
			if (strcmp(icon, "11") == 0 ){
			bmp_init_container(RESOURCE_ID_ICON_SNOW_BLACK, &icon_layer);
			}
			if (strcmp(icon, "12") == 0 ){
			bmp_init_container(RESOURCE_ID_ICON_SUNNY_BLACK, &icon_layer);
			}
			if (strcmp(icon, "13") == 0 ){
			bmp_init_container(RESOURCE_ID_ICON_UNKNOWN_BLACK, &icon_layer);
			}
			if (strcmp(icon, "14") == 0 ){
			bmp_init_container(RESOURCE_ID_ICON_FOG_BLACK, &icon_layer);
			}
	bitmap_layer_set_alignment(&icon_layer.layer, GAlignCenter);
	bitmap_layer_set_background_color(&icon_layer.layer, GColorClear);
		layer_add_child(&window.layer, &icon_layer.layer.layer);
		layer_set_frame(&icon_layer.layer.layer, GRect (35, 0, 70, 70));
	//bitmap_layer_set_compositing_mode(&icon_layer.layer, GCompOpClear);
	//}
	//currenticon = icon;
}
void set_timer(AppContextRef ctx) {
	app_timer_send_event(ctx, 900000, 1);
}

void location(float latitude, float longitude, float altitude, float accuracy, void* context) {
	
	// Fix the floats
	our_latitude = latitude * 10000;
	our_longitude = longitude * 10000;
	located = true;
	request_weather();
	set_timer((AppContextRef)context);
}
/*Convert decimal hours, into hours and minutes with rounding*/
int hours(float time)
{
    return (int)(time+1/60.0*0.5);
}

int mins(float time)
{
    int m = (int)((time-(int)time)*60.0+0.5);
	return (m==60)?0:m;
}

//return julian day number for time
int tm2jd(PblTm* time)
{
    int y,m;
    y = time->tm_year + 1900;
    m = time->tm_mon + 1;
    return time->tm_mday-32075+1461*(y+4800+(m-14)/12)/4+367*(m-2-(m-14)/12*12)/12-3*((y+4900+(m-14)/12)/100)/4;
}

float moon_phase(int jdn)
{
    double jd;
    jd = jdn-2451550.1;
    jd /= 29.530588853;
    jd -= (int)jd;
    return jd;
}

//If 12 hour time, subtract 12 from hr if hr > 12
char* thr(float time, char ap)
{
    static char fmttime[] = "00:00A";
    int h = hours(time);
    int m = mins(time);
    if (clock_is_24h_style()) {
        mini_snprintf(fmttime, sizeof(fmttime), "%d:%02d",h,m);
    } else {
        if (h > 11) {
            if (h > 12) h -= 12;
            mini_snprintf(fmttime, sizeof(fmttime), (ap==1)?"%d:%02dP":"%d:%02d",h,m);
        } else {
			if (h == 0) h=12;
            mini_snprintf(fmttime, sizeof(fmttime), (ap==1)?"%d:%02dA":"%d:%02d",h,m);
        }
    }
    return fmttime;
}
char* mthr(float time1, float time2, char* inject)
{
    static char fmttime[] = "00:00A> 00:00A";
    int h1 = hours(time1);
    int m1 = mins(time1);
    int h2 = hours(time2);
    int m2 = mins(time2);
    if (clock_is_24h_style()) {
        mini_snprintf(fmttime, sizeof(fmttime), "%d:%02d%s %d:%02d",h1,m1,inject,h2,m2);
	} else {
        if (h1 > 11 && h2 > 11) {
            if (h1 > 12) h1 -= 12;
            if (h2 > 12) h2 -= 12;
            mini_snprintf(fmttime, sizeof(fmttime), "%d:%02dP%s %d:%02dP",h1,m1,inject,h2,m2);
        } else if (h1 > 11) {
			if (h1 > 12) h1 -= 12;
			if (h2 == 0) h2=12;
            mini_snprintf(fmttime, sizeof(fmttime), "%d:%02dP%s %d:%02dA",h1,m1,inject,h2,m2);
        } else if (h2 > 11) {
			if (h2 > 12) h2 -= 12;
			if (h1 == 0) h1=12;
            mini_snprintf(fmttime, sizeof(fmttime), "%d:%02dA%s %d:%02dP",h1,m1,inject,h2,m2);
        } else {
			if (h1 == 0) h1=12;
			if (h2 == 0) h2=12;
            mini_snprintf(fmttime, sizeof(fmttime), "%d:%02dA%s %d:%02dA",h1,m1,inject,h2,m2);
        }
    }
    return fmttime;
}

// Called once per day
void handle_day(AppContextRef ctx, PebbleTickEvent* t)
{

    (void)t;
    (void)ctx;

    static char riseText[] = "00:00";
    static char setText[] = "00:00";
    //static char moon1Text[] = "<00:00a\n<00:00a";
    //static char moon2Text[] = "00:00a>\n00:00a>";
    static char date[] = "00/00/0000";
    static char moon[] = "m";
    static char moonp[] = "-----";
    float moonphase_number = 0.0;
    int moonphase_letter = 0;
    float sunrise, sunset;//, moonrise[3], moonset[3];
    PblTm* time = t->tick_time;
    if (!t)
        get_time(time);

    // date
    string_format_time(date, sizeof(date), DATEFMT, time);
    //text_layer_set_text(&dateLayer, date);


    moonphase_number = moon_phase(tm2jd(time));
    moonphase_letter = (int)(moonphase_number*27 + 0.5);
    // correct for southern hemisphere
    if ((moonphase_letter > 0) && (LAT < 0))
        moonphase_letter = 28 - moonphase_letter;
    // select correct font char
    if (moonphase_letter == 14) {
        moon[0] = (unsigned char)(48);
    } else if (moonphase_letter == 0) {
        moon[0] = (unsigned char)(49);
    } else if (moonphase_letter < 14) {
        moon[0] = (unsigned char)(moonphase_letter+96);
    } else {
        moon[0] = (unsigned char)(moonphase_letter+95);
    }
    text_layer_set_text(&moonPercent, moon);
    if (moonphase_number >= 0.5) {
        mini_snprintf(moonp,sizeof(moonp)," %d-",(int)((1-(1+pbl_cos(moonphase_number*M_PI*2))/2)*100));
    } else {
        mini_snprintf(moonp,sizeof(moonp)," %d+",(int)((1-(1+pbl_cos(moonphase_number*M_PI*2))/2)*100));
    }
    //text_layer_set_text(&moonPercent, moonp);

    //sun rise set
    sunmooncalc(tm2jd(time), TZ, LAT, -LON, 1, &sunrise, &sunset);
    (sunrise == 99.0) ? mini_snprintf(riseText,sizeof(riseText),"--:--") : mini_snprintf(riseText,sizeof(riseText),"%s",thr(sunrise,0));
    (sunset == 99.0) ? mini_snprintf(setText,sizeof(setText),"--:--") : mini_snprintf(setText,sizeof(setText),"%s",thr(sunset,0));

    text_layer_set_text(&layer_textSRise, riseText);
    text_layer_set_text(&layer_textSSet, setText);
	request_weather();
}

	static const uint32_t const segments[] = {70};
VibePattern pat = {
	    .durations = segments,
		.num_segments = ARRAY_LENGTH(segments),
	};
void handle_second_tick(AppContextRef ctx, PebbleTickEvent *t)
{
    (void)ctx;

    // Need to be static because they're used by the system later.
    //static char year_text[] = "0000";
    static char date_text[] = "Xxxxxxxxx 00";
    static char day_text[] = "Xxxxxxxxx";
    static char time_text[] = "00:00";
    static char secs_text[] = "00";
    static char ampm_text[] = "XX";

    if (t->units_changed & DAY_UNIT)
    {
        string_format_time(day_text, sizeof(day_text), "%A", t->tick_time);
        text_layer_set_text(&day_layer, day_text);

        string_format_time(date_text, sizeof(date_text), "%b %e", t->tick_time);
        text_layer_set_text(&date_layer, date_text);

    }

    if (clock_is_24h_style())
    {
        strcpy(ampm_text, "  ");
        string_format_time(time_text, sizeof(time_text), "%R", t->tick_time);
    }
    else
    {
        string_format_time(ampm_text, sizeof(ampm_text), "%p", t->tick_time);
        string_format_time(time_text, sizeof(time_text), "%I:%M", t->tick_time);

        // Kludge to handle lack of non-padded hour format string
        // for twelve hour clock.
        if (!clock_is_24h_style() && (time_text[0] == '0'))
        {
            memmove(time_text, &time_text[1], sizeof(time_text) - 1);
        }
    }

    string_format_time(secs_text, sizeof(secs_text), "%S", t->tick_time);
    text_layer_set_text(&time_layer, time_text);
    text_layer_set_text(&secs_layer, secs_text);
    text_layer_set_text(&ampm_layer, ampm_text);
	
    // on the top of the hour
    if (t->tick_time->tm_min == 0 && t->tick_time->tm_sec == 0) {
#ifdef VIBHOUR
        // vibrate once if between 6am and 10pm
        if (t->tick_time->tm_hour >= 6 && t->tick_time->tm_hour <= 22)
            vibes_enqueue_custom_pattern(pat);
#endif
        //perform daily tasks is hour is 0
        if (t->tick_time->tm_hour == 0)
            handle_day(ctx, t);
    }
}
void handle_deinit(AppContextRef ctx)
{
    fonts_unload_custom_font(font_date);
    fonts_unload_custom_font(font_time);
    fonts_unload_custom_font(font_temp);
    fonts_unload_custom_font(font_cond);
    fonts_unload_custom_font(font_times);
    fonts_unload_custom_font(font_moon);
}

void reconnect(void* context) {
	request_weather();
}

void pbl_main(void *params) {
  PebbleAppHandlers handlers = {
    .init_handler = &handle_init,
        .deinit_handler = &handle_deinit,
    .messaging_info = {
      .buffer_sizes = {
        .inbound = 124,
        .outbound = 256,
      }
    },
        .tick_info =
        {
            .tick_handler = &handle_second_tick,
            .tick_units = SECOND_UNIT
        },
		.timer_handler = handle_timer
  };
	#if MAKE_SCREENSHOT
  http_set_app_id(27623);
  
  //http_register_callbacks((HTTPCallbacks){ 
      //.failure = NULL,
         //}, NULL);

  http_capture_main(&handlers);
#endif
  app_event_loop(params, &handlers);
}
 void handle_timer(AppContextRef ctx, AppTimerHandle handle, uint32_t cookie) {
	request_weather();
	// Update again in fifteen minutes.
	if(cookie)
		set_timer(ctx);
}

void http_success(int32_t request_id, int http_status, DictionaryIterator* received, void* context) {
  if (request_id != HTTP_COOKIE) {
    return;
  }
	
  Tuple* tuple1 = dict_find(received, 0);
/*	uint16_t value = tuple1->value->int16;
	int16_t temp = value & 0x3ff;
	if(value & 0x400) temp = -temp;
  static char temp_str[5];
  memcpy(temp_str, itoa(temp), 4);
  int degree_pos = strlen(temp_str);
  memcpy(&temp_str[degree_pos], "°", 3);*/
  text_layer_set_text(&layer_textTemp, tuple1->value->cstring);
 
  Tuple* tuple2 = dict_find(received, 1);
  text_layer_set_text(&layerConditions, tuple2->value->cstring);
 
  Tuple* tuple3 = dict_find(received, 2);
  text_layer_set_text(&layer_textImage, tuple3->value->cstring);
	
  Tuple* tuple4 = dict_find(received, 3);
  text_layer_set_text(&layerUpdated, tuple4->value->cstring);
 
  text_layer_set_text(&layer_textError, "");
	
  Tuple* tuple5 = dict_find(received, 4);
  set_icon(tuple5->value->cstring);
	if (havess == false)
	{
		//http_capture_send(10);
		havess = true;
	}
}
 
void http_failure(int32_t request_id, int http_status, void* context) {
  httpebble_error(http_status >= 1000 ? http_status - 1000 : http_status);
}
 
void window_appear(Window* me) {
 
//request_weather();  
}
void request_weather() 
{
	
	if(!located) 
	{
		http_location_request();
		return;
	}
DictionaryIterator* dict;
	char lat[50];
	char lon[50];
	snprintf(lat, sizeof(lat), "%d", our_latitude);
	snprintf(lon, sizeof(lon), "%d", our_longitude);
  //text_layer_set_text(&layerUpdated, lat);

  //text_layer_set_text(&layer_textError, lon);
	char str[200];
	strcpy(str, "http://viwebworks.net/weatherpage.aspx?lat=");
	strcat(str, lat);
	strcat(str, "&lon=");
	strcat(str, lon);
  HTTPResult  result = http_out_get(str, HTTP_COOKIE, &dict);
  if (result != HTTP_OK) {
	  set_icon(ERROR_CODE);
    httpebble_error(result);
    return;
  }
	dict_write_int32(dict, WEATHER_KEY_LATITUDE, our_latitude);
	dict_write_int32(dict, WEATHER_KEY_LONGITUDE, our_longitude);
	
   result = http_out_send();
  if (result != HTTP_OK) {
	  set_icon(ERROR_CODE);
    httpebble_error(result);
    return;
  }
}


void line_layer_update_callback(Layer *l, GContext *ctx)
{
    (void)l;

    graphics_context_set_stroke_color(ctx, FG_COLOR);
    graphics_draw_line(ctx, GPoint(0, 70), GPoint(144, 70));
    graphics_draw_line(ctx, GPoint(0, 71), GPoint(144, 71));
    graphics_draw_line(ctx, GPoint(0, 113), GPoint(144, 113));
    graphics_draw_line(ctx, GPoint(0, 114), GPoint(144, 114));
}
void handle_init(AppContextRef ctx) {
  http_set_app_id(2147483647);
 
    PblTm tm;
    PebbleTickEvent t;
    ResHandle res_d;
    ResHandle res_t;
    ResHandle res_te;
    ResHandle res_c;
    ResHandle res_ts;
    resource_init_current_app(&APP_RESOURCES);
  http_register_callbacks((HTTPCallbacks) {
    .success = http_success,
    .failure = http_failure,
    .reconnect=reconnect,
    .location=location
  }, (void*)ctx);
 
  window_init(&window, "Abe's Watchface");
  window_stack_push(&window, true);
    window_set_background_color(&window, BG_COLOR);
  window_set_window_handlers(&window, (WindowHandlers){
    .appear  = window_appear
  });
	
	
    res_d = resource_get_handle(RESOURCE_ID_FONT_ROBOTO_CONDENSED_20);
    res_t = resource_get_handle(RESOURCE_ID_FONT_ROBOTO_BOLD_SUBSET_41);
    res_te = resource_get_handle(RESOURCE_ID_FONT_FUTURA_TEMP_18);
    res_c = resource_get_handle(RESOURCE_ID_FONT_FUTURA_CONDITIONS_12);
    res_ts = resource_get_handle(RESOURCE_ID_FONT_FUTURA_TIMES_14);
    font_date = fonts_load_custom_font(res_d);
    font_time = fonts_load_custom_font(res_t);
    font_temp = fonts_load_custom_font(res_te);
    font_cond = fonts_load_custom_font(res_c);
    font_times = fonts_load_custom_font(res_ts);
    font_moon = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_MOON_PHASES_SUBSET_24));
	//output label
	
  text_layer_init(&layer_textTemp, GRect(5, 0, 75, 26));
  text_layer_set_text_color(&layer_textTemp, FG_COLOR);
  text_layer_set_background_color(&layer_textTemp, GColorClear);
  text_layer_set_font(&layer_textTemp, font_temp);
  text_layer_set_text_alignment(&layer_textTemp, GTextAlignmentLeft);
  layer_add_child(&window.layer, &layer_textTemp.layer);
 
  text_layer_init(&layer_textImage, GRect(44, 0, 100, 26));
  text_layer_set_text_color(&layer_textImage, FG_COLOR);
  text_layer_set_background_color(&layer_textImage, GColorClear);
  text_layer_set_font(&layer_textImage, font_cond);
  text_layer_set_text_alignment(&layer_textImage, GTextAlignmentRight);
  layer_add_child(&window.layer, &layer_textImage.layer);
	
  text_layer_init(&layerUpdated, GRect(2, 53, 75, 20));
  text_layer_set_text_color(&layerUpdated, FG_COLOR);
  text_layer_set_background_color(&layerUpdated, GColorClear);
  text_layer_set_font(&layerUpdated, font_cond);
  text_layer_set_text_alignment(&layerUpdated, GTextAlignmentLeft);
  layer_add_child(&window.layer, &layerUpdated.layer);
 
  text_layer_init(&layerConditions, GRect(44, 53, 99, 20));
  text_layer_set_text_color(&layerConditions, FG_COLOR);
  text_layer_set_background_color(&layerConditions, GColorClear);
  text_layer_set_font(&layerConditions, font_cond);
  text_layer_set_text_alignment(&layerConditions, GTextAlignmentRight);
  layer_add_child(&window.layer, &layerConditions.layer);
	
  text_layer_init(&layer_textSRise, GRect(5, 145, 75, 20));
  text_layer_set_text_color(&layer_textSRise, FG_COLOR);
  text_layer_set_background_color(&layer_textSRise, GColorClear);
  text_layer_set_font(&layer_textSRise, font_cond);
  text_layer_set_text_alignment(&layer_textSRise, GTextAlignmentLeft);
  layer_add_child(&window.layer, &layer_textSRise.layer);
 
  text_layer_init(&layer_textSSet, GRect(39, 145, 100, 20));
  text_layer_set_text_color(&layer_textSSet, FG_COLOR);
  text_layer_set_background_color(&layer_textSSet, GColorClear);
  text_layer_set_font(&layer_textSSet, font_cond);
  text_layer_set_text_alignment(&layer_textSSet, GTextAlignmentRight);
  layer_add_child(&window.layer, &layer_textSSet.layer);
	
	
    text_layer_init(&day_layer, GRect(5, 115, 144, 33));
    text_layer_set_font(&day_layer, font_date);
    text_layer_set_text_color(&day_layer, FG_COLOR);
    text_layer_set_background_color(&day_layer, GColorClear);
    text_layer_set_text_alignment(&day_layer, GTextAlignmentLeft);
    layer_add_child(&window.layer, &day_layer.layer);

    text_layer_init(&time_layer, GRect(5, 66, 114, 60));
    text_layer_set_text_color(&time_layer, FG_COLOR);
    text_layer_set_background_color(&time_layer, GColorClear);
    text_layer_set_font(&time_layer, font_time);
    layer_add_child(&window.layer, &time_layer.layer);

    text_layer_init(&secs_layer, GRect(119, 70, 144-116, 28));
    text_layer_set_font(&secs_layer, font_date);
    text_layer_set_text_color(&secs_layer, FG_COLOR);
    text_layer_set_background_color(&secs_layer, GColorClear);
    layer_add_child(&window.layer, &secs_layer.layer);

    text_layer_init(&ampm_layer, GRect(116, 89, 144-116, 28));
    text_layer_set_font(&ampm_layer, font_date);
    text_layer_set_text_color(&ampm_layer, FG_COLOR);
    text_layer_set_background_color(&ampm_layer, GColorClear);
    layer_add_child(&window.layer, &ampm_layer.layer);

    text_layer_init(&date_layer, GRect(1, 115, 144-1, 168-118));
    text_layer_set_font(&date_layer, font_date);
    text_layer_set_text_color(&date_layer, FG_COLOR);
    text_layer_set_background_color(&date_layer, GColorClear);
    text_layer_set_text_alignment(&date_layer, GTextAlignmentRight);
    layer_add_child(&window.layer, &date_layer.layer);

    text_layer_init(&moonPercent, GRect(0, 142, 144, 168-142));
    text_layer_set_text_color(&moonPercent, FG_COLOR);
    text_layer_set_background_color(&moonPercent, GColorClear);
    text_layer_set_font(&moonPercent, font_moon);
    text_layer_set_text_alignment(&moonPercent, GTextAlignmentCenter);
    layer_add_child(&window.layer, &moonPercent.layer);
	
  text_layer_init(&layer_textError, GRect(0, 35, 144, 26));
  text_layer_set_text_color(&layer_textError, FG_COLOR);
  text_layer_set_background_color(&layer_textError, GColorClear);
  text_layer_set_font(&layer_textError, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_text_alignment(&layer_textError, GTextAlignmentCenter);
  layer_add_child(&window.layer, &layer_textError.layer);

    layer_init(&line_layer, window.layer.frame);
    line_layer.update_proc = &line_layer_update_callback;
    layer_add_child(&window.layer, &line_layer);
	
    get_time(&tm);
    t.tick_time = &tm;
    t.units_changed = SECOND_UNIT | MINUTE_UNIT | HOUR_UNIT | DAY_UNIT;

	located = false;
    handle_second_tick(ctx, &t);
	handle_day(ctx, &t);
	set_timer(ctx);
	
	#if MAKE_SCREENSHOT
	http_capture_init(ctx);
	#endif
}
 
void httpebble_error(int error_code) {
 
  static char error_message[] = "UNKNOWN_HTTP_ERRROR_CODE_GENERATED";
 
  switch (error_code) {
    case HTTP_SEND_TIMEOUT:
      strcpy(error_message, "HTTP_SEND_TIMEOUT");
    break;
    case HTTP_SEND_REJECTED:
      strcpy(error_message, "HTTP_SEND_REJECTED");
    break;
    case HTTP_NOT_CONNECTED:
      strcpy(error_message, "HTTP_NOT_CONNECTED");
    break;
    case HTTP_BRIDGE_NOT_RUNNING:
      strcpy(error_message, "HTTP_BRIDGE_NOT_RUNNING");
    break;
    case HTTP_INVALID_ARGS:
      strcpy(error_message, "HTTP_INVALID_ARGS");
    break;
    case HTTP_BUSY:
      strcpy(error_message, "HTTP_BUSY");
    break;
    case HTTP_BUFFER_OVERFLOW:
      strcpy(error_message, "HTTP_BUFFER_OVERFLOW");
    break;
    case HTTP_ALREADY_RELEASED:
      strcpy(error_message, "HTTP_ALREADY_RELEASED");
    break;
    case HTTP_CALLBACK_ALREADY_REGISTERED:
      strcpy(error_message, "HTTP_CALLBACK_ALREADY_REGISTERED");
    break;
    case HTTP_CALLBACK_NOT_REGISTERED:
      strcpy(error_message, "HTTP_CALLBACK_NOT_REGISTERED");
    break;
    case HTTP_NOT_ENOUGH_STORAGE:
      strcpy(error_message, "HTTP_NOT_ENOUGH_STORAGE");
    break;
    case HTTP_INVALID_DICT_ARGS:
      strcpy(error_message, "HTTP_INVALID_DICT_ARGS");
    break;
    case HTTP_INTERNAL_INCONSISTENCY:
      strcpy(error_message, "HTTP_INTERNAL_INCONSISTENCY");
    break;
    case HTTP_INVALID_BRIDGE_RESPONSE:
      strcpy(error_message, "HTTP_INVALID_BRIDGE_RESPONSE");
    break;
    default: {
      strcpy(error_message, "HTTP_ERROR_UNKNOWN");
    }
  }
 set_icon(ERROR_CODE);
  text_layer_set_text(&layer_textError, error_message);
}