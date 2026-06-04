#include "pch.h"
#include "Profiler.h"
#include <thread>

void Profiler::Timer::Start()
{
    // record the curr time as start time
    m_start = std::chrono::high_resolution_clock::now();

    //call begin Timer and convert start time to microseconds
    uint64_t ts = m_start.time_since_epoch().count() / 1000;
    Profiler::Get()->BeginTimer(m_name, ts);

}

void Profiler::Timer::Stop()
{
    // record curr time as end time then add to frame total
    auto end = std::chrono::high_resolution_clock::now();
    double duration = std::chrono::duration<double, std::milli>(end - m_start).count();
    m_frameTime += duration;

    //call end timer and conver to microseconds
    uint64_t ts = end.time_since_epoch().count() / 1000;
    Profiler::Get()->EndTimer(ts);
}

void Profiler::Timer::Reset()
{
    // add this frame's time into overall time
    m_totalTime += m_frameTime;
    // count this frame
    m_frameCount++;
    // track longest frame
    if (m_frameTime > m_maxTime)
        m_maxTime = m_frameTime;
    // reset current frame time for next frame
    m_frameTime = 0.0;
}

double Profiler::Timer::GetAvg_ms() const
{
    if (m_frameCount == 0)
        return 0.0;
    return m_totalTime / m_frameCount;
}


Profiler::ScopedTimer::ScopedTimer(Timer* timer)
    : m_timer(timer)
{
    m_timer->Start();
}

Profiler::ScopedTimer::~ScopedTimer()
{
    m_timer->Stop();
}

Profiler::Timer* Profiler::GetTimer(const std::string& name)
{
    std::lock_guard<std::mutex> lock(m_timerMutex);
    // search through timer map and return the timer if it exists
    auto it = m_timerMap.find(name);
    if (it != m_timerMap.end())
        return it->second;

    // if not create timer, add to map, and return it
    Timer* timer = new Timer();
    timer->m_name = name;
    m_timerMap[name] = timer;
    return timer;
}

void Profiler::ResetAll()
{
    std::lock_guard<std::mutex> lock(m_timerMutex);
    // loop through timermap and call reset on timers
    for (auto& pair : m_timerMap)
        pair.second->Reset();
}

const std::unordered_map<std::string, Profiler::Timer*>& Profiler::GetTimers() const
{
    return m_timerMap;
}

Profiler::Profiler()
{
    //open json file and start array with [
    m_JSONFile.open("profile.json");
    if (m_JSONFile.is_open())
    {
        m_JSONFile << "[\n";
    }
}

Profiler::~Profiler()
{
    //close the json array
    if (m_JSONFile.is_open())
    {
        m_JSONFile << "\n]";
        m_JSONFile.close();
    }

    //create profile.txt when done
    std::ofstream file("profile.txt");
    //loop through timers and record their name avg time and max time
    if (file.is_open())
    {
        file << "Profile Results\n";
        for (auto& pair : m_timerMap)
        {
            Timer* timer = pair.second;
            file << timer->GetName()
                 << " average: " << timer->GetAvg_ms() << " ms"
                 << " max: " << timer->GetMax_ms() << " ms"
                 << "\n";
        }
        file.close();
    }

    //clean up timers
    for (auto& pair : m_timerMap)
        delete pair.second;
    m_timerMap.clear();
}

void Profiler::BeginTimer(const std::string& name, uint64_t startTime)
{
    if (!m_JSONFile.is_open())
        return;

    size_t threadID = std::hash<std::thread::id>{}(std::this_thread::get_id());

    std::lock_guard<std::mutex> lock(m_jsonMutex);

    if (!m_firstEvent)
        m_JSONFile << ",\n";
    m_firstEvent = false;

    m_JSONFile << "{"
               << "\"ph\":\"B\","
               << "\"name\":\"" << name << "\","
               << "\"ts\":" << startTime << ","
               << "\"pid\":1,"
               << "\"tid\":" << threadID
               << "}";
}

void Profiler::EndTimer(uint64_t endTime)
{
    if (!m_JSONFile.is_open())
        return;

    size_t threadID = std::hash<std::thread::id>{}(std::this_thread::get_id());

    std::lock_guard<std::mutex> lock(m_jsonMutex);

    if (!m_firstEvent)
        m_JSONFile << ",\n";
    m_firstEvent = false;

    m_JSONFile << "{"
               << "\"ph\":\"E\","
               << "\"ts\":" << endTime << ","
               << "\"pid\":1,"
               << "\"tid\":" << threadID
               << "}";
}

