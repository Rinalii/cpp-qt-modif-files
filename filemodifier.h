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

signals:
    void signalProgress(int percent);
    void signalFinished(bool success, const QString &errorMessage);

public slots:
    void slotExitRequested();
    void slotPauseRequested();
    void slotResumeRequested();

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

    struct RecoverySettings {
        int curr_file_index_{0};
        qint64 curr_file_offset_{0};
        QString curr_input_file_;
        QString curr_output_file_;

        void Clear() {
            curr_file_index_ = 0;
            curr_file_offset_ = 0;
            curr_input_file_.clear();
            curr_output_file_.clear();
        }
    };

    bool ModifyFile(const QString &input_filepath, const QString &output_filepath);
    bool IsKeyValid(const QByteArray &key);
    QStringList GetSuitableFileNames(const QString& in_path, const QString& mask);
    bool CalculateTotalBytes();

    QString GetOutputFilename(const QString &output_path, const QString &filename) const;
    static QString IncrementNumber(const QString &str);
    static QString GetOutputFilename(const QString &output_path, const QString &filename, bool modify_filename);
    void WaitForResume();

    static bool OpenFile(QFile& file, QIODevice::OpenMode mode, const QString& filepath, qint64 offset, QString& error_msg);

private:
    // Настройки для обработки файлов
    Settings settings_;

    qint64 bytes_processed_;
    qint64 total_bytes_;

    // Настройки для возобновления обработки с места, на котором остановились
    RecoverySettings recovery_settings_;

    // Выход / пауза
    std::atomic<bool> exit_requested_{false};
    std::atomic<bool> pause_requested_{false};
    std::atomic<bool> is_paused_{false};

    // Для приостановки потока, чтобы он не нагружал систему
    QMutex wait_mutex_;               // для синхронизации с QWaitCondition
    QWaitCondition wait_cond_;
};

#endif // FILEMODIFIER_H
