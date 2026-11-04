


// class SessionService : public QObject {
//     Q_OBJECT
// public:
//     static SessionService& instance();

//     Q_INVOKABLE void createSession(const QString& charId, const QString& title);
//     Q_INVOKABLE void deleteSession(const QString& charId, const QString& sessionId);
//     Q_INVOKABLE void renameSession(const QString& charId, const QString& sessionId, const QString& newTitle);
//     Q_INVOKABLE QVariantList getSessions(const QString& charId);

// signals:
//     void sessionCreated(const QString& charId, const QString& sessionId);
//     void sessionDeleted(const QString& charId, const QString& sessionId);
//     void sessionRenamed(const QString& charId, const QString& sessionId);
//     void operationFailed(const QString& error);
// };