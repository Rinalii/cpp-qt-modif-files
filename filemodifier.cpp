#include "filemodifier.h"

#include <QFile>
#include <QDebug>
#include <QDir>

FileModifier::FileModifier(QObject *parent)
    : QObject(parent) {
}

bool FileModifier::ModifyFile(const QString &input_filepath, const QString &output_filepath) {
    QFile in_file(input_filepath);
    if (!in_file.open(QIODevice::ReadOnly)) {
        emit signalFinished(false, "Cannot open input file: " + input_filepath);
        return false;
    }
    QFile out_file(output_filepath);
    if (!out_file.open(QIODevice::WriteOnly)) {
        in_file.close();
        emit signalFinished(false, "Cannot open output file: " + output_filepath);
        return false;
    }

    const qint64 buffer_size = 1024 * 1024;
    const int key_size = key_.size();
    QByteArray chunk;

    while (!in_file.atEnd()) {
        if (exit_requested_.load()) {
            out_file.close();
            QFile::remove(output_filepath);
            in_file.close();
            emit signalFinished(false, "Operation cancelled by user");
            return false;
        }

        chunk = in_file.read(buffer_size);
        for (int i = 0; i < chunk.size(); ++i) {
            chunk[i] ^= key_[i % key_size];
        }
        out_file.write(chunk);

        bytes_processed_ += chunk.size();
        int percent = (bytes_processed_ * 100) / total_bytes_;
        emit signalProgress(percent);
    }

    in_file.close();
    out_file.close();
    return true;
}

bool FileModifier::IsKeyValid(const QByteArray &key) {
    if (key.isEmpty()) {
        emit signalFinished(false, "Key cannot be empty");
        return false;
    }
    if (key.size() != 8) {
        emit signalFinished(false, "Key must be exactly 8 bytes (16 hex digits)");
        return false;
    }
    return true;
}

void FileModifier::ModifyFiles(const QString &input_path, const QString &output_path, bool modify_filename, const QString &mask, const QByteArray &key, bool remove_source) {
    if (!IsKeyValid(key)) return;
    input_path_ = input_path;
    output_path_ = output_path;
    modify_filename_ = modify_filename;
    key_ = key;
    remove_source_ = remove_source;
    exit_requested_.store(false);

    filenames_ = GetSuitableFileNames(input_path, mask);
    bytes_processed_ = 0;

    if(!CalculateTotalBytes()) return;

    for (const QString &filename : filenames_) {
        if (exit_requested_.load()) return;
        QString input_file = input_path + "/" + filename;
        QString output_file = output_path + "/" + GetOutputFilename(output_path, filename, modify_filename_);
        if(!ModifyFile(input_file, output_file)) return;
    }
}

void FileModifier::slotExitRequested() {
    exit_requested_.store(true);
}

QStringList FileModifier::GetSuitableFileNames(const QString &in_path, const QString &mask) {
    QDir dir(in_path);
    if (!dir.exists()) {
        emit signalFinished(false, "Directory does not exist");
        return QStringList{};
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
    return dir.entryList();
}

bool FileModifier::CalculateTotalBytes() {
    for (const QString &filename : filenames_) {
        QString input_file = input_path_ + "/" + filename;
        if (!QFileInfo::exists(input_file)) {
            emit signalFinished(false, "File " + input_file + " doesn't exists");
            return false;
        }
        total_bytes_ += QFileInfo(input_file).size();
    }
    return true;
}

QString FileModifier::IncrementNumber(const QString &str) {
    QString result;

    int carry = 1;
    int i = str.size()-1;
    for(; i>=0 && carry != 0; --i) {
        if(!str[i].isDigit()) break;
        int digit = str[i].digitValue() + carry;
        if(digit == 10) {
            result += '0';
            carry = 1;
        } else {
            result += QChar(digit + '0');
            carry = 0;
        }
    }
    if(carry == 1) {
        result +='1';
    }
    std::reverse(result.begin(), result.end());
    result = str.left(i+1) + result;
    return result;
}

QString FileModifier::GetOutputFilename(const QString &output_path, const QString &filename, bool modify_filename) {
    if(QFileInfo::exists(output_path + "/" + filename) && modify_filename) {
        QFileInfo info(filename);
        QString base_name = info.baseName();
        QString suffix = info.suffix();
        QString new_base = IncrementNumber(base_name);
        if (!suffix.isEmpty()) {
            return new_base + "." + suffix;
        } else {
            return new_base;
        }
    }
    return filename;
}
