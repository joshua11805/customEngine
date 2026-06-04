#pragma once
#if 1

#define PROFILE_SCOPE(name) \
Profiler::ScopedTimer name##_scope(Profiler::Get()->GetTimer(std::string(#name)))

#else

#define PROFILE_SCOPE(name)

#endif

#include <string>
#include <chrono>
#include <unordered_map>
#include <mutex>


class Profiler
{
public:
    //follows singleton pattern
    static Profiler* Get()
    {
        static Profiler instance;
        return &instance;
    }

    class Timer
    {
        friend class Profiler;

    public:
        void Start();
        void Stop();
        void Reset();

        const std::string& GetName() const { return m_name; }
        double GetTime_ms() const { return m_frameTime; }
        double GetMax_ms() const { return m_maxTime; }
        double GetAvg_ms() const;

    private:
        Timer() = default;
        ~Timer() = default;

        std::string m_name;
        double m_frameTime = 0.0;
        double m_maxTime   = 0.0;
        double m_totalTime = 0.0;
        int m_frameCount   = 0;
        std::chrono::high_resolution_clock::time_point m_start;
    };

    class ScopedTimer
    {
    public:
        ScopedTimer(Timer* timer);
        ~ScopedTimer();

    private:
        Timer* m_timer;
    };

public:
    Timer* GetTimer(const std::string& name);
    void ResetAll();
    const std::unordered_map<std::string, Timer*>& GetTimers() const;

    //chrome trace functions
    void BeginTimer(const std::string &name, uint64_t startTime);
    void EndTimer(uint64_t endTime);
private:
    Profiler();
    ~Profiler();

    std::unordered_map<std::string, Timer*> m_timerMap;
    std::ofstream m_JSONFile;
    bool m_firstEvent = true; //tracks if we need a comma before the event

    std::mutex m_jsonMutex; //adding mutex because of race conditions when compiling json
    std:: mutex m_timerMutex;
};