#include "filesystemutils.h"

#include <QDir>

namespace FileSystemUtils {

QString CreateFilename(const QString &new_base, const QString &suffix);

bool GetSuitableFileNames(const QString &input_path, const QString &mask, QStringList &result_filenames, QString &error_msg) {
    QDir dir(input_path);
    if (!dir.exists()) {
        error_msg = "Directory does not exist";
        return false;
    }

    QStringList filters;
    if (mask.isEmpty()) {
        // По умолчанию – все файлы
        filters << "*";
    } else {
        QStringList parts = mask.split(',', Qt::SkipEmptyParts);
        for (QString part : parts) {
            part = part.trimmed();
            if (part.isEmpty()) continue;
            if (part.contains('*') || part.contains('?')) {
                filters << part;
            } else if (part.startsWith('.')) {  // Начинается с точки – считаем расширением, добавляем "*"
                filters << "*" + part;
            } else {
                filters << part;                // точное имя
            }
        }
    }
    if (filters.isEmpty()) {
        filters << "*";
    }

    dir.setNameFilters(filters);
    dir.setFilter(QDir::Files | QDir::NoDotAndDotDot);
    result_filenames = dir.entryList();
    return true;
}

bool EnsureDirectoryExists(const QString &path, QString &error_msg) {
    error_msg.clear();
    if (path.isEmpty()) {
        error_msg = "path is empty";
        return false;
    }

    QDir dir(path);
    if (dir.exists()) {
        return true;
    }

    if (QDir().mkpath(path)) {
        return true;
    }

    error_msg = "Failed to create directory: \"" + path + "\". ";
    return false;
}

bool GetOutputFilename(const QString &output_path, const QString &curr_filename, bool modify_filename_if_exists, QString &result_filename, QString &error_msg) {
    error_msg.clear();
    if (output_path.isEmpty()) {
        error_msg = "output_path is empty";
        return false;
    }
    if (curr_filename.isEmpty()) {
        error_msg = "curr_filename is empty";
        return false;
    }
    if(QFileInfo::exists(output_path + "/" + curr_filename) && modify_filename_if_exists) {
        QFileInfo info(curr_filename);
        QString base_name = info.baseName();
        QString suffix = info.suffix();

        int i = 1;
        QString new_filename = CreateFilename(base_name + QString("_%1").arg(i), suffix);

        while(QFileInfo::exists(output_path + "/" + new_filename)) {
            ++i;
            new_filename = CreateFilename(base_name + QString("_%1").arg(i), suffix);
        }
        result_filename = new_filename;
        return true;
    }

    result_filename = curr_filename;
    return true;
}

bool OpenFile(QFile &file, QIODeviceBase::OpenMode mode, const QString &filepath, qint64 offset, QString &error_msg) {
    error_msg.clear();
    if (!file.open(mode)) {
        error_msg = "Cannot open file: " + filepath;
        return false;
    }
    if (offset > 0) {
        if (!file.seek(offset)) {
            error_msg = "Cannot seek in file: " + filepath;
            file.close();
            return false;
        }
    }
    return true;
}

bool CalculateTotalBytes(const QString &input_path, const QStringList &filenames, qint64 &result_total_bytes, QString &error_msg) {\
    error_msg.clear();
    result_total_bytes = 0;
    for (const QString &filename : filenames) {
        QString input_file = input_path + "/" + filename;
        if (!QFileInfo::exists(input_file)) {
            error_msg = "File " + input_file + " doesn't exists";
            return false;
        }
        result_total_bytes += QFileInfo(input_file).size();
    }
    return true;
}

QString CreateFilename(const QString &new_base, const QString &suffix) {
    if (!suffix.isEmpty()) {
        return new_base + "." + suffix;
    } else {
        return new_base;
    }
}

} // namespace FileSystemUtils
