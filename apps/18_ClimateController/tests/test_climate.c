#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "climate_service.h"
#include "modbus_service.h"
#include "nvs.h"
#include "freertos/semphr.h"
static int64_t now;
static bool coils[4], reserved, fail_write, fail_read, fail_save, wrong_mode;
static unsigned writes;
static modbus_service_status_t sensor = {.online=true, .temperature_tenths_c=240, .humidity_tenths_rh=440};
static int16_t stored[7];
int64_t esp_timer_get_time(void) { return now; }
SemaphoreHandle_t xSemaphoreCreateMutex(void) { return (void *)1; }
int xSemaphoreTake(SemaphoreHandle_t s, unsigned d) { (void)s; (void)d; return 1; }
int xSemaphoreGive(SemaphoreHandle_t s) { (void)s; return 1; }
const char *esp_err_to_name(esp_err_t e) { return e ? "ERROR" : "ESP_OK"; }
esp_err_t nvs_open(const char *n,int m,nvs_handle_t *h) { (void)n; (void)m; *h=1; return ESP_OK; }
esp_err_t nvs_get_blob(nvs_handle_t h,const char *k,void *v,size_t *n) { (void)h;(void)k;(void)v;(void)n;return ESP_ERR_NVS_NOT_FOUND; }
esp_err_t nvs_set_blob(nvs_handle_t h,const char *k,const void *v,size_t n) { (void)h;(void)k;assert(n==sizeof(stored));memcpy(stored,v,n);return ESP_OK; }
esp_err_t nvs_commit(nvs_handle_t h) { (void)h;return fail_save ? ESP_FAIL : ESP_OK; }
void nvs_close(nvs_handle_t h) { (void)h; }
void modbus_service_get_status(modbus_service_status_t *s) { *s=sensor; }
uint8_t modbus_service_ma01_get_slave(void) { return 2; }
esp_err_t modbus_service_climate_claim(bool c) { reserved=c;return ESP_OK; }
esp_err_t modbus_service_climate_snapshot(bool c[4], bool require_level) { memcpy(c,coils,sizeof(coils));return fail_read ? ESP_FAIL : (require_level && wrong_mode) ? ESP_ERR_NOT_SUPPORTED : ESP_OK; }
esp_err_t modbus_service_write_single_coil(uint8_t a,uint16_t c,bool on) { (void)a;assert(reserved);++writes;if(fail_write)return ESP_FAIL;coils[c]=on;assert(!(coils[0]&&coils[1]));assert(!(coils[2]&&coils[3]));return ESP_OK; }
esp_err_t modbus_service_ma01_set(uint8_t c,bool on) { return modbus_service_write_single_coil(2,c-1,on); }
static void tick(void) { now+=6000000;climate_service_poll(); }
static void state(const char *expected) { char s[32];climate_service_format_binding("climate.state",s,sizeof(s));assert(!strcmp(s,expected)); }
int main(void) {
    climate_config_t c=climate_defaults();
    assert(c.t_min==250 && c.t_max==300 && c.t_hyst==5 && c.rh_min==450 && c.rh_max==600 && c.rh_hyst==20);
    bool d[4]={0};
    climate_evaluate(&c,249,449,d); assert(d[0]&&d[2]);
    climate_evaluate(&c,255,470,d); assert(d[0]&&d[2]);
    climate_evaluate(&c,256,471,d); assert(!d[0]&&!d[2]);
    climate_evaluate(&c,301,601,d); assert(d[1]&&d[3]);
    climate_evaluate(&c,295,580,d); assert(d[1]&&d[3]);
    climate_evaluate(&c,294,579,d); assert(!d[1]&&!d[3]);
    for(int t=-400;t<=1250;t++) for(int rh=0;rh<=1000;rh+=10) {
        climate_evaluate(&c,t,rh,d);assert(!(d[0]&&d[1]));assert(!(d[2]&&d[3]));
    }
    assert(climate_service_init()==ESP_OK);tick();state("OFF");assert(writes==0);
    c.t_min=260; fail_save=true; assert(climate_service_set_config(&c)!=ESP_OK);
    climate_service_get_config(&c);assert(c.t_min==250);fail_save=false;
    char response[80];
    assert(climate_service_command("CLIMATE SET 250 300 5 450 600 20",response,sizeof(response))==ESP_OK);
    assert(stored[0]==1 && stored[1]==250 && stored[6]==20);
    assert(climate_service_command("CLIMATE SET 250 300 5 450 600 20 junk",response,sizeof(response))!=ESP_OK);
    assert(climate_service_command("CLIMATE SET 250 300 5 450 600",response,sizeof(response))!=ESP_OK);
    assert(climate_service_enable(true)==ESP_OK);tick();state("STARTING");tick();state("AUTO");assert(coils[0]&&coils[2]);
    unsigned before=writes;tick();assert(writes==before);
    assert(climate_service_adjust(0, 5)==ESP_OK);
    climate_service_get_config(&c);assert(c.t_min==255 && stored[1]==255);
    assert(climate_service_adjust(0, -5)==ESP_OK);
    climate_service_get_config(&c);assert(c.t_min==250 && stored[1]==250);
    assert(climate_service_adjust(2, -10)!=ESP_OK);
    climate_service_get_config(&c);assert(c.t_hyst==5);
    sensor.temperature_tenths_c=310;sensor.humidity_tenths_rh=650;tick();assert(!coils[0]&&!coils[1]&&!coils[2]&&!coils[3]);tick();assert(coils[1]&&coils[3]);
    sensor.online=false;tick();state("FAILSAFE");assert(!coils[1]&&!coils[3]);
    sensor.online=true;tick();assert(coils[1]&&coils[3]);
    fail_read=true;fail_write=true;tick();state("FAULT");assert(climate_service_enable(true)!=ESP_OK);assert(reserved);
    fail_read=false;fail_write=false;tick();state("FAULT");assert(!coils[1]&&!coils[3]);
    climate_service_enable(false);tick();state("OFF");assert(!reserved);
    wrong_mode=true;climate_service_enable(true);tick();tick();state("FAULT");
    assert(!coils[0]&&!coils[1]&&!coils[2]&&!coils[3]);
    climate_service_enable(false);tick();state("OFF");assert(!reserved);
    puts("PASS: thresholds, equality, interlocks, NVS failure, live +/- edits, parser, startup, reversal delay, sensor loss, relay failure and explicit recovery");
}
