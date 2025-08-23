#pragma once

#include <chrono>
#include <string>

enum State {
    STOPPED,
    WORKING,
    RESTING,
    FINISHED
};

class Pomodoro {
  public:
    Pomodoro(const int sessionLengthMinutes, const int restLengthMinutes);
    ~Pomodoro();
    void        start();
    void        stop();
    void        pause();
    void        resume();
    void        reset();
    void        restart();
    void        startRest();
    void        skip();
    bool        isFinished();
    bool        isPaused();

    int         getSessionLength() const;
    int         getRestLength() const;
    int         getRound() const;
    int         getRemainingTime() const;
    std::string getFormattedTime() const;
    float       getProgress();
    State       getState();
    void        setSessionLength(int length);
    void        setRestLength(int length);

  private:
    std::chrono::steady_clock::time_point m_startTime;
    std::chrono::steady_clock::time_point m_pausedTime;
    int                                   m_round;
    float                                 m_progress;
    int                                   m_sessionLength;
    int                                   m_restLength;
    bool                                  m_pause;
    State                                 m_currentState;
};