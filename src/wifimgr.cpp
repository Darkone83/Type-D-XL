#include "wifimgr.h"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include <FFat.h>
#include <DNSServer.h>
#include <esp_wifi.h>

static AsyncWebServer server(80);
namespace WiFiMgr {

static String ssid, password;
static Preferences prefs;
static DNSServer dnsServer;

enum class State { IDLE, CONNECTING, CONNECTED, PORTAL };
static State state = State::PORTAL;

static int connectAttempts = 0;
static const int maxAttempts = 10;
static unsigned long lastAttempt = 0;
static unsigned long retryDelay = 3000;

static void setAPConfig() {
    WiFi.softAPConfig(
        IPAddress(192, 168, 4, 1),
        IPAddress(192, 168, 4, 1),
        IPAddress(255, 255, 255, 0)
    );
}

void loadCreds() {
    prefs.begin("wifi", true);
    ssid = prefs.getString("ssid", "");
    password = prefs.getString("pass", "");
    prefs.end();
}

void saveCreds(const String& s, const String& p) {
    prefs.begin("wifi", false);
    prefs.putString("ssid", s);
    prefs.putString("pass", p);
    prefs.end();
}

void clearCreds() {
    prefs.begin("wifi", false);
    prefs.remove("ssid");
    prefs.remove("pass");
    prefs.end();
}

void startPortal() {
    WiFi.disconnect(true);
    delay(200);
    WiFi.mode(WIFI_AP_STA);
    delay(100);
    setAPConfig();

    bool apok = WiFi.softAP("Type D XL Setup", NULL, 1, 0);
    Serial.printf("[WiFiMgr] softAP result: %d, IP: %s\n", apok, WiFi.softAPIP().toString().c_str());
    delay(500);

    esp_wifi_set_ps(WIFI_PS_NONE);
    esp_wifi_start();

    if (!apok) {
        Serial.println("[WiFiMgr] softAP failed, retrying...");
        WiFi.softAPdisconnect(true);
        delay(200);
        apok = WiFi.softAP("Type D setup", NULL, 1, 0);
        delay(500);
    }

    IPAddress apIP = WiFi.softAPIP();
    dnsServer.start(53, "*", apIP);

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        String page = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>WiFi Setup</title>
    <meta name="viewport" content="width=320,initial-scale=1">
    <style>
        body {background:#111;color:#EEE;font-family:sans-serif;}
        .container {max-width:320px;margin:24px auto;background:#222;padding:2em;border-radius:8px;box-shadow:0 0 16px #0008;}
        input,select,button {width:100%;box-sizing:border-box;margin:.7em 0;padding:.5em;font-size:1.1em;border-radius:5px;border:1px solid #555;}
        .btn-primary {background:#299a2c;color:white;}
        .btn-danger {background:#a22;color:white;}
        .status {margin-top:1em;font-size:.95em;}
        label {display:block;margin-top:.5em;margin-bottom:.1em;}
    </style>
</head>
<body>
    <div class="container">
        <div style="width:100%;text-align:center;margin-bottom:1em">
            <span style="font-size:2em;font-weight:bold;">Type D XL Setup</span>
        </div>
        <form id="wifiForm">
            <label>WiFi Network</label>
            <select id="ssidDropdown" style="margin-bottom:1em;">
                <option value="">Please select a network</option>
            </select>
            <input type="text" id="ssid" placeholder="SSID" style="margin-bottom:1em;">
            <label>Password</label>
            <input type="password" id="pass" placeholder="WiFi Password">
            <button type="button" onclick="save()" class="btn-primary">Connect & Save</button>
            <button type="button" onclick="forget()" class="btn-danger">Forget WiFi</button>
        </form>
        <div class="status" id="status">Status: ...</div>
    </div>
    <script>
        function scan() {
            fetch('/scan').then(r => r.json()).then(list => {
                let dropdown = document.getElementById('ssidDropdown');
                dropdown.innerHTML = '';
                let defaultOpt = document.createElement('option');
                defaultOpt.value = '';
                defaultOpt.text = 'Please select a network';
                dropdown.appendChild(defaultOpt);
                list.forEach(ssid => {
                    let opt = document.createElement('option');
                    opt.value = ssid;
                    opt.text = ssid;
                    dropdown.appendChild(opt);
                });
                dropdown.onchange = function() {
                    document.getElementById('ssid').value = dropdown.value;
                };
            }).catch(() => {
                let dropdown = document.getElementById('ssidDropdown');
                dropdown.innerHTML = '';
                let opt = document.createElement('option');
                opt.value = '';
                opt.text = 'Scan failed';
                dropdown.appendChild(opt);
            });
        }

        // Initial scan, repeat every 2s
        window.onload = scan;
        setInterval(scan, 2000);

        function save() {
            let ssid = document.getElementById('ssid').value;
            let pass = document.getElementById('pass').value;
            fetch('/save', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({ssid:ssid,pass:pass})
            }).then(r => r.text()).then(t => {
                document.getElementById('status').innerText = t;
            });
        }

        function forget() {
            fetch('/forget').then(r => r.text()).then(t => {
                document.getElementById('status').innerText = t;
                document.getElementById('ssid').value = '';
                document.getElementById('pass').value = '';
            });
        }
    </script>
</body>
</html>
        )rawliteral";

        request->send(200, "text/html", page);
    });

    server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request){
        String stat;
        if (WiFi.status() == WL_CONNECTED)
            stat = "Connected to " + WiFi.SSID() + " - IP: " + WiFi.localIP().toString();
        else if (state == State::CONNECTING)
            stat = "Connecting to " + ssid + "...";
        else
            stat = "In portal mode";
        request->send(200, "text/plain", stat);
    });

