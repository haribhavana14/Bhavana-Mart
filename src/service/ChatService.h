#pragma once

#include <chrono>
#include <deque>
#include <mutex>
#include <string>
#include <unordered_map>

struct ChatReply
{
    std::string reply;
    bool rate_limited = false;
    bool degraded = false;
};

class ChatService
{
public:
    ChatService();
    ChatReply Ask(const std::string& session_id,
                  const std::string& message);

private:
    struct SessionState
    {
        std::deque<std::chrono::steady_clock::time_point> message_times;
        std::unordered_map<std::string, std::string> cache;
    };

    std::string api_key_;
    std::mutex mutex_;
    std::unordered_map<std::string, SessionState> sessions_;
};