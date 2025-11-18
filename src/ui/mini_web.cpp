#include <iostream>
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <sstream>
#include <filesystem> // Added for creating directories
#include "../common/state.hpp"
#include "../discovery/discovery.hpp"
#include "../client/client.hpp" 

// Use std::filesystem alias if not already defined in included headers
namespace fs = std::filesystem;

const char* HTML_PAGE = R"HTML(
<!DOCTYPE html>
<html>
<head>
<title>SafeShare Node</title>
<style>
  body { font-family: sans-serif; padding: 20px; background: #f0f2f5; }
  .container { display: flex; gap: 20px; }
  .panel { flex: 1; background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); }
  .btn { padding: 6px 12px; cursor: pointer; background: #007bff; color: white; border: none; border-radius: 4px; margin-right:5px; }
  .btn-red { background: #dc3545; } .btn-green { background: #28a745; } .btn-orange { background: #fd7e14; }
  .alert-box { background: #fff3cd; border: 1px solid #ffeeba; padding: 15px; margin-bottom: 10px; border-radius: 5px; }
  table { width: 100%; border-collapse: collapse; margin-top: 10px; }
  th, td { text-align: left; padding: 8px; border-bottom: 1px solid #eee; }
  #file-browser { margin-top: 20px; display: none; border-top: 2px solid #eee; padding-top:10px; }
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

        <div id="file-browser">
            <h3>📂 Remote Files <span id="remote-host-name" style="font-weight:normal; font-size:0.8em; color:#666;"></span></h3>
            <div id="file-list"></div>
        </div>
    </div>
    <div class="panel">
        <h2>Server Mode (Status)</h2>
        <p>🟢 Active on port 55001</p>
        <p>💾 Downloads save to: <code>./downloads/</code></p>
    </div>
</div>

<script>
let knownTokens = {};
try { if(localStorage.getItem('safeTokens')) knownTokens = JSON.parse(localStorage.getItem('safeTokens')); } catch(e){}

// Poll for requests
setInterval(async () => {
    try {
        let res = await fetch('/api/requests');
        let reqs = await res.json();
        let html = "";
        reqs.forEach(r => {
            html += `<div class="alert-box">
                <strong>🔔 Request from ${r.name}</strong><br>${r.reason}<br><br>
                <button class="btn btn-green" onclick="decide('${r.id}', true)">Accept</button>
                <button class="btn btn-red" onclick="decide('${r.id}', false)">Deny</button>
            </div>`;
        });
        document.getElementById('alerts-area').innerHTML = html;
    } catch(e){}
}, 1000);

async function decide(id, accept) { await fetch(`/api/respond?id=${id}&ans=` + (accept ? 'yes' : 'no')); }
async function scan() {
    let res = await fetch('/api/scan');
    renderPeers(await res.json());
}

function renderPeers(peers) {
    let html = "<table><tr><th>Name</th><th>IP</th><th>Action</th></tr>";
    peers.forEach(p => {
        let btn = "";
        if (knownTokens[p.ip]) {
            btn = `<button class="btn btn-green" onclick="browse('${p.ip}', ${p.port}, '${p.name}')">📂 Browse Files</button>`;
        } else {
            btn = `<button class="btn" onclick="pair('${p.ip}', ${p.port})">🔗 Pair</button>`;
        }
        html += `<tr><td>${p.name}</td><td>${p.ip}</td><td>${btn}</td></tr>`;
    });
    html += "</table>";
    document.getElementById('peer-list').innerHTML = html;
}

async function pair(ip, port) {
    let reason = prompt("Reason?", "File transfer");
    if(!reason) return;
    let res = await fetch(`/api/pair?ip=${ip}&port=${port}&reason=${encodeURIComponent(reason)}`);
    let j = await res.json();
    if(j.status === "ok") {
        alert("Paired!");
        knownTokens[ip] = j.token;
        localStorage.setItem('safeTokens', JSON.stringify(knownTokens));
        scan();
    } else alert("Failed: " + j.message);
}

async function browse(ip, port, name) {
    let token = knownTokens[ip];
    if(!token) return;
    
    document.getElementById('file-browser').style.display = 'block';
    document.getElementById('remote-host-name').innerText = `(${name})`;
    document.getElementById('file-list').innerHTML = "Loading...";

    let res = await fetch(`/api/list?ip=${ip}&port=${port}&token=${token}`);
    let files = await res.json();
    
    let html = "<table><tr><th>Filename</th><th>Action</th></tr>";
    files.forEach(f => {
        html += `<tr>
            <td>${f}</td>
            <td><button class="btn btn-orange" onclick="download('${ip}', ${port}, '${f}')">⬇ Download</button></td>
        </tr>`;
    });
    html += "</table>";
    document.getElementById('file-list').innerHTML = html;
}

async function download(ip, port, file) {
    let token = knownTokens[ip];
    let res = await fetch(`/api/download?ip=${ip}&port=${port}&token=${token}&file=${encodeURIComponent(file)}`);
    let txt = await res.text();
    alert(txt);
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

    // --- API HANDLERS ---
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
        GlobalState::get().mergePeers(scan_once(500));
        auto full_list = GlobalState::get().getAllPeers();
        resp_body = "[";
        for(size_t i=0; i<full_list.size(); i++) {
            resp_body += "{ \"name\": \"" + full_list[i].name + "\", \"ip\": \"" + full_list[i].ip + "\", \"port\": " + std::to_string(full_list[i].port) + " }";
            if(i < full_list.size()-1) resp_body += ",";
        }
        resp_body += "]";
    }
    else if (req.find("GET /api/pair") != std::string::npos) {
        ctype = "application/json";
        std::string ip = "0.0.0.0", port="0", reason="web";
        auto p1 = req.find("ip="); auto p2 = req.find("port="); auto p3 = req.find("reason=");
        if(p1!=std::string::npos) ip = req.substr(p1+3, req.find("&", p1)-(p1+3));
        if(p2!=std::string::npos) port = req.substr(p2+5, req.find("&", p2)-(p2+5));
        if(p3!=std::string::npos) reason = req.substr(p3+7, req.find(" ", p3)-(p3+7));
        
        std::string t = send_perm_request(ip, std::stoi(port), "WebNode", reason);
        resp_body = t.empty() ? "{ \"status\":\"error\", \"message\":\"Failed\" }" : "{ \"status\":\"ok\", \"token\":\"" + t + "\" }";
    }
    // --- NEW: LIST FILES ---
    else if (req.find("GET /api/list") != std::string::npos) {
        ctype = "application/json";
        std::string ip="0", port="0", token="";
        auto p1=req.find("ip="); auto p2=req.find("port="); auto p3=req.find("token=");
        if(p1!=std::string::npos) ip=req.substr(p1+3, req.find("&", p1)-(p1+3));
        if(p2!=std::string::npos) port=req.substr(p2+5, req.find("&", p2)-(p2+5));
        if(p3!=std::string::npos) token=req.substr(p3+6, req.find(" ", p3)-(p3+6));

        auto files = list_remote_files(ip, std::stoi(port), token);
        resp_body = "[";
        for(size_t i=0; i<files.size(); i++) {
            resp_body += "\"" + files[i] + "\"";
            if(i < files.size()-1) resp_body += ",";
        }
        resp_body += "]";
    }
    // --- NEW: DOWNLOAD ---
    else if (req.find("GET /api/download") != std::string::npos) {
        std::string ip="0", port="0", token="", file="";
        auto p1=req.find("ip="); auto p2=req.find("port="); auto p3=req.find("token="); auto p4=req.find("file=");
        if(p1!=std::string::npos) ip=req.substr(p1+3, req.find("&", p1)-(p1+3));
        if(p2!=std::string::npos) port=req.substr(p2+5, req.find("&", p2)-(p2+5));
        if(p3!=std::string::npos) token=req.substr(p3+6, req.find("&", p3)-(p3+6));
        if(p4!=std::string::npos) file=req.substr(p4+5, req.find(" ", p4)-(p4+5));

        // Ensure "downloads" directory exists
        if (!fs::exists("downloads")) fs::create_directory("downloads");
        
        // Pass the specific destination path
        std::string status = download_file(ip, std::stoi(port), token, file, "downloads/" + file);
        resp_body = status;
    }

    std::string response = "HTTP/1.1 200 OK\r\nContent-Type: " + ctype + "\r\nContent-Length: " + std::to_string(resp_body.size()) + "\r\n\r\n" + resp_body;
    write(fd, response.c_str(), response.size());
    close(fd);
}

// Start server function remains identical...
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