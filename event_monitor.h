#pragma once

#include <unordered_map>
#include <vector>
#include <string>
#include <sstream>
#include <sys/inotify.h>

#define SETTING_FILE "settings.ini"
#define LOG_SAVE_PATH "logs/file_event_monitor_"

#define PERFORM_MONITORING 1
#define SEND_EMAIL 2

#define EVENT_SIZE (sizeof(struct inotify_event)) // 이벤트 구조체 크기
#define EVENT_BUFFER_SIZE (1024 * (EVENT_SIZE + 16)) // 한 번에 읽을 수 있는 최대 바이트 수

struct MonitorData {
    std::string eventDescription;
    std::string filePath;
    std::string integrityResult;
    std::string newHash;
    std::string oldHash;
    std::string timestamp;
};

class CEventMonitor {
public:
    CEventMonitor();
    ~CEventMonitor();
    int StartMonitoring();

private:
    int m_inotifyFd;
    std::unordered_map<int, std::string> m_mapWatchDescriptors;
    std::vector<std::string> m_vecWatchList;

    void readWatchList();
    void initializeWatchList();
    void createInotifyInstance();
    void addWatchListToInotify();
    void runEventLoop();
    void processEvent(struct inotify_event *event);
    void printEventsInfo(MonitorData& data);
    void verifyFileIntegrity(MonitorData& data);
    void logEvent(MonitorData& data);
    std::string getLogFilePath();
};
