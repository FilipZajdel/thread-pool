/**
 * MIT License
 * Copyright (c) 2022 FilipZajdel
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include <atomic>
#include <thread>
#include <queue>
#include <iostream>
#include <mutex>
#include <functional>
#include <sstream>
#include <vector>
#include <chrono>
#include <string>
#include <cmath>
#include <omp.h>

using std::cout;
using std::thread;
using std::queue;
using std::mutex;
using std::function;
using std::vector;
using std::atomic_bool;
using std::lock_guard;
using std::string;
using std::istringstream;

class ThreadPool {
    typedef function<void(void)> job_t;
    queue<job_t> jobs;
    vector<thread> workers;
    mutex jobs_mutex;
    atomic_bool stopFlag;
    static void worker(ThreadPool *pool);

  public:
    ThreadPool(unsigned num_of_threads);
    ~ThreadPool();
    void submit(job_t job);
};

void ThreadPool::worker(ThreadPool *pool)
{
    while(true) {
        job_t job;
        bool got_job = false;
        {
            lock_guard<mutex> lock(pool->jobs_mutex);
            got_job = !pool->jobs.empty();

            if (got_job) {
                job = pool->jobs.front();
                pool->jobs.pop();
            } else if (!got_job && pool->stopFlag) {
                break;
            }
        }
        if (got_job) {
            // cout << "Executing job on thread " << std::this_thread::get_id() << "\n";
            job();
        }
    }
}

ThreadPool::ThreadPool(unsigned num_of_threads) : stopFlag(false)
{
    for (unsigned i=0; i<num_of_threads; i++) {
        workers.push_back(thread(ThreadPool::worker, this));
    }
}

void ThreadPool::submit(ThreadPool::job_t job)
{
    lock_guard<mutex> lock(jobs_mutex);
    jobs.push(job);
}

ThreadPool::~ThreadPool()
{
    stopFlag = true;
    for (auto &worker: workers) {
        worker.join();
    }
}

double execute_with_thread_pool(unsigned num_of_jobs)
{
    ThreadPool threadPool(thread::hardware_concurrency()*6);
    double sqrt = 4;

    for (int i=0; i<num_of_jobs; i++) {
        threadPool.submit([&](){
            for(unsigned j=1; j!=10000; j++) {
                sqrt = std::sqrt(sqrt);
                sqrt = sqrt + j;
            }
        });
    }

    return sqrt;
}

double execute_with_many_threads(unsigned num_of_jobs)
{
    vector<thread> threads;
    threads.reserve(num_of_jobs);
    double sqrt = 4;

    for (int i=0; i<num_of_jobs; i++) {
        threads.push_back(
            thread([&](){
                for(unsigned j=1; j!=10000; j++) {
                    sqrt = std::sqrt(sqrt);
                    sqrt = sqrt + j;
                }
            }));
    }

    for (auto &thread: threads)
    {
        thread.join();
    }

    return sqrt;
}

int main(int argc, char *argv[])
{
    function<double(unsigned)> demo;

    if (argc != 3) {
        cout << "help:\n\t" << argv[0] << " pool|threads\n";
        return -1;
    }
    string demo_type = string(argv[1]);
    unsigned num_of_jobs = 0;

    istringstream(string(argv[2])) >> num_of_jobs;

    if (demo_type == "pool") {
        demo = execute_with_thread_pool;
    } else if (demo_type == "threads") {
        demo = execute_with_many_threads;
    } else{
        cout << "help:\n\t" << argv[0] << " pool|threads\n";
        return -1;
    }

    cout << "Result " << demo(num_of_jobs) << "\n";
}
