#include <pebble.h>

#define TZ (-5)
#define SECONDS

// #define MY_UUID { 0xff, 0x40, 0xe8, 0xb0, 0x52, 0x99, 0x41, 0x4a, 0xa9, 0x3f, 0xcc, 0xc9, 0x09, 0x1d, 0x85, 0x5f }
// PBL_APP_INFO(MY_UUID, "Boulder", "N8VI", 1, 0 /* App version */, RESOURCE_ID_IMAGE_MENU_ICON, APP_INFO_WATCH_FACE);

static char *dows[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
static char *mons[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

Window *window;

TextLayer *text_date_layer;
TextLayer *text_time_layer;
TextLayer *text_UTC_layer;
TextLayer *text_batt_layer;
TextLayer *text_conn_layer;
TextLayer *text_dst_layer;

void handle_conn(bool connected)
{
  if (connected) {
    text_layer_set_text(text_conn_layer, "Conn");
  } else {
    text_layer_set_text(text_conn_layer, "Disc");
	vibes_double_pulse(); 	
    }
}

void handle_batt(BatteryChargeState charge)
{
  static char battstate[12];

  snprintf(battstate, 10, "%d%% %s%s", charge.charge_percent, charge.is_charging?"C":" ", charge.is_plugged?"P":" ");
  text_layer_set_text(text_batt_layer, battstate);
}

// Layer line_layer;
/*
void line_layer_update_callback(Layer *me, GContext* ctx) 
{
  (void)me;

  graphics_context_set_stroke_color(ctx, GColorWhite);

  graphics_draw_line(ctx, GPoint(8, 51), GPoint(131, 51));
  graphics_draw_line(ctx, GPoint(8, 52), GPoint(131, 52));
  graphics_draw_line(ctx, GPoint(8, 115), GPoint(131, 115));
  graphics_draw_line(ctx, GPoint(8, 116), GPoint(131, 116));

}
*/
int julian( int unixSecs )
{
   return (int)(( unixSecs / 86400.0 ) + 2440587.5);
}

struct tm *timet_to_tm(int id)
{
  unsigned int dsecs, y,m,epochdays;
  unsigned char mdays[] = {31,28,31,30,31,30,31,31,30,31,30,31};
  static struct tm ret;

  epochdays = id / 86400;
  ret.tm_wday = (epochdays + 4) %7;

  dsecs = id % 86400;

  ret.tm_hour = dsecs / 3600;
  ret.tm_min = (dsecs / 60) % 60;
  ret.tm_sec = dsecs % 60;

  ret.tm_year = 1970;

  for (y=1970; y<2106; y++) {
    if ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0) {
      if (epochdays < 366)
        break;
      epochdays -= 366;
      ret.tm_year++;
    } else {
      if (epochdays < 365)
        break;
      epochdays -= 365;
      ret.tm_year++;
      }
    }

  ret.tm_yday = epochdays;

  if ((ret.tm_year % 4 == 0 && ret.tm_year % 100 != 0) || ret.tm_year % 400 == 0) {
    mdays[1] = 29;
    }
  for (m=0; m<=11; m++) {
    if (epochdays < mdays[m]) {
      ret.tm_mday = epochdays+1;
      ret.tm_mon = m+1;
      break;
      }
    epochdays -= mdays[m];
    }

  return &ret;
}

int tm_to_timet(struct tm *cd)
{
  unsigned int yday,c,q,cq,e,epochdays;
  int m;
  unsigned char mdays[] = {31,28,31,30,31,30,31,31,30,31,30,31};
  static int result;

  yday = 0;
  if ((cd->tm_year % 4 == 0 && cd->tm_year % 100 != 0) || cd->tm_year % 400 == 0) {
    mdays[1] = 29;
    }
  for (m=0; m<cd->tm_mon-1; m++)
    yday += mdays[m];
  yday += cd->tm_mday;
  c = (cd->tm_year/100 - 19);
  cq = (cd->tm_year/400 - 4);
  q = (cd->tm_year - 1969) / 4;
  e = q-c+cq;
  epochdays = 365 * (cd->tm_year - 1970) + e + yday - 1;
  result = 3600*cd->tm_hour + 60*cd->tm_min + cd->tm_sec + (epochdays * 86400);
  return result;
}

/*
bool isDst(int local)
{
  int mar, nov;
  struct tm *tmp, tm;
  int year;
	
  tmp = timet_to_tm(local);
  tmp->tm_mon = 3;
  tmp->tm_mday = 1;
  mar = tm_to_timet(tmp);
  tmp = timet_to_tm(mar);
  mar = tmp->tm_wday;
  mar = (7-mar) % 7 + 1;
  mar += 7;
  tmp->tm_mday = mar;
  tmp->tm_hour = 2;
  tmp->tm_min = 0;
  mar = tm_to_timet(tmp);
	
  tmp = timet_to_tm(local);
  tmp->tm_mon = 11;
  tmp->tm_mday = 1;
  nov = tm_to_timet(tmp);
  tmp = timet_to_tm(nov);
  nov = tmp->tm_wday;
  nov = (7-nov) % 7 + 1;
  tmp->tm_mday = nov;
  tmp->tm_hour = 2;
  tmp->tm_min = 0;
  nov = tm_to_timet(tmp);
  
  return (local >= mar && local <= nov);
  // plus some sort of flag ... has this happened already, etc
}
*/

int gm_time(int ltime)
{
  struct tm *tm_dst;
  int tz = TZ;
	
  tm_dst = localtime((time_t*)&ltime);
  if (tm_dst->tm_isdst) {
	tz += 1; 
	}
	
  ltime -= tz*60*60;
	
  return ltime;
}

/*
int local_time(int utime)
{
  return utime + TZ*60*60;
}

*/

void update_display(int now) 
{
  static char time_text[] = "00:00";
  static char date_text[50];
  static char utc_text[50];
  char *dst_text;
  struct tm *tm_local, *tm_utc;
  int gm_now = gm_time(now);
  struct tm *tm_dst;
	
  tm_dst = localtime((time_t*)&now);
  if (tm_dst->tm_isdst) {
	dst_text = "DT"; 
  } else {
	dst_text = "ST";
	}
	
  tm_local = timet_to_tm(now);
  snprintf(time_text, 6, "%.2d:%.2d", tm_local->tm_hour, tm_local->tm_min);
  snprintf(date_text, 50, "%.2d:%.2d|%.2d:%.2d|%.2d:%.2d|%.2d:%.2d\n%s %d %s     %s",
		                  (tm_local->tm_hour+21)%24, tm_local->tm_min, 
		                  (tm_local->tm_hour+22)%24, tm_local->tm_min, 
		                  (tm_local->tm_hour+23)%24, tm_local->tm_min,
		                  tm_local->tm_hour  , tm_local->tm_min, 
		                  dows[tm_local->tm_wday], 
		                  tm_local->tm_mday, mons[tm_local->tm_mon-1],
		                  dst_text);
  tm_utc   = timet_to_tm(gm_now);
#ifdef SECONDS

  snprintf(utc_text, 50," %.2d-%.2d-%.2d  JD%d\n%.2d:%.2d:%.2dz     %d", 
		                tm_utc->tm_year,tm_utc->tm_mon,tm_utc->tm_mday,
		                julian(gm_now),
                        tm_utc->tm_hour,tm_utc->tm_min,tm_utc->tm_sec,
		                gm_now);	
	
#else
  snprintf(utc_text, 24," %.2d-%.2d-%.2d %.2d:%.2d UT", tm_utc->tm_year,tm_utc->tm_mon, tm_utc->tm_mday,
                                          tm_utc->tm_hour,tm_utc->tm_min);
#endif
  text_layer_set_text(text_date_layer, date_text);
  text_layer_set_text(text_time_layer, time_text);
  text_layer_set_text(text_UTC_layer, utc_text);
  // text_layer_set_text(text_dst_layer, dst_text);
}

static void handle_second_tick(struct tm *tick_time, TimeUnits units_changed) {
  time_t now = time(NULL);
  update_display(now);
}


void init()
{

  window = window_create();
  window_stack_push(window, true);
  window_set_background_color(window, GColorBlack);

  Layer *root_layer = window_get_root_layer(window);
  GRect frame = layer_get_frame(root_layer);

	//6, 30, 144-6, 168-30
  // wday mon mday
  text_date_layer = text_layer_create(GRect(0, 10, frame.size.w, frame.size.h-10));
  text_layer_set_text_color(text_date_layer, GColorWhite);
  text_layer_set_background_color(text_date_layer, GColorClear);
  text_layer_set_font(text_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(text_date_layer, GTextAlignmentCenter);
  text_layer_set_text(text_date_layer, "unix time");
  layer_add_child(root_layer, text_layer_get_layer(text_date_layer));	
	
  // time
	
  text_time_layer = text_layer_create(GRect(6, 47, frame.size.w-6, frame.size.h-47));
  text_layer_set_text_color(text_time_layer, GColorWhite);
  text_layer_set_background_color(text_time_layer, GColorClear);
  text_layer_set_font(text_time_layer, fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49));
  text_layer_set_text_alignment(text_time_layer, GTextAlignmentCenter);
  layer_add_child(root_layer, text_layer_get_layer(text_time_layer));	

	
  // UTC
  text_UTC_layer = text_layer_create(GRect(0, 100, frame.size.w, frame.size.h-100));
  text_layer_set_text_color(text_UTC_layer, GColorWhite);
  text_layer_set_background_color(text_UTC_layer, GColorClear);
  text_layer_set_font(text_UTC_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(text_UTC_layer, GTextAlignmentCenter);
  text_layer_set_text(text_UTC_layer, "unix time");
  layer_add_child(root_layer, text_layer_get_layer(text_UTC_layer));	
	
  // Batt
  text_batt_layer = text_layer_create(GRect(10,142,frame.size.w/2, frame.size.h-142));
  text_layer_set_text_color(text_batt_layer, GColorWhite);
  text_layer_set_background_color(text_batt_layer, GColorClear);
  text_layer_set_font(text_batt_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(text_batt_layer, GTextAlignmentLeft);
  text_layer_set_text(text_batt_layer, "Batt");
  layer_add_child(root_layer, text_layer_get_layer(text_batt_layer));	

	  // Conn
  text_conn_layer = text_layer_create(GRect(frame.size.w/2, 142, frame.size.w/2-10, frame.size.h-142));
  text_layer_set_text_color(text_conn_layer, GColorWhite);
  text_layer_set_background_color(text_conn_layer, GColorClear);
  text_layer_set_font(text_conn_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(text_conn_layer, GTextAlignmentRight);
  text_layer_set_text(text_conn_layer, "Conn?");
  layer_add_child(root_layer, text_layer_get_layer(text_conn_layer));	

	  // DST
/*
  text_dst_layer = text_layer_create(GRect(frame.size.w/2, 0, frame.size.w/2-10, frame.size.h-142));
  text_layer_set_text_color(text_dst_layer, GColorWhite);
  text_layer_set_background_color(text_dst_layer, GColorClear);
  text_layer_set_font(text_dst_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(text_dst_layer, GTextAlignmentRight);
  text_layer_set_text(text_dst_layer, "DST?");
  layer_add_child(root_layer, text_layer_get_layer(text_dst_layer));	
*/	
	// line
/*  layer_init(&line_layer, window.layer.frame);
  line_layer.update_proc = &line_layer_update_callback;
  layer_add_child(&window.layer, &line_layer);
*/
  // Avoid blank display on launch

  update_display(time(NULL));
	
#ifdef SECONDS	
  tick_timer_service_subscribe(SECOND_UNIT, handle_second_tick);
#else
  tick_timer_service_subscribe(MINUTE_UNIT, handle_second_tick);
#endif
  battery_state_service_subscribe(handle_batt);
  handle_batt(battery_state_service_peek());
  bluetooth_connection_service_subscribe(handle_conn);
  handle_conn(bluetooth_connection_service_peek());
}

void deinit(void) 
{
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();
  text_layer_destroy(text_date_layer);
  text_layer_destroy(text_time_layer);
  text_layer_destroy(text_UTC_layer);
  text_layer_destroy(text_batt_layer);
  text_layer_destroy(text_conn_layer);
  window_destroy(window);
}


int main(void) 
{
	  init();
	  app_event_loop();
	  deinit();
}




