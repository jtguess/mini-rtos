// scheduler.hpp

#pragma once
#include <chrono>
#include <cstdInt>
#include <functional>
#include <string>
#include <vector>

struct TaskStats {
  uint64_t releases = 0;
  uint64_t overruns = 0;

  int64_t jitter_min_ns = 0;
  int64_t jitter_max_ns = 0;
  uint64_t jitter_sum_ns = 0;

  int64_t exec_min_ns = 0;
  int64_t exec_max_ns = 0;
  uint64_t exec_sum_ns = 0;

  void observe(int64_t jitter_ns, int64_t exec_ns, bool overrun);
};

enum class Criticality { Hard, BestEffort };

struct TaskSpec {
  std::string name;
  std::chrono::nanoseconds period;
  int priority = 0; // higher runs first when multiple ready
  Criticality crit = Criticality::Hard;
  std::function<void()> fn;
};

struct TaskRuntime {
  TaskSpec spec;
  std::chrono::steady_clock::time_point next_release;
  TaskStats stats;
};

class Scheduler {
  public:
    void add_task(TaskSpec spec);
    void start(std::chrono::steady_clock::time_point t0);
    void run_for(std::chrono::seconds dur);
    const std::vector<TaskRuntime>& tasks() const { return tasks_; }

    // knobs
    std::chrono::nanoseconds lateness_drop_threshold{0}; // e.g. 2*period for best effort
    
  private:
    void run_once_until(std::chrono::steady_clock::time_point end);

    std::vector<TaskRuntime> tasks_;
    bool started_ = false;
};
