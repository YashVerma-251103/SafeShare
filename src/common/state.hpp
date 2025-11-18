// // src/common/state.hpp
// #pragma once
// #include <mutex>
// #include <condition_variable>
// #include <string>
// #include <vector>
// #include <atomic>
// #include "../discovery/discovery.hpp"

// // Simple structure for a pending approval
// struct PendingReq {
//     std::string id;
//     std::string name;
//     std::string reason;
//     bool active = false;
//     int decision = 0; // 0=waiting, 1=accept, 2=deny
// };

// class GlobalState {
// public:
//     static GlobalState& get() {
//         static GlobalState instance;
//         return instance;
//     }

//     // --- Server Side (Approval Logic) ---
    
//     // Called by Server Thread to ask for permission
//     bool waitForApproval(const std::string& name, const std::string& reason) {
//         std::unique_lock<std::mutex> lock(mtx);
//         current_req = { "req-1", name, reason, true, 0 }; // Set active request
        
//         // Wait until decision is made (decision != 0)
//         cv.wait(lock, [this]{ return current_req.decision != 0; });

//         bool accepted = (current_req.decision == 1);
//         current_req.active = false; // Reset
//         return accepted;
//     }

//     // Called by Web UI to submit decision
//     void submitDecision(bool accept) {
//         std::lock_guard<std::mutex> lock(mtx);
//         if (current_req.active) {
//             current_req.decision = accept ? 1 : 2;
//             cv.notify_all(); // Wake up the Server Thread
//         }
//     }

//     // Get info for Web UI polling
//     PendingReq getPending() {
//         std::lock_guard<std::mutex> lock(mtx);
//         return current_req;
//     }

//     // --- Client Side (Discovery Cache) ---
//     void updatePeers(std::vector<DiscoveredPeer> p) {
//         std::lock_guard<std::mutex> lock(mtx);
//         peers = p;
//     }
    
//     std::vector<DiscoveredPeer> getPeers() {
//         std::lock_guard<std::mutex> lock(mtx);
//         return peers;
//     }

// private:
//     std::mutex mtx;
//     std::condition_variable cv;
//     PendingReq current_req;
//     std::vector<DiscoveredPeer> peers;
// };






#pragma once
#include <mutex>
#include <condition_variable>
#include <string>
#include <vector>
#include <map>
#include "../discovery/discovery.hpp"
#include "../common/utils.hpp" 

struct PendingReq {
    std::string id;
    std::string name;
    std::string reason;
    int decision = 0; // 0=waiting, 1=accept, 2=deny
};

class GlobalState {
public:
    static GlobalState& get() {
        static GlobalState instance;
        return instance;
    }

    // --- SERVER: Approval Logic ---
    bool waitForApproval(const std::string& name, const std::string& reason) {
        std::unique_lock<std::mutex> lock(mtx);
        std::string req_id = random_hex(8); 
        requests[req_id] = { req_id, name, reason, 0 };

        cv.wait(lock, [this, req_id]{ 
            return requests[req_id].decision != 0; 
        });

        bool accepted = (requests[req_id].decision == 1);
        requests.erase(req_id);
        return accepted;
    }

    void submitDecision(std::string id, bool accept) {
        std::lock_guard<std::mutex> lock(mtx);
        if (requests.count(id)) {
            requests[id].decision = accept ? 1 : 2;
            cv.notify_all();
        }
    }

    std::vector<PendingReq> getPending() {
        std::lock_guard<std::mutex> lock(mtx);
        std::vector<PendingReq> list;
        for(auto& [k, v] : requests) list.push_back(v);
        return list;
    }

    // --- CLIENT: Persistent Discovery (The Fix) ---
    void mergePeers(const std::vector<DiscoveredPeer>& new_peers) {
        std::lock_guard<std::mutex> l(mtx);
        for(const auto& p : new_peers) {
            // Key by IP to update existing entries or add new ones
            peer_map[p.ip] = p; 
        }
    }
    
    // Remove a peer (e.g. if connection fails)
    void removePeer(const std::string& ip) {
        std::lock_guard<std::mutex> l(mtx);
        peer_map.erase(ip);
    }

    std::vector<DiscoveredPeer> getAllPeers() {
        std::lock_guard<std::mutex> l(mtx);
        std::vector<DiscoveredPeer> list;
        for(auto& [ip, p] : peer_map) list.push_back(p);
        return list;
    }

private:
    std::mutex mtx;
    std::condition_variable cv;
    std::map<std::string, PendingReq> requests;
    
    // CHANGED: Persistent storage for peers
    std::map<std::string, DiscoveredPeer> peer_map; 
};