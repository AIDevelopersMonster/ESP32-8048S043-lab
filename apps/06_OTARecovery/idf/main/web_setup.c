#include "web_setup.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_check.h"
#include "esp_heap_caps.h"
#include "esp_http_server.h"
#include "esp_log.h"

#include "network_manager.h"
#include "ota_manager.h"
#include "storage_credentials.h"
#include "storage_fs.h"
#include "widget_runtime.h"

#define TAG "APP07_WEB"
static httpd_handle_t s_httpd;

static void url_decode(char *dst,size_t dst_len,const char *src){size_t di=0;for(size_t i=0;src[i]&&di+1<dst_len;i++){if(src[i]=='+')dst[di++]=' ';else if(src[i]=='%'&&src[i+1]&&src[i+2]){char h[3]={src[i+1],src[i+2],0};dst[di++]=(char)strtol(h,NULL,16);i+=2;}else dst[di++]=src[i];}dst[di]='\0';}
static bool form_value(const char *body,const char *key,char *out,size_t out_len){char needle[40];snprintf(needle,sizeof(needle),"%s=",key);const char *p=strstr(body,needle);if(!p)return false;p+=strlen(needle);const char *end=strchr(p,'&');size_t n=end?(size_t)(end-p):strlen(p);char encoded[128];if(n>=sizeof(encoded))n=sizeof(encoded)-1;memcpy(encoded,p,n);encoded[n]='\0';url_decode(out,out_len,encoded);return true;}
static esp_err_t redirect_root(httpd_req_t *req,const char *msg){httpd_resp_set_status(req,"303 See Other");httpd_resp_set_hdr(req,"Location","/");return httpd_resp_sendstr(req,msg);}

static esp_err_t root_get(httpd_req_t *req)
{
    ota_status_t ota; ota_manager_get_status(&ota); widget_info_t wi; widget_runtime_get_info(&wi);
    size_t fs_total=0,fs_used=0; storage_fs_info(&fs_total,&fs_used);
    bool setup=network_manager_state()==NETWORK_STATE_AP_SETUP;
    char *html=heap_caps_calloc(1,14000,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT); if(!html) html=calloc(1,14000); if(!html)return ESP_ERR_NO_MEM;
    int n=snprintf(html,14000,
      "<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'><title>KONTAKTS Platform</title>"
      "<style>body{font-family:sans-serif;max-width:820px;margin:20px auto;padding:0 14px;background:#101418;color:#eef}input,select,button{font-size:16px;padding:10px;margin:5px 0;width:100%%;box-sizing:border-box}button{cursor:pointer}.card{background:#182027;padding:16px;border-radius:14px;margin:12px 0}code{word-break:break-all}.grid{display:grid;grid-template-columns:1fr 1fr;gap:10px}@media(max-width:600px){.grid{grid-template-columns:1fr}}</style></head><body>"
      "<h1>KONTAKTS Platform</h1><div class='card'><b>Network:</b> %s<br><b>STA IP:</b> %s<br><b>Setup AP:</b> %s</div>",
      network_manager_state_name(),network_manager_sta_ip(),network_manager_ap_ssid());
    if(setup){n+=snprintf(html+n,14000-n,"<div class='card'><h2>Wi-Fi setup</h2><select id='ssid' name='ssid' form='wf'><option>Scanning...</option></select><form id='wf' method='post' action='/save'><input type='password' name='password' placeholder='Password'><button>Save and connect</button></form></div><script>fetch('/scan').then(r=>r.json()).then(a=>{let s=document.getElementById('ssid');s.innerHTML='';a.forEach(x=>{let o=document.createElement('option');o.name='ssid';o.value=x.ssid;o.textContent=x.ssid+' ('+x.rssi+' dBm)';s.appendChild(o)});s.setAttribute('name','ssid')})</script>");}
    else{
      n+=snprintf(html+n,14000-n,
       "<div class='card'><h2>GitHub OTA</h2><div id='ota'><b>State:</b> %s<br><b>Installed:</b> %s<br><b>Available:</b> %s<br><b>Running:</b> %s<br><b>Image:</b> %s<br><b>Progress:</b> %d%%%%<br><b>Message:</b> %s</div><div class='grid'><form method='post' action='/ota/check'><button>CHECK GITHUB</button></form><form method='post' action='/ota/install'><button>DOWNLOAD & INSTALL</button></form><form method='post' action='/ota/confirm'><button>CONFIRM</button></form><form method='post' action='/ota/rollback'><button>ROLLBACK</button></form></div><form method='post' action='/ota/recovery'><button>FACTORY RECOVERY</button></form></div>",
       ota.state,ota.current_version,ota.available_version[0]?ota.available_version:"-",ota.running_partition,ota.image_state,ota.progress_percent,ota.message);
      n+=snprintf(html+n,14000-n,
       "<div class='card'><h2>Filesystem Widget</h2><div id='widget'><b>Installed:</b> %s<br><b>ID:</b> %s<br><b>Name:</b> %s<br><b>Version:</b> %s<br><b>File:</b> %u bytes<br><b>Generation:</b> %u<br><b>Status:</b> %s<br><b>SPIFFS:</b> %u / %u bytes used</div><p>Select a JSON document (max 32 KiB). It is validated before replacing <code>/storage/widget.json</code>.</p><input id='widgetFile' type='file' accept='.json,application/json'><button onclick='installWidget()'>UPLOAD & INSTALL WIDGET</button><form method='post' action='/widget/delete'><button>DELETE EXTERNAL WIDGET</button></form><p id='uploadMsg'></p></div>",
       wi.installed?"yes":"no",wi.id,wi.name,wi.version,(unsigned)wi.file_size,(unsigned)wi.generation,wi.status,(unsigned)fs_used,(unsigned)fs_total);
      n+=snprintf(html+n,14000-n,
       "<div class='card'><form method='post' action='/clear'><button>Clear saved Wi-Fi and use setup AP</button></form></div><script>async function installWidget(){let f=document.getElementById('widgetFile').files[0];if(!f){uploadMsg.textContent='Choose JSON first';return}let t=await f.text();let r=await fetch('/widget/install',{method:'POST',headers:{'Content-Type':'application/json'},body:t});uploadMsg.textContent=await r.text();if(r.ok)setTimeout(()=>location.reload(),400)}setInterval(()=>fetch('/ota/status').then(r=>r.json()).then(x=>{document.getElementById('ota').innerHTML='<b>State:</b> '+x.state+'<br><b>Installed:</b> '+x.current_version+'<br><b>Available:</b> '+x.available_version+'<br><b>Running:</b> '+x.running_partition+'<br><b>Image:</b> '+x.image_state+'<br><b>Progress:</b> '+x.progress_percent+'%%<br><b>Message:</b> '+x.message}).catch(()=>{}),2000)</script>");
    }
    n+=snprintf(html+n,14000-n,"</body></html>"); httpd_resp_set_type(req,"text/html");esp_err_t err=httpd_resp_send(req,html,n);free(html);return err;
}

