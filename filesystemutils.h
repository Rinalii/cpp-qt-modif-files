#ifndef FILESYSTEMUTILS_H
#define FILESYSTEMUTILS_H

#include <QFile>
#include <QStringList>

namespace FileSystemUtils {
    bool GetSuitableFileNames(const QString &input_path, const QString &mask, QStringList &result_filenames, QString &error_msg);
    bool EnsureDirectoryExists(const QString &path, QString &error_msg);
    bool GetOutputFilename(const QString &output_path, const QString &curr_filename, bool modify_filename_if_exists, QString &result_filename, QString &error_msg);
    bool OpenFile(QFile &file, QIODeviceBase::OpenMode mode, const QString &filepath, qint64 offset, QString &error_msg);
    bool CalculateTotalBytes(const QString &input_path, const QStringList &filenames, qint64 &total_bytes, QString &error_msg);
} // namespace FileSystemUtils

#endif // FILESYSTEMUTILS_H
