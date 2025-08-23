#include "pomodoro.hpp"
#include <chrono>

Pomodoro::Pomodoro(const int sessionLengthMinutes, const int restLengthMinutes) {
    m_sessionLength = sessionLengthMinutes;
    m_restLength    = restLengthMinutes;
    m_progress      = 0.0f;
    m_round         = 0;
    m_currentState  = State::STOPPED;
    m_pause         = false;
}

Pomodoro::~Pomodoro() {
    stop();
}

void Pomodoro::start() {
    m_startTime    = std::chrono::steady_clock::now();
    m_currentState = State::WORKING;
    m_pause        = false;
    m_progress     = 0.0f;
}

void Pomodoro::startRest() {
    m_startTime    = std::chrono::steady_clock::now();
    m_currentState = State::RESTING;
    m_pause        = false;
    m_progress     = 0.0f;
}

void Pomodoro::stop() {
    m_progress     = 0.0f;
    m_round        = 0;
    m_currentState = State::STOPPED;
    m_pause        = false;
}

void Pomodoro::pause() {
    if (!m_pause && m_currentState != State::STOPPED) {
        m_pausedTime = std::chrono::steady_clock::now();
        m_pause      = true;
    }
}

void Pomodoro::resume() {
    if (m_pause) {
        auto pauseDuration = std::chrono::steady_clock::now() - m_pausedTime;
        m_startTime += pauseDuration;
        m_pause = false;
    }
}

void Pomodoro::reset() {
    m_currentState == State::WORKING ? start() : startRest();
}

void Pomodoro::restart() {
    stop();
    start();
}

void Pomodoro::skip() {
    if (m_currentState == State::RESTING) {
        start();
    } else if (m_currentState == State::WORKING) {
        startRest();
    }
}

void Pomodoro::setRestLength(int minutes) {
    m_restLength = minutes;
}

void Pomodoro::setSessionLength(int minutes) {
    m_sessionLength = minutes;
}

int Pomodoro::getRestLength() const {
    return m_restLength;
}

int Pomodoro::getRound() const {
    return m_round;
}

float Pomodoro::getProgress() {
    if (m_currentState == State::STOPPED)
        return 0.0f;

    const int   lengthMS         = (m_currentState == State::WORKING ? m_sessionLength : m_restLength) * 60 * 1000;
    const int   currentRemaining = getRemainingTime();

    const float progress = (float)currentRemaining / lengthMS;

    return std::clamp(m_currentState == State::WORKING ? 1.0f - progress : progress, 0.0f, 1.0f);
}

int Pomodoro::getRemainingTime() const {
    if (m_currentState == State::STOPPED)
        return 0;
    auto      now      = std::chrono::steady_clock::now();
    auto      elapsed  = std::chrono::duration_cast<std::chrono::milliseconds>(m_pause ? m_pausedTime - m_startTime : now - m_startTime).count();
    const int lengthMS = (m_currentState == State::WORKING ? m_sessionLength : m_restLength) * 60 * 1000;

    return std::max(0, lengthMS - (int)elapsed);
}

std::string Pomodoro::getFormattedTime() const {
    int  remaining = getRemainingTime();
    int  minutes   = remaining / 60000;
    int  seconds   = (remaining % 60000) / 1000;
    char buffer[11];
    snprintf(buffer, sizeof(buffer), "%02d:%02d", minutes, seconds);
    return std::string(buffer);
}

State Pomodoro::getState() {
    isFinished();
    return m_currentState;
}

int Pomodoro::getSessionLength() const {
    return m_sessionLength;
}

bool Pomodoro::isPaused() {
    return m_pause;
}

bool Pomodoro::isFinished() {
    if (getRemainingTime() <= 0 && m_currentState == State::WORKING) {
        this->m_round++;
        m_currentState = State::FINISHED;
        startRest();
        return true;
    }
    if (getRemainingTime() <= 0 && m_currentState == State::RESTING) {
        start();
        return true;
    }
    return false;
}