static esp_err_t favicon_get(httpd_req_t *req){httpd_resp_set_status(req,"204 No Content");return httpd_resp_send(req,NULL,0);}
static esp_err_t status_get(httpd_req_t *req){char json[192];int n=snprintf(json,sizeof(json),"{\"state\":\"%s\",\"ip\":\"%s\",\"ap\":\"%s\"}",network_manager_state_name(),network_manager_sta_ip(),network_manager_ap_ssid());httpd_resp_set_type(req,"application/json");return httpd_resp_send(req,json,n);}
static esp_err_t ota_status_get(httpd_req_t *req){ota_status_t s;ota_manager_get_status(&s);char json[768];int n=snprintf(json,sizeof(json),"{\"state\":\"%s\",\"current_version\":\"%s\",\"available_version\":\"%s\",\"running_partition\":\"%s\",\"image_state\":\"%s\",\"progress_percent\":%d,\"busy\":%s,\"update_available\":%s,\"pending_verify\":%s,\"message\":\"%s\"}",s.state,s.current_version,s.available_version,s.running_partition,s.image_state,s.progress_percent,s.busy?"true":"false",s.update_available?"true":"false",s.pending_verify?"true":"false",s.message);httpd_resp_set_type(req,"application/json");return httpd_resp_send(req,json,n);}
static esp_err_t widget_status_get(httpd_req_t *req){widget_info_t s;widget_runtime_get_info(&s);char json[512];int n=snprintf(json,sizeof(json),"{\"installed\":%s,\"generation\":%u,\"file_size\":%u,\"id\":\"%s\",\"name\":\"%s\",\"version\":\"%s\",\"status\":\"%s\"}",s.installed?"true":"false",(unsigned)s.generation,(unsigned)s.file_size,s.id,s.name,s.version,s.status);httpd_resp_set_type(req,"application/json");return httpd_resp_send(req,json,n);}
static esp_err_t scan_get(httpd_req_t *req){char *json=NULL;esp_err_t err=network_manager_scan_json(&json);if(err==ESP_ERR_INVALID_STATE){httpd_resp_set_status(req,"409 Conflict");return httpd_resp_sendstr(req,"scan available only in AP setup mode");}if(err!=ESP_OK){httpd_resp_send_err(req,HTTPD_500_INTERNAL_SERVER_ERROR,"scan failed");return ESP_OK;}httpd_resp_set_type(req,"application/json");esp_err_t e=httpd_resp_sendstr(req,json);free(json);return e;}

