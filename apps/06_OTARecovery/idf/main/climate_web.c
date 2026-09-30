#include "climate_web.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "climate_service.h"
#include "command_service.h"

#define TAG "APP18_WEB"

static bool query_value(httpd_req_t *req, const char *key, char *out, size_t out_len)
{
    char query[256];
    if (!req || !key || !out || out_len == 0) return false;
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK) return false;
    return httpd_query_key_value(query, key, out, out_len) == ESP_OK;
}

static bool query_long(httpd_req_t *req, const char *key, long *out)
{
    char value[24];
    if (!out || !query_value(req, key, value, sizeof(value))) return false;
    char *end = NULL;
    long v = strtol(value, &end, 10);
    if (!end || *end != '\0' || v < -32768 || v > 32767) return false;
    *out = v;
    return true;
}

static void set_no_cache(httpd_req_t *req)
{
    httpd_resp_set_hdr(req, "Cache-Control", "no-store, no-cache, must-revalidate");
}

static void binding(const char *name, char *out, size_t out_len)
{
    climate_service_format_binding(name, out, out_len);
}

static esp_err_t status_get(httpd_req_t *req)
{
    climate_config_t cfg = {0};
    climate_service_get_config(&cfg);

    char state[24], sensor[24], relay[24], temperature[24], humidity[24];
    char heater[24], cooler[24], humidifier[32], dehumidifier[32];
    char edit_state[24], error[40];

    binding("climate.state", state, sizeof(state));
    binding("climate.sensor_state", sensor, sizeof(sensor));
    binding("climate.relay_state", relay, sizeof(relay));
    binding("climate.temperature", temperature, sizeof(temperature));
    binding("climate.humidity", humidity, sizeof(humidity));
    binding("climate.heater", heater, sizeof(heater));
    binding("climate.cooler", cooler, sizeof(cooler));
    binding("climate.humidifier", humidifier, sizeof(humidifier));
    binding("climate.dehumidifier", dehumidifier, sizeof(dehumidifier));
    binding("climate.edit_state", edit_state, sizeof(edit_state));
    binding("climate.error", error, sizeof(error));

    char json[1024];
    int n = snprintf(
        json, sizeof(json),
        "{"
        "\"state\":\"%s\",\"sensor\":\"%s\",\"relay\":\"%s\","
        "\"temperature\":\"%s\",\"humidity\":\"%s\","
        "\"heater\":\"%s\",\"cooler\":\"%s\","
        "\"humidifier\":\"%s\",\"dehumidifier\":\"%s\","
        "\"edit_state\":\"%s\",\"error\":\"%s\","
        "\"config\":{"
        "\"t_min\":%d,\"t_max\":%d,\"t_hyst\":%d,"
        "\"rh_min\":%d,\"rh_max\":%d,\"rh_hyst\":%d"
        "}"
        "}",
        state, sensor, relay,
        temperature, humidity,
        heater, cooler, humidifier, dehumidifier,
        edit_state, error,
        cfg.t_min, cfg.t_max, cfg.t_hyst,
        cfg.rh_min, cfg.rh_max, cfg.rh_hyst);

    if (n < 0 || n >= (int)sizeof(json)) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "climate status too large");
        return ESP_OK;
    }

    httpd_resp_set_type(req, "application/json");
    set_no_cache(req);
    return httpd_resp_send(req, json, n);
}

static esp_err_t execute_and_reply(httpd_req_t *req, const char *command)
{
    char response[256];
    esp_err_t err = command_service_execute(command, response, sizeof(response));
    httpd_resp_set_type(req, "text/plain");
    set_no_cache(req);
    if (err != ESP_OK) httpd_resp_set_status(req, "409 Conflict");
    return httpd_resp_sendstr(req, response);
}

static esp_err_t command_post(httpd_req_t *req)
{
    char op[16];
    if (!query_value(req, "op", op, sizeof(op))) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "op required");
        return ESP_OK;
    }

    if (strcmp(op, "ON") == 0) return execute_and_reply(req, "CLIMATE ON");
    if (strcmp(op, "OFF") == 0) return execute_and_reply(req, "CLIMATE OFF");
    if (strcmp(op, "DEFAULTS") == 0) return execute_and_reply(req, "CLIMATE DEFAULTS");

    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "op must be ON, OFF or DEFAULTS");
    return ESP_OK;
}

