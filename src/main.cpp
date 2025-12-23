#include "scheduler.hpp"
#include <atomic>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <thread>

static void burn_cpu(std::chrono::milliseconds d) {
  auto start = std::chrono::steady_clock::now();
  volatile double x = 0.0;
  while (std::chrono::steady_clock::now() - start < d) {
    x += std::sin(x + 0.001);
  }
}

int main(int argc, char** argv) {
  std::atomic<bool> overload{false};
  if (argc > 1 && std::string(argv[1]) == "--overload") overload = true;

  Scheduler sched;
  sched.lateness_drop_threshold = std::chrono::milliseconds(40);

  // 10ms sensor
  sched.add_task(TaskSpec{
      .name = "sensor_10ms",
      .period = std::chrono::milliseconds(10),
      .priority = 3,
      .crit = Criticality::Hard,
      .fn = []{
        // pretend to sample + tiny filter
        static double y = 0.0;
        double x = 1.0;
        y = 0.9*y + 0.1*x;
      }
      });

  // 50ms compute (best-effort)
  sched.add_task(TaskSpec{
      .name = "compute_50ms",
      .period = std::chrono::milliseconds(50),
      .priority = 2,
      .crit = Criticality::BestEffort,
      .fn = [&]{
      // normal work
        burn_cpu(std::chrono::milliseconds(3));
        if (overload) {
        // occasional spike to force overruns
          static int k = 0;
          if ((++k % 10) == 0) burn_cpu(std::chrono::milliseconds(80));
        }
      }
      });

  // 200ms comms
  sched.add_task(TaskSpec{
      .name = "comms_200ms",
      .period = std::chrono::milliseconds(200),
      .priority = 1,
      .crit = Criticality::Hard,
      .fn = []{
        // placeholder
      }
      });

  // stats printer (separate thread just reading stable snapshot)
  std::atomic<bool> running{true};
  std::thread printer([&]{
      while (running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        std::cout << "\n--- telemetry ---\n";
        std::cout << std::left << std::setw(14) << "task"
                  << std::right << std::setw(10) << "rel"
                  << std::setw(10) << "ovr"
                  << std::setw(14) << "jit_avg(us)"
                  << std::setw(14) << "jit_max(us)"
                  << std::setw(14) << "exe_avg(us)"
                  << std::setw(14) << "exe_max(us)"
                  << "\n";

        for (auto& t : sched.tasks()) {
        const auto& s = t.stats;
        double jit_avg_us = s.releases ? (double)s.jitter_sum_ns / (double)s.releases / 1000.0 : 0.0;
        double exe_avg_us = s.releases ? (double)s.exec_sum_ns / (double)s.releases / 1000.0 : 0.0;

        std::cout << std::left << std::setw(14) << t.spec.name
                  << std::right << std::setw(10) << s.releases
                  << std::setw(10) << s.overruns
                  << std::setw(14) << jit_avg_us
                  << std::setw(14) << (double)s.jitter_max_ns / 1000.0
                  << std::setw(14) << exe_avg_us
                  << std::setw(14) << (double)s.exec_max_ns / 1000.0
                  << "\n";
        }
      }
      });

  sched.run_for(std::chrono::seconds(15));
  running = false;
  printer.join();
}
