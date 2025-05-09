#pragma once
#include <string>
#include <mutex>
#include <atomic>

struct InitializationState {
    std::atomic<int> percentage = 0;
    std::string statusMessage = "Starting...";
    std::atomic<bool> completed = false;
    std::mutex messageMutex;

    void Update(int perc, const std::string& msg, bool done = false) {
        std::lock_guard<std::mutex> lock(messageMutex);
        percentage.store(perc);
        statusMessage = msg;
        completed.store(done);
    }

    void Get(int& perc, std::string& msg, bool& done) {
        std::lock_guard<std::mutex> lock(messageMutex);
        perc = percentage.load();
        msg = statusMessage;
        done = completed.load();
    }
};

extern InitializationState g_InitState;