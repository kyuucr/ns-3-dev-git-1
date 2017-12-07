/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2017 Natale Patriciello <natale.patriciello@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Natale Patriciello <natale.patriciello@gmail.com>
 * Special Thanks to: https://github.com/nbsdx/ThreadPool
 */
#pragma once

#include <array>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <queue>
#include <functional>
#include <future>

namespace ns3 {

/**
 * \brief A thread pool class
 *
 * The objective of this class is to provide a safe pool in which it is possible
 * to execute 'jobs'. A job is any Callable object (function, method, lambda..)
 * that can be invoked. If the number of threads is positive, then the Callable
 * will be invoked in a different thread. The jobs are stored in a queue, therefore
 * the oldest enqueued job will run as soon as there is an available thread.
 * If the number of threads equals 0, then the job is invoked as soon as the get
 * method on the corresponding future is invoked.
 *
 * The threads, once created, will be living for all the duration of the
 * simulation. If no jobs are available, they will sleep on a
 * std::condition_variable, and woken up as soon as there is a job available.
 *
 * \see AddJob
 */
class ThreadPool
{
private:
  std::vector<std::thread> m_threads;           //!< Thread pool
  std::queue<std::function<void(void)>> m_jobs; //!< Jobs queue

  std::atomic_int         m_jobsLeft;           //!< Number of jobs waiting to execute
  std::atomic_bool        m_bailout;            //!< Indicate if we have to exit
  std::atomic_bool        m_finished;           //!< Indicate if we are done processing jobs
  std::condition_variable m_job_available_var;  //!< conditional wait for jobless threads
  std::condition_variable m_waitVar;            //!< conditional wait for the jobs waiting to be executed
  std::mutex              m_waitMutex;          //!< Mutex to operate on the wait variable
  std::mutex              m_queueMutex;         //!< Mutex to operate on the jobs queue

  /**
   *  \brief Take the next job in the queue and run it.
   *
   * Notify the main thread that a job has completed through the waitVar
   * variable.
   */
  void Task ()
  {
    while( !m_bailout )
      {
        std::function<void(void)> fun = NextJob ();

        // Execute the job
        fun ();

        --m_jobsLeft;
        m_waitVar.notify_one ();
      }
  }

  /**
   * \brief Get the next job
   *
   * Pop the first item in the queue, otherwise wait for a signal from the main thread.
   * \return A std::function with the next job to execute
   */
  std::function<void(void)> NextJob ()
  {
    std::function<void(void)> res;
    std::unique_lock<std::mutex> job_lock (m_queueMutex);

    // Wait for a job if we don't have any.
    m_job_available_var.wait ( job_lock, [this]() -> bool { return m_jobs.size () || m_bailout; } );

    // Get job from the queue
    if( !m_bailout )
      {
        res = std::move (m_jobs.front ());
        m_jobs.pop ();
      }
    else
      { // If we're bailing out, 'inject' a job into the queue to keep jobs_left accurate.
        res = [] {};
        ++m_jobsLeft;
      }
    return res;
  }

public:
  /**
   * \brief ThreadPool default constructor
   *
   * Please remember to call Init
   * \see Init
   */
  ThreadPool ()
    : m_jobsLeft (0),
      m_bailout (false),
      m_finished (false)
  {

  }

  /**
   * \brief ThreadPool constructor when there is the thread number available
   * \param threadCount the number of thread to create
   *
   * Automatically calls Init
   */
  ThreadPool(uint16_t threadCount)
    : m_jobsLeft (0),
      m_bailout (false),
      m_finished (false)
  {
    Init (threadCount);
  }

  /**
   * \brief Initialize the pool with the given number of threads
   * \param threadCount the number of thread to create
   */
  void Init (uint16_t threadCount)
  {
    m_threads.resize (threadCount);
    for (uint16_t i = 0; i < threadCount; ++i)
      {
        m_threads[i] = std::thread ( [this] { this->Task (); } );
      }
  }

  /**
   * \brief Destroy the pool
   *
   * JoinAll on deconstruction.
   */
  ~ThreadPool()
  {
    JoinAll ();
  }

  /**
   * \return The number of threads in this pool
   */
  unsigned long Size () const
  {
    return m_threads.size ();
  }

  /**
   * \return The number of jobs left in the queue.
   */
  unsigned long JobsRemaining ()
  {
    std::lock_guard<std::mutex> guard (m_queueMutex);
    return m_jobs.size ();
  }

  /**
   * \brief Add a new job to the pool.
   *
   * If there are no jobs in the queue,
   * a thread is woken up to take the job. If all threads are busy,
   * the job is added to the end of the queue.
   *
   * \param f The job to execute
   * \param args The parameters of the job
   * \return a std::future which holds the result. To retrieve it, call the
   * corresponding get method
   */
  template<class F, class ... Args>
  auto AddJob (F&& f, Args&&... args)->std::future<typename std::result_of<F (Args...)>::type>
  {
    using return_type = typename std::result_of<F (Args...)>::type;
    std::future<return_type> res;

    if (m_threads.size () > 0)
      {
        // If we have threads, put the job inside a packaged_task

        auto task = std::make_shared< std::packaged_task<return_type ()> >(
            std::bind (std::forward<F>(f), std::forward<Args>(args) ...)
            );

        res = task->get_future ();

        {
          std::lock_guard<std::mutex> lock (m_queueMutex);
          // Enqueue in the jobs queue a lambda function that calls the job
          m_jobs.emplace ([task](){ (*task)(); });
          ++m_jobsLeft;
        }
        // Notify a thread that there is a job for him
        m_job_available_var.notify_one ();
      }
    else
      {
        // If we do not have any available thread, use std::async with a
        // deferred option: the job will run when the .get method will be
        // called on the future.
        res = std::async (std::launch::deferred,
                          std::bind (std::forward<F>(f), std::forward<Args>(args) ...));
      }

    return res;
  }

  /**
   *  \brief Join with all threads.
   *
   *  Block until all threads have completed.
   *  The queue will be empty after this call, and the threads will
   *  be done. After invoking `ThreadPool::JoinAll`, the pool can no
   *  longer be used. If you need the pool to exist past completion
   *  of jobs, look to use `ThreadPool::WaitAll`.
   *
   * \param waitForAll: If true, will wait for the queue to empty
   * before joining with threads. If false, will complete
   * current jobs, then inform the threads to exit.
   */
  void JoinAll ( bool waitForAll = true )
  {
    if (!m_finished)
      {
        if( waitForAll )
          {
            WaitAll ();
          }

        // note that we're done, and wake up any thread that's
        // waiting for a new job
        m_bailout = true;
        m_job_available_var.notify_all ();

        for (auto &x : m_threads)
          {
            if (x.joinable ())
              {
                x.join ();
              }
          }
        m_finished = true;
      }
  }

  /**
   *  \brief Wait for the pool to empty before continuing.
   *
   * This does not call `std::thread::join`, it only waits until
   * all jobs have finshed executing.
   */
  void WaitAll ()
  {
    if( m_jobsLeft > 0 )
      {
        std::unique_lock<std::mutex> lk (m_waitMutex);
        m_waitVar.wait (lk, [this] { return this->m_jobsLeft == 0; } );
        lk.unlock ();
      }
  }
};

} // namespace ns3
