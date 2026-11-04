#ifndef SCOPETIMER_H
#define SCOPETIMER_H

#include <QElapsedTimer>
#include <QString>
#include <QDebug>
#include <functional>

/*
基本用法
ScopeTimer t("heavyOperation");
ScopeTimer t2("step1: parse");

带上下文
ScopeTimer t(QString("createPrompts %1条").arg(n));

关掉
ScopeTimer::setEnabled(false);

接到项目日志
ScopeTimer::setReporter([](const QString &tag, qint64 ms){
    LOG_INFO("perf") << tag << ms;
});
*/


class ScopeTimer
{
public:
    using Reporter = std::function<void(const QString &tag, qint64 ms)>;

    static void setEnabled(bool on) noexcept { s_enabled = on; }
    static void setReporter(Reporter r)      { s_reporter = std::move(r); }

    explicit ScopeTimer(QString tag)
        : m_tag(std::move(tag))
    {
        if (s_enabled) m_timer.start();
    }

    ~ScopeTimer()
    {
        if (s_enabled && !m_tag.isEmpty())
            s_reporter(m_tag, m_timer.elapsed());
    }

    ScopeTimer(const ScopeTimer &)            = delete;
    ScopeTimer &operator=(const ScopeTimer &) = delete;

    qint64 elapsed() const { return s_enabled ? m_timer.elapsed() : 0; }

    // 中途手动收尾，析构不再输出
    void stop()
    {
        if (s_enabled && !m_tag.isEmpty()) {
            s_reporter(m_tag, m_timer.elapsed());
            m_tag.clear();
        }
    }

private:
    QString       m_tag;
    QElapsedTimer m_timer;

    static inline bool     s_enabled  = true;
    static inline Reporter s_reporter = [](const QString &tag, qint64 ms) {
        qDebug().nospace() << "[" << tag << "] 耗时 " << ms << " ms";
    };
};

#endif
