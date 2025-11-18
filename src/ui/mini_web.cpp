#include <iostream>
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <sstream>
#include "../common/state.hpp"
#include "../discovery/discovery.hpp"
#include "../client/client.hpp" 

const char* HTML_PAGE = R"HTML(
<!DOCTYPE html>
<html>
<head>
<title>SafeShare Node</title>
<style>
  body { font-family: sans-serif; padding: 20px; background: #f0f2f5; }
  .container { display: flex; gap: 20px; }
  .panel { flex: 1; background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); }
  .btn { padding: 5px 10px; cursor: pointer; background: #007bff; color: white; border: none; border-radius: 4px; margin-right:5px; }
  .btn-red { background: #dc3545; } .btn-green { background: #28a745; } .btn-grey { background: #6c757d; cursor: default; }
  .alert-box { background: #fff3cd; border: 1px solid #ffeeba; padding: 15px; margin-bottom: 10px; border-radius: 5px; }
  table { width: 100%; border-collapse: collapse; margin-top: 10px; }
  th, td { text-align: left; padding: 8px; border-bottom: 1px solid #eee; }
</style>
</head>
<body>

<h1>SafeShare Dashboard</h1>
<div id="alerts-area"></div>

<div class="container">
    <div class="panel">
        <h2>Client Mode (Neighbors)</h2>
        <button class="btn" onclick="scan()">🔄 Scan</button>
        <div id="peer-list"></div>
    </div>
    <div class="panel">
        <h2>Server Mode (Status)</h2>
        <p>🟢 Active on port 55001</p>
    </div>
</div>

<script>
// Store tokens in memory: { "192.168.1.5": "abc-123-token" }
let knownTokens = {};

// Load tokens from local storage if any (persistence across refresh)
try {
    if(localStorage.getItem('safeTokens')) {
        knownTokens = JSON.parse(localStorage.getItem('safeTokens'));
    }
} catch(e){}

setInterval(async () => {
    try {
        let res = await fetch('/api/requests');
        let reqs = await res.json();
        let html = "";
        reqs.forEach(r => {
            html += `<div class="alert-box">
                <strong>🔔 Request from ${r.name}</strong><br>Reason: ${r.reason}<br><br>
                <button class="btn btn-green" onclick="decide('${r.id}', true)">Accept</button>
                <button class="btn btn-red" onclick="decide('${r.id}', false)">Deny</button>
            </div>`;
        });
        document.getElementById('alerts-area').innerHTML = html;
    } catch(e){}
}, 1000);

async function decide(id, accept) {
    await fetch(`/api/respond?id=${id}&ans=` + (accept ? 'yes' : 'no'));
}

async function scan() {
    let res = await fetch('/api/scan');
    let peers = await res.json();
    renderPeers(peers);
}

function renderPeers(peers) {
    let html = "<table><tr><th>Name</th><th>IP</th><th>Action</th></tr>";
    peers.forEach(p => {
        let actionBtn = "";
        // Check if we already have a token for this IP
        if (knownTokens[p.ip]) {
            actionBtn = `<button class="btn btn-green" onclick="alert('Token: ' + knownTokens['${p.ip}'])">✅ Connected</button>`;
        } else {
            actionBtn = `<button class="btn" onclick="pair('${p.ip}', ${p.port})">Pair</button>`;
        }
        
        html += `<tr><td>${p.name}</td><td>${p.ip}</td><td>${actionBtn}</td></tr>`;
    });
    html += "</table>";
    document.getElementById('peer-list').innerHTML = html;
}

async function pair(ip, port) {
    let reason = prompt("Reason?", "File transfer");
    if(reason) {
        let res = await fetch(`/api/pair?ip=${ip}&port=${port}&reason=${encodeURIComponent(reason)}`);
        let json = await res.json();
        
        if(json.status === "ok") {
            alert("Paired Successfully!");
            // Save token
            knownTokens[ip] = json.token;
            localStorage.setItem('safeTokens', JSON.stringify(knownTokens));
            scan(); // Refresh UI to show "Connected"
        } else {
            alert("Pairing Failed: " + json.message);
        }
    }
}
</script>
</body>
</html>
)HTML";

void handle_http(int fd) {
    char buffer[4096];
    int r = read(fd, buffer, sizeof(buffer)-1);
    if(r<=0) { close(fd); return; }
    buffer[r] = 0;
    std::string req(buffer);
    
    std::string resp_body = "{}";
    std::string ctype = "text/html";

    if (req.find("GET / ") != std::string::npos) {
        resp_body = HTML_PAGE;
    }
    else if (req.find("GET /api/requests") != std::string::npos) {
        ctype = "application/json";
        auto list = GlobalState::get().getPending();
        resp_body = "[";
        for(size_t i=0; i<list.size(); i++) {
            resp_body += "{ \"id\": \"" + list[i].id + "\", \"name\": \"" + list[i].name + "\", \"reason\": \"" + list[i].reason + "\" }";
            if(i < list.size()-1) resp_body += ",";
        }
        resp_body += "]";
    }
    else if (req.find("GET /api/respond") != std::string::npos) {
        auto id_pos = req.find("id=");
        if(id_pos != std::string::npos) {
            std::string id = req.substr(id_pos+3, req.find("&", id_pos) - (id_pos+3));
            bool yes = (req.find("ans=yes") != std::string::npos);
            GlobalState::get().submitDecision(id, yes);
        }
        resp_body = "OK";
    }
    else if (req.find("GET /api/scan") != std::string::npos) {
        ctype = "application/json";
        auto fresh = scan_once(500); 
        GlobalState::get().mergePeers(fresh);
        auto full_list = GlobalState::get().getAllPeers();

        resp_body = "[";
        for(size_t i=0; i<full_list.size(); i++) {
            resp_body += "{ \"name\": \"" + full_list[i].name + "\", \"ip\": \"" + full_list[i].ip + "\", \"port\": " + std::to_string(full_list[i].port) + " }";
            if(i < full_list.size()-1) resp_body += ",";
        }
        resp_body += "]";
    }
    else if (req.find("GET /api/pair") != std::string::npos) {
        // CHANGED: Return JSON with Token
        ctype = "application/json";
        std::string ip, port_s, reason;
        auto p_ip = req.find("ip=");
        auto p_port = req.find("port=");
        auto p_reason = req.find("reason=");
        
        if (p_ip != std::string::npos && p_port != std::string::npos) {
             ip = req.substr(p_ip+3, req.find("&", p_ip)-(p_ip+3));
             port_s = req.substr(p_port+5, req.find("&", p_port)-(p_port+5));
             if(p_reason != std::string::npos)
                reason = req.substr(p_reason+7, req.find(" ", p_reason)-(p_reason+7));
             
             std::string token = send_perm_request(ip, std::stoi(port_s), "WebNode", reason);
             
             if (!token.empty()) {
                 resp_body = "{ \"status\": \"ok\", \"token\": \"" + token + "\" }";
             } else {
                 resp_body = "{ \"status\": \"error\", \"message\": \"Denied or Offline\" }";
             }
        } else {
            resp_body = "{ \"status\": \"error\", \"message\": \"Invalid Params\" }";
        }
    }

    std::string response = "HTTP/1.1 200 OK\r\nContent-Type: " + ctype + "\r\nContent-Length: " + std::to_string(resp_body.size()) + "\r\n\r\n" + resp_body;
    write(fd, response.c_str(), response.size());
    close(fd);
}

// ... start_web_server ...
void start_web_server() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    sockaddr_in address{};
    address.sin_family = AF_INET; address.sin_addr.s_addr = INADDR_ANY; address.sin_port = htons(8080);
    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 3);
    std::cout << "OPEN DASHBOARD: http://localhost:8080\n";
    while (true) {
        int new_socket = accept(server_fd, nullptr, nullptr);
        if (new_socket >= 0) std::thread(handle_http, new_socket).detach();
    }
}