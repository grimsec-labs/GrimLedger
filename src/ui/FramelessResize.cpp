#include "ui/FramelessResize.h"

#ifdef Q_OS_WIN
#include <windows.h>
#include <windowsx.h>
#endif

bool FramelessResize::handleNativeEvent(
    QWidget* window,
    const QByteArray& eventType,
    void* message,
    qintptr* result,
    int borderWidth) {
#ifdef Q_OS_WIN
    if (!window || window->isMaximized()) {
        return false;
    }

    if (eventType != "windows_generic_MSG" && eventType != "windows_dispatcher_MSG") {
        return false;
    }

    const auto* msg = static_cast<MSG*>(message);
    if (msg->message != WM_NCHITTEST) {
        return false;
    }

    RECT rect;
    if (!GetWindowRect(msg->hwnd, &rect)) {
        return false;
    }

    const LONG x = GET_X_LPARAM(msg->lParam);
    const LONG y = GET_Y_LPARAM(msg->lParam);
    const LONG left = rect.left;
    const LONG top = rect.top;
    const LONG right = rect.right;
    const LONG bottom = rect.bottom;
    const LONG bw = borderWidth;

    const bool atLeft = x >= left && x < left + bw;
    const bool atRight = x < right && x >= right - bw;
    const bool atTop = y >= top && y < top + bw;
    const bool atBottom = y < bottom && y >= bottom - bw;

    if (atTop && atLeft) {
        *result = HTTOPLEFT;
        return true;
    }
    if (atTop && atRight) {
        *result = HTTOPRIGHT;
        return true;
    }
    if (atBottom && atLeft) {
        *result = HTBOTTOMLEFT;
        return true;
    }
    if (atBottom && atRight) {
        *result = HTBOTTOMRIGHT;
        return true;
    }
    if (atLeft) {
        *result = HTLEFT;
        return true;
    }
    if (atRight) {
        *result = HTRIGHT;
        return true;
    }
    if (atTop) {
        *result = HTTOP;
        return true;
    }
    if (atBottom) {
        *result = HTBOTTOM;
        return true;
    }
#else
    Q_UNUSED(window);
    Q_UNUSED(eventType);
    Q_UNUSED(message);
    Q_UNUSED(result);
    Q_UNUSED(borderWidth);
#endif
    return false;
}
