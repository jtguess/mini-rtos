// scheduler.cpp
#include "scheduler.hpp"
#include <algorithm>
#include <chrono>
#include <thread>

static inline int64_t to_ns(std::chrono::nanoseconds ns) { return ns.count(); }

void TaskStats::observe(int64_t jitter_ns, int64_t exec_ns, bool overrun) {
  releases++;
  if (releases == 1) {
    jitter_min_ns = jitter_max_ns = jitter_ns;
    exec_min_ns = exec_max_ns = exec_ns;
  } else {
    jitter_min_ns = std::min(jitter_min_ns, jitter_ns);
    jitter_max_ns = std::max(jitter_max_ns, jitter_ns);
    exec_min_ns = std::min(exec_min_ns, exec_ns);
    exec_max_ns = std::max(exec_max_ns, exec_ns);
  }
  jitter_sum_ns += (jitter_ns < 0 ? 0ULL : static_cast<uint64_t>(jitter_ns));
  exec_sum_ns += static_cast<uint64_t>(exec_ns);
  if (overrun) overruns++;
}

void Scheduler::add_task(TaskSpec spec) {
  tasks_.push_back(TaskRuntime{std::move(spec), {}, {}});
}

void Scheduler::start(std::chrono::steady_clock::time_point t0) {
  for (auto& t : tasks_) t.next_release = t0 + t.spec.period;
  started_ = true;
}

void Scheduler::run_for(std::chrono::seconds dur) {
  if (!started_) start(std::chrono::steady_clock::now());
  auto end = std::chrono::steady_clock::now() + dur;
  run_once_until(end);
}

void Scheduler::run_once_until(std::chrono::steady_clock::time_point end) {
  while (std::chrono::steady_clock::now() < end) {
    // find earliest release
    auto earliest = tasks_[0].next_release;
    for (auto& t : tasks_) earliest = std::min(earliest, t.next_release);

    std::this_thread::sleep_until(earliest);

    auto now = std::chrono::steady_clock::now();

    // collect ready tasks
    std::vector<TaskRuntime*> ready;
    ready.reserve(tasks_.size());
    for (auto& t : tasks_) {
      if (t.next_release <= now) ready.push_back(&t);
    }

    // run ready tasks by priority (higher first)
    std::sort(ready.begin(), ready.end(), [](auto* a, auto* b) {
        return a->spec.priority > b->spec.priority;
    });

    for (auto* t : ready) {
      auto release = t->next_release;
      auto start = std::chrono::steady_clock::now();
      auto jitter = std::chrono::duration_cast<std::chrono::nanoseconds>(start - release).count();

      // drop best-effort if too late
      if (t->spec.crit == Criticality::BestEffort && lateness_drop_threshold.count() > 0) {
        auto late_by = start - release;
        if (late_by > lateness_drop_threshold) {
          // Skip execution, but still advance schedule deterministically
          do { t->next_release += t->spec.period; } while (t->next_release <= start);
          t->stats.observe(std::max<int64_t>(0, jitter), 0, false);
          continue;
        }
      }

      t->spec.fn();

      auto finish = std::chrono::steady_clock::now();
      auto exec = std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start).count();
      bool overrun = finish > (release + t->spec.period);

      t->stats.observe(std::max<int64_t>(0, jitter), exec, overrun);

      // advance next_release without drift
      do { t->next_release += t->spec.period; } while (t->next_release <= finish);
    }
  }
}
