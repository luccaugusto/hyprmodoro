#include "pomodoro.hpp"

Pomodoro::Pomodoro(const int sessionLengthMinutes, const int restLengthMinutes) {
    m_sessionLength  = sessionLengthMinutes;
    m_restLength     = restLengthMinutes;
    m_progress       = 0.0f;
    m_round          = 0;
    m_currentState   = PomodoroState::STOPPED;
    m_lastState      = PomodoroState::STOPPED;
    m_pause          = false;
    m_autoTransition = true;
}

Pomodoro::~Pomodoro() {
    stop();
}

void Pomodoro::start() {
    m_startTime = std::chrono::steady_clock::now();
    setState(PomodoroState::WORKING);
    m_pause    = false;
    m_progress = 0.0f;
}

void Pomodoro::startRest() {
    m_startTime = std::chrono::steady_clock::now();
    setState(PomodoroState::RESTING);
    m_pause    = false;
    m_progress = 0.0f;
}

void Pomodoro::stop() {
    m_progress = 0.0f;
    m_round    = 0;
    setState(PomodoroState::STOPPED);
    m_pause = false;
}

void Pomodoro::pause() {
    if (!m_pause && m_currentState != PomodoroState::STOPPED) {
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
    m_currentState == PomodoroState::WORKING ? start() : startRest();
}

void Pomodoro::restart() {
    stop();
    start();
}

void Pomodoro::skip() {
    if (m_currentState == PomodoroState::RESTING) {
        start();
    } else if (m_currentState == PomodoroState::WORKING) {
        startRest();
    }
}

void Pomodoro::setState(PomodoroState newState) {
    m_lastState    = m_currentState;
    m_currentState = newState;
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
    if (m_currentState == PomodoroState::STOPPED)
        return 0.0f;

    const int   lengthMS         = (m_currentState == PomodoroState::WORKING ? m_sessionLength : m_restLength) * 60 * 1000;
    const int   currentRemaining = getRemainingTime();

    const float progress = (float)currentRemaining / lengthMS;

    return std::clamp(m_currentState == PomodoroState::WORKING ? 1.0f - progress : progress, 0.0f, 1.0f);
}

int Pomodoro::getRemainingTime() const {
    if (m_currentState == PomodoroState::STOPPED ||
        m_currentState == PomodoroState::WAITING_FOR_REST ||
        m_currentState == PomodoroState::WAITING_FOR_WORK)
        return 0;

    auto      now      = std::chrono::steady_clock::now();
    auto      elapsed  = std::chrono::duration_cast<std::chrono::milliseconds>(m_pause ? m_pausedTime - m_startTime : now - m_startTime).count();
    const int lengthMS = (m_currentState == PomodoroState::WORKING ? m_sessionLength : m_restLength) * 60 * 1000;

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

PomodoroState Pomodoro::getState() {
    isFinished();
    return m_currentState;
}

PomodoroState Pomodoro::getLastState() const {
    return m_lastState;
}

int Pomodoro::getSessionLength() const {
    return m_sessionLength;
}

bool Pomodoro::isPaused() {
    return m_pause;
}

bool Pomodoro::isFinished() {
    if (getRemainingTime() <= 0 && m_currentState == PomodoroState::WORKING) {
        this->m_round++;

        if (m_onSessionEnd) {
            m_onSessionEnd(PomodoroState::WORKING);
        }

        if (m_autoTransition) {
            startRest();
        } else {
            setState(PomodoroState::WAITING_FOR_REST);
            Pomodoro::pause();
        }
        return true;
    }

    if (getRemainingTime() <= 0 && m_currentState == PomodoroState::RESTING) {
        if (m_onSessionEnd) {
            m_onSessionEnd(PomodoroState::RESTING);
        }

        if (m_autoTransition) {
            start();
        } else {
            setState(PomodoroState::WAITING_FOR_WORK);
            Pomodoro::pause();
        }
        return true;
    }

    return false;
}

void Pomodoro::setOnSessionEndCallback(std::function<void(PomodoroState)> callback) {
    m_onSessionEnd = callback;
}

void Pomodoro::setAutoTransition(bool autoTransition) {
    m_autoTransition = autoTransition;
}

bool Pomodoro::getAutoTransition() const {
    return m_autoTransition;
}
