#ifndef GAME_JOBMANAGER_H
#define GAME_JOBMANAGER_H
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>

class JobManager
{
public:
    static JobManager* Get()
    {
        static JobManager instance;
        return &instance;
    }

    class Job
    {
    public:
        virtual void DoIt() = 0;
        virtual ~Job() = default;
    };

    class Worker
    {
    public:
        void Begin();
        void End();
        static void Loop();
    private:
        std::thread m_thread;
    };
    void Begin();
    void End();
    void AddJob(Job* pJob);
    void WaitForJobs();
    bool IsShutdown() const { return m_shutdown; }
private:
    static constexpr int NUM_WORKERS = 4;
    Worker m_workers[NUM_WORKERS];
    //using atomic bool as the shutdown signal because it is thread safe
    std::atomic<bool> m_shutdown = false;
    std::queue<Job*> m_jobs; //using queue because it is FIFO
    std::mutex m_jobMutex;
    std::atomic<int> m_jobCount = 0;
};


#endif //GAME_JOBMANAGER_H