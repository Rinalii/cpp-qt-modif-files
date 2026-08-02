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
    void ModifyFiles(const QString &input_path, const QString &output_path, bool modify_filename, const QString& mask, const QByteArray &key, bool remove_source);

signals:
    void signalProgress(int percent);
    void signalFinished(bool success, const QString &errorMessage);

public slots:
    void slotExitRequested();

private:
    QString input_path_;
    QString output_path_;
    QStringList filenames_;
    bool modify_filename_;
    QByteArray key_;
    bool remove_source_;
    std::atomic<bool> exit_requested_{false};
    qint64 bytes_processed_;
    qint64 total_bytes_;

    bool ModifyFile(const QString &input_filepath, const QString &output_filepath);
    bool IsKeyValid(const QByteArray &key);
    QStringList GetSuitableFileNames(const QString& in_path, const QString& mask);
    bool CalculateTotalBytes();

    QString GetOutputFilename(const QString &output_path, const QString &filename) const;
    static QString IncrementNumber(const QString &str);
    static QString GetOutputFilename(const QString &output_path, const QString &filename, bool modify_filename);
};

#endif // FILEMODIFIER_H