static esp_err_t set_post(httpd_req_t *req)
{
    const char *keys[6] = {"t_min", "t_max", "t_hyst", "rh_min", "rh_max", "rh_hyst"};
    long v[6];

    for (unsigned i = 0; i < 6; ++i) {
        if (!query_long(req, keys[i], &v[i])) {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                                "all six settings are required as integer tenths");
            return ESP_OK;
        }
    }

    char command[128];
    snprintf(command, sizeof(command),
             "CLIMATE SET %ld %ld %ld %ld %ld %ld",
             v[0], v[1], v[2], v[3], v[4], v[5]);
    return execute_and_reply(req, command);
}

static esp_err_t page_get(httpd_req_t *req)
{
    static const char page[] =
        "<!doctype html><html><head>"
        "<meta name='viewport' content='width=device-width,initial-scale=1,viewport-fit=cover'>"
        "<title>Climate Controller</title>"
        "<style>"
        "*{box-sizing:border-box}body{font-family:Arial,sans-serif;max-width:900px;margin:0 auto;padding:14px;background:#0d1117;color:#eef}"
        "a{color:#58a6ff}.top,.card{background:#182027;border:1px solid #2d3944;border-radius:14px;padding:14px;margin:10px 0}"
        ".hero{display:grid;grid-template-columns:1fr 1fr;gap:10px}.metric{background:#111820;border-radius:12px;padding:16px;text-align:center}"
        ".value{font-size:32px;font-weight:800}.label{color:#9aa7b2;font-size:13px;text-transform:uppercase;letter-spacing:.05em}"
        ".status{display:grid;grid-template-columns:repeat(3,1fr);gap:8px}.pill{background:#111820;border-radius:10px;padding:10px;text-align:center}"
        ".outputs{display:grid;grid-template-columns:1fr 1fr;gap:8px}.out{background:#111820;border-radius:10px;padding:12px;font-weight:700}"
        ".actions{display:grid;grid-template-columns:1fr 1fr 1fr;gap:8px}"
        "button,input{width:100%;font-size:16px;padding:11px;border-radius:9px;border:1px solid #45525f}"
        "button{cursor:pointer;font-weight:800}.settings{display:grid;grid-template-columns:1fr 1fr;gap:12px}"
        ".field{display:grid;grid-template-columns:1fr 120px;gap:8px;align-items:center;margin:7px 0}.muted{color:#9aa7b2}"
        ".ok{color:#5ee38a}.warn{color:#f0ad4e}.bad{color:#ff7b72}"
        "#msg{min-height:22px;white-space:pre-wrap}"
        "@media(max-width:620px){body{padding:9px}.hero,.outputs,.settings{grid-template-columns:1fr 1fr}.status{grid-template-columns:1fr}.actions{grid-template-columns:1fr 1fr}.actions button:last-child{grid-column:1/-1}.value{font-size:28px}.field{grid-template-columns:1fr 105px}}"
        "</style></head><body>"
        "<p><a href='/'>KONTAKTS Platform</a></p>"
        "<div class='top'><h1>Climate Controller</h1>"
        "<p class='muted'>Phone / browser transport through the common climate service. AUTO ON reserves MA01 for climate control; AUTO OFF releases external relay control.</p>"
        "<div class='status'>"
        "<div class='pill'><div class='label'>Mode</div><b id='state'>--</b></div>"
        "<div class='pill'><div class='label'>Sensor</div><b id='sensor'>--</b></div>"
        "<div class='pill'><div class='label'>Relays</div><b id='relay'>--</b></div>"
        "</div></div>"
        "<div class='hero'>"
        "<div class='metric'><div class='label'>Temperature</div><div class='value'><span id='temp'>--</span> &deg;C</div></div>"
        "<div class='metric'><div class='label'>Humidity</div><div class='value'><span id='rh'>--</span> %RH</div></div>"
        "</div>"
        "<div class='card'><h2>Outputs</h2><div class='outputs'>"
        "<div class='out'>DO1 HEATER: <span id='do1'>--</span></div>"
        "<div class='out'>DO2 COOLER: <span id='do2'>--</span></div>"
        "<div class='out'>DO3 HUMIDIFIER: <span id='do3'>--</span></div>"
        "<div class='out'>DO4 DEHUMIDIFIER: <span id='do4'>--</span></div>"
        "</div></div>"
        "<div class='card'><h2>Control</h2><div class='actions'>"
        "<button onclick=\"cmd('ON')\">AUTO ON</button>"
        "<button onclick=\"cmd('OFF')\">AUTO OFF</button>"
        "<button onclick=\"defaults()\">DEFAULTS</button>"
        "</div><p id='msg' class='muted'></p></div>"
        "<div class='card'><h2>Limits</h2><div class='settings'>"
        "<div><h3>Temperature</h3>"
        "<div class='field'><label>MIN, &deg;C</label><input id='tmin' type='number' step='0.1' oninput='dirty=true'></div>"
        "<div class='field'><label>MAX, &deg;C</label><input id='tmax' type='number' step='0.1' oninput='dirty=true'></div>"
        "<div class='field'><label>HYST, &deg;C</label><input id='thyst' type='number' step='0.1' min='0.1' oninput='dirty=true'></div>"
        "</div><div><h3>Humidity</h3>"
        "<div class='field'><label>MIN, %RH</label><input id='rhmin' type='number' step='0.1' oninput='dirty=true'></div>"
        "<div class='field'><label>MAX, %RH</label><input id='rhmax' type='number' step='0.1' oninput='dirty=true'></div>"
        "<div class='field'><label>HYST, %RH</label><input id='rhhyst' type='number' step='0.1' min='0.1' oninput='dirty=true'></div>"
        "</div></div><button onclick='save()'>SAVE LIMITS</button>"
        "<p class='muted'>The same validation as the touchscreen is used. Values are persisted in NVS after a successful save.</p>"
        "</div>"
        "<div class='card'><div><b>Edit:</b> <span id='edit'>--</span></div><div><b>Error:</b> <span id='err'>--</span></div></div>"
        "<script>"
        "let dirty=false;"
        "const $=id=>document.getElementById(id);"
        "function one(v){return (Number(v)/10).toFixed(1)}"
        "function tint(el,v){el.className=(v==='ONLINE'||v==='AUTO'||v==='OFF')?'ok':(v.includes('WAIT')||v==='STALE'||v==='FAILSAFE')?'warn':v==='FAULT'?'bad':''}"
        "function applyCfg(c){if(dirty)return;$('tmin').value=one(c.t_min);$('tmax').value=one(c.t_max);$('thyst').value=one(c.t_hyst);$('rhmin').value=one(c.rh_min);$('rhmax').value=one(c.rh_max);$('rhhyst').value=one(c.rh_hyst)}"
        "async function refresh(){try{let r=await fetch('/api/climate/status',{cache:'no-store'});if(!r.ok)throw new Error(await r.text());let x=await r.json();"
        "$('state').textContent=x.state;$('sensor').textContent=x.sensor;$('relay').textContent=x.relay;tint($('state'),x.state);tint($('sensor'),x.sensor);tint($('relay'),x.relay);"
        "$('temp').textContent=x.temperature;$('rh').textContent=x.humidity;$('do1').textContent=x.heater;$('do2').textContent=x.cooler;$('do3').textContent=x.humidifier;$('do4').textContent=x.dehumidifier;"
        "$('edit').textContent=x.edit_state;$('err').textContent=x.error;applyCfg(x.config)}catch(e){$('msg').textContent='Status: '+e.message}}"
        "async function post(url){let r=await fetch(url,{method:'POST'});let t=await r.text();$('msg').textContent=t;if(!r.ok)throw new Error(t);await refresh()}"
        "async function cmd(op){try{await post('/api/climate/command?op='+op)}catch(e){}}"
        "async function defaults(){try{dirty=false;await post('/api/climate/command?op=DEFAULTS')}catch(e){}}"
        "function tenth(id){let v=Number($(id).value);if(!Number.isFinite(v))throw new Error('All six limits are required');return Math.round(v*10)}"
        "async function save(){try{let q=new URLSearchParams({t_min:tenth('tmin'),t_max:tenth('tmax'),t_hyst:tenth('thyst'),rh_min:tenth('rhmin'),rh_max:tenth('rhmax'),rh_hyst:tenth('rhhyst')});"
        "await post('/api/climate/set?'+q.toString());dirty=false;await refresh()}catch(e){$('msg').textContent=e.message}}"
        "refresh();setInterval(refresh,1000);"
        "</script></body></html>";

    httpd_resp_set_type(req, "text/html");
    set_no_cache(req);
    return httpd_resp_sendstr(req, page);
}

esp_err_t climate_web_register(httpd_handle_t server)
{
    if (!server) return ESP_ERR_INVALID_ARG;

    const httpd_uri_t handlers[] = {
        {.uri = "/climate", .method = HTTP_GET, .handler = page_get},
        {.uri = "/api/climate/status", .method = HTTP_GET, .handler = status_get},
        {.uri = "/api/climate/command", .method = HTTP_POST, .handler = command_post},
        {.uri = "/api/climate/set", .method = HTTP_POST, .handler = set_post},
    };

    for (size_t i = 0; i < sizeof(handlers) / sizeof(handlers[0]); ++i) {
        esp_err_t err = httpd_register_uri_handler(server, &handlers[i]);
        if (err != ESP_OK) return err;
    }
    return ESP_OK;
}
