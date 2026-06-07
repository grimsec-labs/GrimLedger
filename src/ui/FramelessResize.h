#pragma once

#include <QByteArray>
#include <QWidget>

// Edge resize hit-testing for frameless windows (Windows native).
class FramelessResize {
public:
    static constexpr int kBorderWidth = 6;

    static bool handleNativeEvent(
        QWidget* window,
        const QByteArray& eventType,
        void* message,
        qintptr* result,
        int borderWidth = kBorderWidth);
};
