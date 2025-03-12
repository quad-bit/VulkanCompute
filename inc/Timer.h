#pragma once
#include <chrono>
#include <thread>

class Timer
{
private:
    uint32_t m_frameCounter;
    double m_targetFrameDuration;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_startPoint;
public:

    Timer(uint32_t targetFPS) : m_frameCounter(0), m_targetFrameDuration(1.0/targetFPS)
    {
        m_startPoint = std::chrono::high_resolution_clock::now();
    }

    void Update()
    {
        m_frameCounter++;
        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsedTime = now - m_startPoint;

        if (elapsedTime.count() >= 1.0)
        {
            //std::cout << "fps : " << m_frameCounter / elapsedTime.count() << "\n";

            m_frameCounter = 0;
            m_startPoint = now;
        }

        // limiter
        auto frameEndTime = std::chrono::high_resolution_clock::now();
        auto frameDuration = frameEndTime - now;

        if (frameDuration.count() < m_targetFrameDuration)
        {
            //std::this_thread::sleep_for(std::chrono::duration<double>(m_targetFrameDuration - frameDuration.count()));
        }
    }

    void Sleep(double milliSeconds)
    {
        std::this_thread::sleep_for(std::chrono::duration<double>(milliSeconds));
    }

};