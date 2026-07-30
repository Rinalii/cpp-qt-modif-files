#ifndef FILEMODIFIER_H
#define FILEMODIFIER_H

#include <QString>
#include <QObject>
#include <atomic>

class FileModifier : public QObject
{
    Q_OBJECT
public:
    explicit FileModifier(QObject *parent = nullptr);

    void modifyFile(const QString &input_path, const QString &output_path, const QByteArray &key, bool remove_source);

signals:
    void progress(int percent);
    void finished(bool success, const QString &errorMessage);

public slots:
    void slotExitRequested();

private:
    QString input_path_;
    QString output_path_;
    QByteArray key_;
    bool remove_source_;
    std::atomic<bool> exit_requested_{false};
};

#endif // FILEMODIFIER_H
