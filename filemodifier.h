#ifndef FILEMODIFIER_H
#define FILEMODIFIER_H

#include <QString>
#include <QObject>
#include <atomic>
#include <QMutex>
#include <QWaitCondition>
#include <QFile>

class FileModifier : public QObject
{
    Q_OBJECT
public:
    explicit FileModifier(QObject *parent = nullptr);
    void ModifyFiles(const QString &input_path, const QString &output_path, bool modify_filename, const QString& mask, const QByteArray &key, bool remove_source);

    void RequestExit();
    void RequestPause();
    void RequestResume();

signals:
    void signalProgress(int percent);
    void signalFinished(bool success, const QString &error_message);
    void signalStartFileModification(const QString &filename);

private:
    struct Settings {
        QString input_path_;
        QString output_path_;
        QStringList filenames_;
        bool modify_filename_ = false;
        QByteArray key_;
        bool remove_source_ = false;

        void Clear() {
            input_path_.clear();
            output_path_.clear();
            filenames_.clear();
            modify_filename_ = false;
            key_.clear();
            remove_source_ = false;
        }
    };

    static bool IsKeyValid(const QByteArray &key, QString &error_msg);

    static bool OpenFiles(QFile &inFile, QFile &outFile, const QString &input_filepath,
                          const QString &output_filepath, qint64 offset, QString &error_msg);

    bool PrepareProcessing(const QString &input_path, const QString &output_path,
                           bool modify_filename, const QString &mask, const QByteArray &key, bool remove_source, QString &error_msg);

    bool ProcessOneFile(const QString &filename, QString &error_msg);

    bool ModifyFile(const QString &input_filepath, const QString &output_filepath, QString &error_msg);

    void WaitForResume();

    bool ProcessFileData(QFile &inFile, QFile &outFile, const QByteArray &key, qint64 total_bytes,
                qint64 &bytes_processed, QString &error_msg);

    bool HandlePause(QFile &inFile, QFile &outFile, const QString &input_filepath, const QString &output_filepath,
                qint64 &offset, QString &error_msg);

private:
    // Настройки для обработки файлов
    Settings settings_;

    qint64 bytes_processed_;
    qint64 total_bytes_;

    // Выход / пауза
    std::atomic<bool> exit_requested_{false};
    std::atomic<bool> pause_requested_{false};
    std::atomic<bool> is_paused_{false};

    // Для приостановки потока, чтобы он не нагружал систему
    QMutex wait_mutex_;               // для синхронизации с QWaitCondition
    QWaitCondition wait_cond_;
};

#endif // FILEMODIFIER_H
