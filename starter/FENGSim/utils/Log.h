#pragma once
#include <QDebug>
#include <QThread>
#include <QDateTime>
#include <cstring>   // for strrchr

// ===== 终端颜色（ANSI） =====
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_RESET   "\033[0m"

// ===== 提取文件名（不带路径） =====
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

// ===== 获取当前时间戳（精确到毫秒） =====
inline QString logTimeMs()
{
    return QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
}

// ===== 根据 Debug / Release 自动启用/关闭日志 =====
// Qt 下：Release 通常会定义 QT_NO_DEBUG
#ifdef QT_NO_DEBUG       // Release：日志全部关掉（不产生输出，也不影响语法）
    #define LOGD if (false) qDebug()
    #define LOGW if (false) qWarning()
    #define LOGE if (false) qCritical()
#else                    // Debug：正常输出详细日志
    #define LOGD qDebug().noquote() << COLOR_GREEN \
        << logTimeMs() \
        << "[D]" << __FILENAME__ << ":" << __LINE__ \
        << "(" << QThread::currentThreadId() << ")" \
        << __FUNCTION__ << ":" << COLOR_RESET

    #define LOGW qWarning().noquote() << COLOR_YELLOW \
        << logTimeMs() \
        << "[W]" << __FILENAME__ << ":" << __LINE__ \
        << "(" << QThread::currentThreadId() << ")" \
        << __FUNCTION__ << ":" << COLOR_RESET

    #define LOGE qCritical().noquote() << COLOR_RED \
        << logTimeMs() \
        << "[E]" << __FILENAME__ << ":" << __LINE__ \
        << "(" << QThread::currentThreadId() << ")" \
        << __FUNCTION__ << ":" << COLOR_RESET
#endif

