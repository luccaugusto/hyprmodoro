#pragma once

#include <chrono>
#include <string>
#include <functional>

enum State {
    STOPPED,
    WORKING,
    RESTING,
    FINISHED,
    WAITING_FOR_REST,
    WAITING_FOR_WORK
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
    State       getLastState() const;
    void        setState(State newState);
    void        setSessionLength(int length);
    void        setRestLength(int length);
    void        setOnSessionEndCallback(std::function<void(State)> callback);
    void        setAutoTransition(bool autoTransition);
    bool        getAutoTransition() const;

  private:
    std::chrono::steady_clock::time_point m_startTime;
    std::chrono::steady_clock::time_point m_pausedTime;
    int                                   m_round;
    float                                 m_progress;
    int                                   m_sessionLength;
    int                                   m_restLength;
    bool                                  m_pause;
    bool                                  m_autoTransition;
    State                                 m_currentState;
    State                                 m_lastState;
    
    std::function<void(State)>            m_onSessionEnd;
};