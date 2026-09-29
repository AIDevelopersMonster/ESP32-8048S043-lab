#include "ma01_web.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "command_service.h"
#include "modbus_service.h"

#define TAG "APP15_WEB"

static bool query_value(httpd_req_t *req, const char *key, char *out, size_t out_len)
{
    char query[160];
    if (!req || !key || !out || out_len == 0) return false;
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK) return false;
    return httpd_query_key_value(query, key, out, out_len) == ESP_OK;
}

static void set_no_cache(httpd_req_t *req)
{
    httpd_resp_set_hdr(req, "Cache-Control", "no-store, no-cache, must-revalidate");
}

static esp_err_t build_state_json(char *out, size_t out_len, bool refresh)
{
    if (!out || out_len == 0) return ESP_ERR_INVALID_ARG;

    esp_err_t refresh_err = ESP_OK;
    if (refresh) {
        char response[512];
        refresh_err = command_service_execute("MA01 READ", response, sizeof(response));
    }

    modbus_service_status_t bus = {0};
    modbus_service_get_status(&bus);
    char ma01_state[16];
    modbus_service_format_binding("modbus.ma01.state", ma01_state, sizeof(ma01_state));
    bool online = strcmp(ma01_state, "ONLINE") == 0;

    bool states[8] = {0};
    for (unsigned i = 0; i < 8; ++i) {
        char binding[24];
        char value[12];
        snprintf(binding, sizeof(binding), "modbus.ma01.do%u", i + 1);
        modbus_service_format_binding(binding, value, sizeof(value));
        states[i] = strcmp(value, "ON") == 0;
    }

    snprintf(out, out_len,
             "{\"address\":%u,\"online\":%s,\"refresh_ok\":%s,"
             "\"tx\":%lu,\"rx\":%lu,\"timeouts\":%lu,\"crc_errors\":%lu,"
             "\"protocol_errors\":%lu,\"do\":[%s,%s,%s,%s,%s,%s,%s,%s],"
             "\"error\":\"%s\"}",
             (unsigned)modbus_service_ma01_get_slave(),
             online ? "true" : "false",
             refresh_err == ESP_OK ? "true" : "false",
             (unsigned long)bus.tx_frames,
             (unsigned long)bus.rx_frames,
             (unsigned long)bus.timeouts,
             (unsigned long)bus.crc_errors,
             (unsigned long)bus.protocol_errors,
             states[0] ? "true" : "false",
             states[1] ? "true" : "false",
             states[2] ? "true" : "false",
             states[3] ? "true" : "false",
             states[4] ? "true" : "false",
             states[5] ? "true" : "false",
             states[6] ? "true" : "false",
             states[7] ? "true" : "false",
             refresh_err == ESP_OK ? "" : esp_err_to_name(refresh_err));
    return refresh_err;
}

static esp_err_t build_config_json(char *out, size_t out_len)
{
    if (!out || out_len == 0) return ESP_ERR_INVALID_ARG;

    char response[1024];
    esp_err_t err = command_service_execute("MA01 CONFIG", response, sizeof(response));
    if (err != ESP_OK) return err;

    size_t used = (size_t)snprintf(out, out_len, "{\"address\":%u,\"channels\":[",
                                   (unsigned)modbus_service_ma01_get_slave());
    for (unsigned i = 0; i < 8 && used < out_len; ++i) {
        char mode_binding[28];
        char pulse_binding[28];
        char mode[16];
        char pulse[24];
        snprintf(mode_binding, sizeof(mode_binding), "modbus.ma01.mode%u", i + 1);
        snprintf(pulse_binding, sizeof(pulse_binding), "modbus.ma01.pulse%u", i + 1);
        modbus_service_format_binding(mode_binding, mode, sizeof(mode));
        modbus_service_format_binding(pulse_binding, pulse, sizeof(pulse));
        unsigned long pulse_ms = strtoul(pulse, NULL, 10);
        used += (size_t)snprintf(out + used, out_len - used,
                                 "%s{\"channel\":%u,\"mode\":\"%s\",\"pulse_ms\":%lu}",
                                 i ? "," : "", i + 1, mode, pulse_ms);
    }
    if (used < out_len) snprintf(out + used, out_len - used, "]}");
    return ESP_OK;
}