static esp_err_t save_post(httpd_req_t *req){if(req->content_len<=0||req->content_len>=256){httpd_resp_send_err(req,HTTPD_400_BAD_REQUEST,"invalid body");return ESP_OK;}char body[256];int got=httpd_req_recv(req,body,req->content_len);if(got<=0)return ESP_FAIL;body[got]='\0';char ssid[33]={0},password[65]={0};if(!form_value(body,"ssid",ssid,sizeof(ssid))||!ssid[0]){httpd_resp_send_err(req,HTTPD_400_BAD_REQUEST,"SSID required");return ESP_OK;}form_value(body,"password",password,sizeof(password));ESP_RETURN_ON_ERROR(storage_credentials_save(ssid,password),TAG,"save failed");ESP_RETURN_ON_ERROR(network_manager_connect(ssid,password),TAG,"connect failed");return redirect_root(req,"Saved");}
static esp_err_t clear_post(httpd_req_t *req){ESP_RETURN_ON_ERROR(storage_credentials_clear(),TAG,"clear failed");network_manager_disconnect_sta();ESP_RETURN_ON_ERROR(network_manager_enter_ap_setup(),TAG,"AP failed");return redirect_root(req,"Cleared");}
static esp_err_t ota_response(httpd_req_t *req,esp_err_t err){if(err!=ESP_OK){httpd_resp_set_status(req,"409 Conflict");char msg[96];snprintf(msg,sizeof(msg),"OTA action rejected: %s",esp_err_to_name(err));return httpd_resp_sendstr(req,msg);}return redirect_root(req,"OTA action accepted");}
static esp_err_t ota_check_post(httpd_req_t *r){return ota_response(r,ota_manager_start_check());}static esp_err_t ota_install_post(httpd_req_t *r){return ota_response(r,ota_manager_start_install());}static esp_err_t ota_confirm_post(httpd_req_t *r){return ota_response(r,ota_manager_confirm_running());}static esp_err_t ota_rollback_post(httpd_req_t *r){return ota_response(r,ota_manager_start_rollback());}static esp_err_t ota_recovery_post(httpd_req_t *r){return ota_response(r,ota_manager_start_recovery());}

static esp_err_t widget_install_post(httpd_req_t *req)
{
    if(req->content_len<=0||req->content_len>WIDGET_MAX_JSON_BYTES){httpd_resp_send_err(req,HTTPD_400_BAD_REQUEST,"widget must be 1..32768 bytes");return ESP_OK;}
    char *json=heap_caps_malloc((size_t)req->content_len+1,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);if(!json)json=malloc((size_t)req->content_len+1);if(!json){httpd_resp_send_err(req,HTTPD_500_INTERNAL_SERVER_ERROR,"no memory");return ESP_OK;}
    int total=0;while(total<req->content_len){int got=httpd_req_recv(req,json+total,req->content_len-total);if(got<=0){free(json);return ESP_FAIL;}total+=got;}json[total]='\0';
    char reason[128];esp_err_t err=widget_runtime_install_json(json,(size_t)total,reason,sizeof(reason));free(json);
    if(err!=ESP_OK){httpd_resp_set_status(req,"400 Bad Request");httpd_resp_set_type(req,"text/plain");return httpd_resp_sendstr(req,reason);}
    httpd_resp_set_type(req,"text/plain");return httpd_resp_sendstr(req,"INSTALL PASS - widget rendered live and persisted to /storage/widget.json");
}
static esp_err_t widget_delete_post(httpd_req_t *req){esp_err_t err=widget_runtime_delete();if(err!=ESP_OK){httpd_resp_send_err(req,HTTPD_500_INTERNAL_SERVER_ERROR,"delete failed");return ESP_OK;}return redirect_root(req,"Widget deleted");}

esp_err_t web_setup_start(void)
{
    httpd_config_t config=HTTPD_DEFAULT_CONFIG();config.max_uri_handlers=20;config.stack_size=8192;
    ESP_RETURN_ON_ERROR(httpd_start(&s_httpd,&config),TAG,"httpd_start failed");
    const httpd_uri_t h[]={
      {.uri="/",.method=HTTP_GET,.handler=root_get},{.uri="/favicon.ico",.method=HTTP_GET,.handler=favicon_get},{.uri="/status",.method=HTTP_GET,.handler=status_get},{.uri="/scan",.method=HTTP_GET,.handler=scan_get},{.uri="/save",.method=HTTP_POST,.handler=save_post},{.uri="/clear",.method=HTTP_POST,.handler=clear_post},
      {.uri="/ota/status",.method=HTTP_GET,.handler=ota_status_get},{.uri="/ota/check",.method=HTTP_POST,.handler=ota_check_post},{.uri="/ota/install",.method=HTTP_POST,.handler=ota_install_post},{.uri="/ota/confirm",.method=HTTP_POST,.handler=ota_confirm_post},{.uri="/ota/rollback",.method=HTTP_POST,.handler=ota_rollback_post},{.uri="/ota/recovery",.method=HTTP_POST,.handler=ota_recovery_post},
      {.uri="/widget/status",.method=HTTP_GET,.handler=widget_status_get},{.uri="/widget/install",.method=HTTP_POST,.handler=widget_install_post},{.uri="/widget/delete",.method=HTTP_POST,.handler=widget_delete_post},
    };
    for(size_t i=0;i<sizeof(h)/sizeof(h[0]);++i)ESP_RETURN_ON_ERROR(httpd_register_uri_handler(s_httpd,&h[i]),TAG,"URI handler failed");
    ESP_LOGI(TAG,"Platform HTTP server started with widget install API");return ESP_OK;
}
