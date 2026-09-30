#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "climate_service.h"
#include "modbus_service.h"
#include "nvs.h"
#include "freertos/semphr.h"
static int64_t now;
static bool coils[4], reserved, fail_write, fail_read, fail_save, wrong_mode, corrupt_echo;
static unsigned writes, snapshot_reads, transient_remaining;
static esp_err_t read_error = ESP_FAIL;
static modbus_service_status_t sensor = {.online=true, .sensor_online=true, .sensor_seen=true, .temperature_tenths_c=240, .humidity_tenths_rh=440};
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
esp_err_t modbus_service_climate_snapshot(bool c[4], bool require_level) {
    ++snapshot_reads;
    if (transient_remaining) { --transient_remaining; return ESP_ERR_INVALID_CRC; }
    if (fail_read) return read_error;
    memcpy(c,coils,sizeof(coils));
    return require_level && wrong_mode ? ESP_ERR_NOT_SUPPORTED : ESP_OK;
}
esp_err_t modbus_service_write_single_coil(uint8_t a,uint16_t c,bool on) { (void)a;assert(reserved);++writes;if(fail_write)return ESP_FAIL;coils[c]=on;assert(!(coils[0]&&coils[1]));assert(!(coils[2]&&coils[3]));return corrupt_echo ? ESP_ERR_INVALID_CRC : ESP_OK; }
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
    char reading[24];
    sensor.sensor_seen=false;
    climate_service_format_binding("climate.temperature",reading,sizeof(reading));assert(!strcmp(reading,"--"));
    climate_service_format_binding("climate.sensor_state",reading,sizeof(reading));assert(!strcmp(reading,"NO SENSOR"));
    sensor.sensor_seen=true;
    c.t_min=260; fail_save=true; assert(climate_service_set_config(&c)!=ESP_OK);
    climate_service_get_config(&c);assert(c.t_min==250);fail_save=false;
    char response[80];
    assert(climate_service_command("CLIMATE SET 250 300 5 450 600 20",response,sizeof(response))==ESP_OK);
    assert(stored[0]==1 && stored[1]==250 && stored[6]==20);
    assert(climate_service_command("CLIMATE SET 250 300 5 450 600 20 junk",response,sizeof(response))!=ESP_OK);
    assert(climate_service_command("CLIMATE SET 250 300 5 450 600",response,sizeof(response))!=ESP_OK);
    assert(climate_service_enable(true)==ESP_OK);fail_write=true;tick();state("STARTING");fail_write=false;corrupt_echo=true;tick();state("AUTO");assert(coils[0]&&coils[2]);corrupt_echo=false;
    unsigned before=writes;tick();assert(writes==before);
    assert(climate_service_adjust(0, 5)==ESP_OK);
    climate_service_get_config(&c);assert(c.t_min==255 && stored[1]==255);
    assert(climate_service_adjust(0, -5)==ESP_OK);
    climate_service_get_config(&c);assert(c.t_min==250 && stored[1]==250);
    assert(climate_service_adjust(2, -10)!=ESP_OK);
    climate_service_get_config(&c);assert(c.t_hyst==5);
    sensor.temperature_tenths_c=310;sensor.humidity_tenths_rh=650;tick();assert(!coils[0]&&!coils[1]&&!coils[2]&&!coils[3]);tick();assert(coils[1]&&coils[3]);
    sensor.online=false;sensor.sensor_online=false;before=writes;tick();state("SENSOR WAIT");
    assert(coils[1]&&coils[3]&&writes==before);
    climate_service_format_binding("climate.temperature", reading, sizeof(reading));assert(!strcmp(reading,"31.0"));
    climate_service_format_binding("climate.humidity", reading, sizeof(reading));assert(!strcmp(reading,"65.0"));
    climate_service_format_binding("climate.sensor_state", reading, sizeof(reading));assert(!strcmp(reading,"STALE"));
    sensor.online=true;sensor.sensor_online=true;tick();state("AUTO");assert(coils[1]&&coils[3]);
    climate_service_format_binding("climate.temperature", reading, sizeof(reading));assert(!strcmp(reading,"31.0"));
    sensor.online=false;sensor.sensor_online=false;
    for (int i=0;i<9;i++) { before=writes;tick();state("SENSOR WAIT");assert(writes==before); }
    tick();state("FAILSAFE");assert(!coils[1]&&!coils[3]);
    sensor.online=true;sensor.sensor_online=true;tick();state("AUTO");assert(coils[1]&&coils[3]);
    /* Two corrupted reads followed by a valid response never cause OFF. */
    transient_remaining=2;before=writes;unsigned reads_before=snapshot_reads;
    tick();state("AUTO");assert(writes==before && snapshot_reads==reads_before+3);
    /* Failed readback retains last confirmed DOs, without commanding from stale data. */
    read_error=ESP_ERR_INVALID_CRC;fail_read=true;before=writes;
    tick();state("RELAY WAIT");assert(writes==before && coils[1] && coils[3]);
    climate_service_format_binding("climate.cooler",reading,sizeof(reading));assert(!strcmp(reading,"ON (STALE)"));
    climate_service_format_binding("climate.relay_state",reading,sizeof(reading));assert(!strcmp(reading,"STALE"));
    fail_read=false;tick();state("AUTO");assert(writes==before);
    climate_service_format_binding("climate.cooler",reading,sizeof(reading));assert(!strcmp(reading,"ON"));
    /* Repeated bad reads expire at 60 seconds, then attempt OFF and latch fault. */
    fail_read=true;read_error=ESP_ERR_TIMEOUT;
    for (int i=0;i<9;i++) { before=writes;tick();state("RELAY WAIT");assert(writes==before); }
    tick();state("FAULT");assert(!coils[1]&&!coils[3]);assert(reserved);
    climate_service_format_binding("climate.cooler",reading,sizeof(reading));assert(!strcmp(reading,"ON (STALE)"));
    fail_read=false;tick();state("FAULT");assert(!reserved);
    climate_service_format_binding("climate.cooler",reading,sizeof(reading));assert(!strcmp(reading,"OFF"));
    climate_service_enable(false);tick();state("OFF");
    climate_service_enable(true);tick();tick();state("AUTO");assert(coils[1]&&coils[3]);
    read_error=ESP_FAIL;
    fail_read=true;fail_write=true;tick();state("FAULT");assert(climate_service_enable(true)!=ESP_OK);assert(reserved);
    fail_read=false;fail_write=false;tick();state("FAULT");assert(!coils[1]&&!coils[3]);assert(!reserved);
    before=writes;tick();assert(writes==before);
    climate_service_enable(false);tick();state("OFF");assert(!reserved);
    wrong_mode=true;climate_service_enable(true);tick();tick();state("FAULT");
    assert(!coils[0]&&!coils[1]&&!coils[2]&&!coils[3]);
    climate_service_enable(false);tick();state("OFF");assert(!reserved);
    puts("PASS: thresholds, equality, interlocks, NVS failure, live +/- edits, parser, startup, reversal delay, sensor loss, relay failure and explicit recovery");
}