static esp_err_t page_get(httpd_req_t *req)
{
    static const char page[] =
        "<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>"
        "<title>MA01 Web Control</title>"
        "<style>"
        "body{font-family:Arial,sans-serif;max-width:1100px;margin:18px auto;padding:0 12px;background:#101418;color:#eef}"
        "a{color:#58a6ff}.top,.card{background:#182027;border:1px solid #2d3944;border-radius:14px;padding:14px;margin:10px 0}"
        ".grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:10px}.ch{background:#151c22;border:1px solid #34404a;border-radius:12px;padding:12px}"
        ".row{display:grid;grid-template-columns:repeat(3,1fr);gap:7px;margin-top:8px}"
        "button,select,input{font-size:15px;padding:9px;border-radius:8px;border:1px solid #45525f;box-sizing:border-box;width:100%}"
        "button{cursor:pointer;font-weight:700}.state{font-size:22px;font-weight:800}.on{color:#5ee38a}.off{color:#a9b3bc}.warn{color:#f0ad4e}.muted{color:#9aa7b2}"
        "@media(max-width:720px){.grid{grid-template-columns:1fr}.row{grid-template-columns:1fr 1fr}}"
        "</style></head><body>"
        "<p><a href='/'>← KONTAKTS Platform</a></p><h1>MA01 Web Control</h1>"
        "<div class='top'><b>Browser transport → command/service layer → MA01 provider → UART1/RS485</b>"
        "<p class='muted'>Local engineering UI. No authentication in this MVP; do not expose it to the public Internet.</p>"
        "<div id='bus'>Waiting for state...</div>"
        "<div class='row'><button onclick='scan()'>SCAN MA01</button><button onclick='refresh()'>READ ALL</button><input id='addr' type='number' min='1' max='247' placeholder='Slave address'><button onclick='setAddr()'>SET ADDRESS</button></div>"
        "<p id='msg' class='muted'></p></div>"
        "<div id='channels' class='grid'></div>"
        "<script>"
        "const C=document.getElementById('channels'),M=document.getElementById('msg');"
        "for(let i=1;i<=8;i++){let d=document.createElement('div');d.className='ch';d.innerHTML="
        "'<h2>DO'+i+'</h2><div id=\"s'+i+'\" class=\"state off\">--</div>' +"
        "'<div class=\"row\"><button onclick=\"cmd('+i+',\\'ACTION\\')\">ACTION</button><button onclick=\"cmd('+i+',\\'ON\\')\">ON</button><button onclick=\"cmd('+i+',\\'OFF\\')\">OFF</button></div>' +"
        "'<div class=\"row\"><select id=\"m'+i+'\"><option>LEVEL</option><option>PULSE</option></select><button onclick=\"mode('+i+')\">SET MODE</button><span></span></div>' +"
        "'<div class=\"row\"><input id=\"p'+i+'\" type=\"number\" min=\"0\" max=\"65535\" placeholder=\"Pulse ms\"><button onclick=\"pulse('+i+')\">SET PULSE</button><button onclick=\"cmd('+i+',\\'TOGGLE\\')\">TOGGLE</button></div>';"
        "C.appendChild(d)}"
        "function q(v){return encodeURIComponent(v)}"
        "async function post(url){let r=await fetch(url,{method:'POST'}),t=await r.text();M.textContent=t;if(!r.ok)throw new Error(t);await refresh();return t}"
        "function render(x){document.getElementById('addr').value=x.address||'';let b=document.getElementById('bus');"
        "b.innerHTML='<b>Address:</b> '+(x.address||'not set')+' &nbsp; <b>Bus:</b> '+(x.online?'ONLINE':'OFFLINE')+' &nbsp; <b>TX/RX:</b> '+x.tx+'/'+x.rx+' &nbsp; <b>Timeouts:</b> '+x.timeouts+(x.refresh_ok?'':' &nbsp; <span class=\"warn\">'+x.error+'</span>');"
        "for(let i=1;i<=8;i++){let e=document.getElementById('s'+i),on=!!x.do[i-1];e.textContent=x.online?(on?'ON':'OFF'):'--';e.className='state '+(on&&x.online?'on':'off')}}"
        "async function refresh(){try{let r=await fetch('/api/ma01/status'),x=await r.json();render(x);if(!x.refresh_ok)M.textContent=x.error}catch(e){M.textContent=e.message}}"
        "async function config(){try{let r=await fetch('/api/ma01/config'),t=await r.text();if(!r.ok)throw new Error(t);let x=JSON.parse(t);"
        "for(let c of x.channels){document.getElementById('m'+c.channel).value=c.mode;document.getElementById('p'+c.channel).value=c.pulse_ms}M.textContent='Configuration refreshed'}catch(e){M.textContent=e.message}}"
        "async function cmd(ch,op){try{await post('/api/ma01/command?ch='+ch+'&op='+op)}catch(e){}}"
        "async function mode(ch){try{await post('/api/ma01/command?ch='+ch+'&op=MODE&value='+q(document.getElementById('m'+ch).value));await config()}catch(e){}}"
        "async function pulse(ch){try{await post('/api/ma01/command?ch='+ch+'&op=PULSEMS&value='+q(document.getElementById('p'+ch).value));await config()}catch(e){}}"
        "async function scan(){try{await post('/api/ma01/scan');await config()}catch(e){}}"
        "async function setAddr(){try{await post('/api/ma01/address?value='+q(document.getElementById('addr').value));await config()}catch(e){}}"
        "refresh();config();"
        "</script></body></html>";
    httpd_resp_set_type(req, "text/html");
    set_no_cache(req);
    return httpd_resp_sendstr(req, page);
}

static esp_err_t status_get(httpd_req_t *req)
{
    char json[768];
    (void)build_state_json(json, sizeof(json), true);
    httpd_resp_set_type(req, "application/json");
    set_no_cache(req);
    return httpd_resp_sendstr(req, json);
}

