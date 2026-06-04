#include "JobManager.h"
//Job Manager Funcs
void JobManager::Begin()
{
    //loop through and being all workers
    for (int i = 0; i < NUM_WORKERS; i++)
        m_workers[i].Begin();
}

void JobManager::End()
{
    m_shutdown = true;
    for (int i = 0; i < NUM_WORKERS; i++)
    {
        m_workers[i].End();
    }
}

void JobManager::AddJob(Job* pJob)
{
    //increment job count before adding to queue
    m_jobCount++;
    //lock mutex while modifying queue
    {
        std::lock_guard<std::mutex> lock(m_jobMutex);
        m_jobs.push(pJob);
    }
    //should unlock upon leaving scope
}

void JobManager::WaitForJobs()
{
    //sleep until jobs are done
    while (m_jobCount > 0)
    {
        std::this_thread::sleep_for(std::chrono::nanoseconds(1));
    }
}

//Worker Funcs
void JobManager::Worker::Begin()
{
    //create new thread and tell it to run loop
    m_thread = std::thread(Loop);
}

void JobManager::Worker::End()
{
    //wait for thread to finish then release
    if (m_thread.joinable())
        m_thread.join();
}

void JobManager::Worker::Loop()
{
    //keep running until shutdown signal is received
    while (!JobManager::Get()->IsShutdown())
    {
        Job* pJob = nullptr;

        {
            //lock the mutex
            std::lock_guard<std::mutex> lock(JobManager::Get()->m_jobMutex);
            //grab next job and remove it from m_jobs
            if (!JobManager::Get()->m_jobs.empty())
            {
                pJob = JobManager::Get()->m_jobs.front();
                JobManager::Get()->m_jobs.pop();
            }
        }

        //if theres a job, do it and remove from queue, else sleep
        if (pJob)
        {
            pJob->DoIt();
            JobManager::Get()->m_jobCount--;
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::nanoseconds(1));
        }

    }
}