    server.on("/connect", HTTP_GET, [](AsyncWebServerRequest *request){
        String ss, pw;
        if (request->hasParam("ssid")) ss = request->getParam("ssid")->value();
        if (request->hasParam("pass")) pw = request->getParam("pass")->value();
        if (ss.length() == 0) {
            request->send(400, "text/plain", "SSID missing");
            return;
        }
        saveCreds(ss, pw);
        ssid = ss;
        password = pw;
        state = State::CONNECTING;
        connectAttempts = 1;
        WiFi.begin(ssid.c_str(), password.c_str());
        request->send(200, "text/plain", "Connecting to: " + ssid);
    });

    server.on("/forget", HTTP_GET, [](AsyncWebServerRequest *request){
        clearCreds();
        ssid = ""; password = "";
        WiFi.disconnect();
        state = State::PORTAL;
        request->send(200, "text/plain", "WiFi credentials cleared.");
    });

    server.on("/debug/forget", HTTP_GET, [](AsyncWebServerRequest *request){
        clearCreds();
        ssid = "";
        password = "";
        WiFi.disconnect(true);
        state = State::PORTAL;
        Serial.println("[DEBUG] WiFi credentials cleared via /debug/forget");
        request->send(200, "text/plain", "WiFi credentials cleared (debug).");
    });

    server.on("/scan", HTTP_GET, [](AsyncWebServerRequest *request){
        int n = WiFi.scanNetworks();
        String json = "[";
        for (int i = 0; i < n; ++i) {
            if (i) json += ",";
            json += "\"" + WiFi.SSID(i) + "\"";
        }
        json += "]";
        request->send(200, "application/json", json);
    });

    server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request){},
    NULL,
    [](AsyncWebServerRequest *request, uint8_t* data, size_t len, size_t index, size_t total) {
        String body;
        for (size_t i = 0; i < len; ++i) body += (char)data[i];
        String ss, pw;

        // Parse JSON manually (since we don’t want to include ArduinoJson for just this)
        int ssidStart = body.indexOf("\"ssid\"");
        if (ssidStart != -1) {
            int colon = body.indexOf(':', ssidStart);
            int quote1 = body.indexOf('"', colon+1);
            int quote2 = body.indexOf('"', quote1+1);
            ss = body.substring(quote1+1, quote2);
        }
        int passStart = body.indexOf("\"pass\"");
        if (passStart != -1) {
            int colon = body.indexOf(':', passStart);
            int quote1 = body.indexOf('"', colon+1);
            int quote2 = body.indexOf('"', quote1+1);
            pw = body.substring(quote1+1, quote2);
        }

        if (ss.length() == 0) {
            request->send(400, "text/plain", "SSID missing");
            return;
        }
        saveCreds(ss, pw);
        ssid = ss;
        password = pw;
        state = State::CONNECTING;
        connectAttempts = 1;
        WiFi.begin(ssid.c_str(), password.c_str());
        request->send(200, "text/plain", "Connecting to: " + ssid);
    }
);

    auto cp = [](AsyncWebServerRequest *r){
        r->send(200, "text/html", "<meta http-equiv='refresh' content='0; url=/' />");
    };
    server.on("/generate_204", HTTP_GET, cp);
    server.on("/hotspot-detect.html", HTTP_GET, cp);
    server.on("/redirect", HTTP_GET, cp);
    server.on("/ncsi.txt", HTTP_GET, cp);
    server.on("/captiveportal", HTTP_GET, cp);
    server.onNotFound(cp);

    server.begin();
    state = State::PORTAL;
}

void stopPortal() {
    dnsServer.stop();
}

void tryConnect() {
    if (ssid.length() > 0) {
        WiFi.mode(WIFI_AP_STA);
        delay(100);
        WiFi.begin(ssid.c_str(), password.c_str());
        state = State::CONNECTING;
        connectAttempts = 1;
        lastAttempt = millis();
    } else {
        startPortal();
    }
}

void begin() {
    loadCreds();
    startPortal();
    if (ssid.length() > 0)
        tryConnect();
}

void loop() {
    dnsServer.processNextRequest();
    if (state == State::CONNECTING) {
        if (WiFi.status() == WL_CONNECTED) {
            state = State::CONNECTED;
            dnsServer.stop();
            WiFi.softAPdisconnect(true);
            Serial.println("[WiFiMgr] WiFi connected.");
            Serial.print("[WiFiMgr] IP Address: ");
            Serial.println(WiFi.localIP());  
        } else if (millis() - lastAttempt > retryDelay) {
            connectAttempts++;
            if (connectAttempts >= maxAttempts) {
                state = State::PORTAL;
                startPortal();
            } else {
                WiFi.disconnect();
                WiFi.begin(ssid.c_str(), password.c_str());
                lastAttempt = millis();
            }
        }
    }
}

void restartPortal() {
    startPortal();
}

void forgetWiFi() {
    clearCreds();
    startPortal();
}

void forgetWiFiFromSerial() {
    clearCreds();
    WiFi.disconnect(true);
    ssid = "";
    password = "";
    Serial.println("[SerialCmd] WiFi credentials forgotten.");
    startPortal();
}

bool isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

String getStatus() {
    if (isConnected()) return "Connected to: " + ssid;
    if (state == State::CONNECTING) return "Connecting to: " + ssid;
    return "Not connected";
}

} // namespace WiFiMgr