static esp_err_t events_get(httpd_req_t *req)
{
    char json[768];
    (void)build_state_json(json, sizeof(json), false);

    char event[900];
    int n = snprintf(event, sizeof(event), "retry: 1500\nevent: state\ndata: %s\n\n", json);
    httpd_resp_set_type(req, "text/event-stream");
    httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
    httpd_resp_set_hdr(req, "Connection", "close");
    return httpd_resp_send(req, event, n);
}

static esp_err_t config_get(httpd_req_t *req)
{
    char json[1024];
    esp_err_t err = build_config_json(json, sizeof(json));
    if (err != ESP_OK) {
        char msg[96];
        snprintf(msg, sizeof(msg), "MA01 CONFIG failed: %s", esp_err_to_name(err));
        httpd_resp_set_status(req, "409 Conflict");
        httpd_resp_set_type(req, "text/plain");
        return httpd_resp_sendstr(req, msg);
    }
    httpd_resp_set_type(req, "application/json");
    set_no_cache(req);
    return httpd_resp_sendstr(req, json);
}

static esp_err_t execute_and_reply(httpd_req_t *req, const char *command)
{
    char response[1024];
    esp_err_t err = command_service_execute(command, response, sizeof(response));
    httpd_resp_set_type(req, "text/plain");
    set_no_cache(req);
    if (err != ESP_OK) httpd_resp_set_status(req, "409 Conflict");
    return httpd_resp_sendstr(req, response);
}

static esp_err_t command_post(httpd_req_t *req)
{
    char ch_text[8], op[16], value[24];
    if (!query_value(req, "ch", ch_text, sizeof(ch_text)) ||
        !query_value(req, "op", op, sizeof(op))) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "ch and op required");
        return ESP_OK;
    }

    char *end = NULL;
    long ch = strtol(ch_text, &end, 10);
    if (!end || *end || ch < 1 || ch > 8) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "channel must be 1..8");
        return ESP_OK;
    }

    char command[96];
    if (strcmp(op, "ACTION") == 0 || strcmp(op, "ON") == 0 ||
        strcmp(op, "OFF") == 0 || strcmp(op, "TOGGLE") == 0) {
        snprintf(command, sizeof(command), "MA01 DO%ld %s", ch, op);
    } else if (strcmp(op, "MODE") == 0) {
        if (!query_value(req, "value", value, sizeof(value)) ||
            (strcmp(value, "LEVEL") != 0 && strcmp(value, "PULSE") != 0)) {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "mode must be LEVEL or PULSE");
            return ESP_OK;
        }
        snprintf(command, sizeof(command), "MA01 DO%ld MODE %s", ch, value);
    } else if (strcmp(op, "PULSEMS") == 0) {
        if (!query_value(req, "value", value, sizeof(value))) {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "pulse value required");
            return ESP_OK;
        }
        char *pulse_end = NULL;
        long pulse_ms = strtol(value, &pulse_end, 10);
        if (!pulse_end || *pulse_end || pulse_ms < 0 || pulse_ms > 65535) {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "pulse must be 0..65535 ms");
            return ESP_OK;
        }
        snprintf(command, sizeof(command), "MA01 DO%ld PULSEMS %ld", ch, pulse_ms);
    } else {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "unsupported operation");
        return ESP_OK;
    }

    return execute_and_reply(req, command);
}

static esp_err_t scan_post(httpd_req_t *req)
{
    return execute_and_reply(req, "MA01 SCAN");
}

static esp_err_t address_post(httpd_req_t *req)
{
    char value[12];
    if (!query_value(req, "value", value, sizeof(value))) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "value required");
        return ESP_OK;
    }
    char *end = NULL;
    long address = strtol(value, &end, 10);
    if (!end || *end || address < 1 || address > 247) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "address must be 1..247");
        return ESP_OK;
    }
    char command[32];
    snprintf(command, sizeof(command), "MA01 ADDR %ld", address);
    return execute_and_reply(req, command);
}

esp_err_t ma01_web_register(httpd_handle_t server)
{
    if (!server) return ESP_ERR_INVALID_ARG;

    const httpd_uri_t handlers[] = {
        {.uri = "/ma01", .method = HTTP_GET, .handler = page_get},
        {.uri = "/api/ma01/status", .method = HTTP_GET, .handler = status_get},
        {.uri = "/api/ma01/events", .method = HTTP_GET, .handler = events_get},
        {.uri = "/api/ma01/config", .method = HTTP_GET, .handler = config_get},
        {.uri = "/api/ma01/command", .method = HTTP_POST, .handler = command_post},
        {.uri = "/api/ma01/scan", .method = HTTP_POST, .handler = scan_post},
        {.uri = "/api/ma01/address", .method = HTTP_POST, .handler = address_post},
    };

    for (size_t i = 0; i < sizeof(handlers) / sizeof(handlers[0]); ++i) {
        esp_err_t err = httpd_register_uri_handler(server, &handlers[i]);
        if (err != ESP_OK) return err;
    }
    return ESP_OK;
}
