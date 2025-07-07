#pragma once

#include <chrono>

class Timer {
public:
  Timer() : m_Start(), m_End(), m_LastCheckpoint(), m_Running(false) {}

  void Start() {
    m_Start = std::chrono::high_resolution_clock::now();
    m_LastCheckpoint = m_Start;
    m_Running = true;
  }

  void Stop() {
    m_End = std::chrono::high_resolution_clock::now();
    m_Running = false;
  }

  double GetElapsedMilliseconds() const {
    auto end = m_Running ? std::chrono::high_resolution_clock::now() : m_End;
    return std::chrono::duration<double, std::milli>(end - m_Start).count();
  }

  double GetElapsedSeconds() const {
    auto end = m_Running ? std::chrono::high_resolution_clock::now() : m_End;
    return std::chrono::duration<double>(end - m_Start).count();
  }

  double GetDeltaMilliseconds() {
    auto now = m_Running ? std::chrono::high_resolution_clock::now() : m_End;
    double delta = std::chrono::duration<double, std::milli>(now - m_LastCheckpoint).count();
    m_LastCheckpoint = now;
    return delta;
  }

  double GetDeltaSeconds() {
    auto now = m_Running ? std::chrono::high_resolution_clock::now() : m_End;
    double delta = std::chrono::duration<double>(now - m_LastCheckpoint).count();
    m_LastCheckpoint = now;
    return delta;
  }

private:
  std::chrono::high_resolution_clock::time_point m_Start;
  std::chrono::high_resolution_clock::time_point m_End;
  std::chrono::high_resolution_clock::time_point m_LastCheckpoint;
  bool m_Running;
};
