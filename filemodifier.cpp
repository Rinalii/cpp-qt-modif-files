#include "filemodifier.h"

#include <QFile>
#include <QDebug>

FileModifier::FileModifier(QObject *parent)
    : QObject(parent) {
}

void FileModifier::modifyFile(const QString &input_path, const QString &output_path, const QByteArray &key, bool remove_source) {
    input_path_ = input_path;
    output_path_ = output_path;
    key_ = key;
    remove_source_ = remove_source;
    exit_requested_ = false;

    if (key_.isEmpty()) {
        emit finished(false, "Key cannot be empty");
        return;
    }
    if (key_.size() != 8) {
        emit finished(false, "Key must be exactly 8 bytes (16 hex digits)");
        return;
    }

    QFile in_file(input_path_);
    if (!in_file.open(QIODevice::ReadOnly)) {
        emit finished(false, "Cannot open input file: " + input_path_);
        return;
    }
    QFile out_file(output_path_);
    if (!out_file.open(QIODevice::WriteOnly)) {
        emit finished(false, "Cannot open output file: " + output_path_);
        return;
    }

    qint64 total_bytes = in_file.size();
    const qint64 buffer_size = 1024 * 1024; // 1 МБ
    const int key_size = key_.size();   // 8 Б

    QByteArray chunk;
    qint64 bytes_processed = 0;

    while (!in_file.atEnd()) {
        if (exit_requested_.load()) {
            out_file.close();
            QFile::remove(output_path_); // удаляем неполный выходной файл
            emit finished(false, "Operation cancelled by user");
            return;
        }

        chunk = in_file.read(buffer_size);
        for (int i = 0; i < chunk.size(); ++i) {
            chunk[i] ^= key_[i % key_size];
        }
        out_file.write(chunk);

        bytes_processed += chunk.size();
        int percent = static_cast<int>((bytes_processed * 100) / total_bytes);
        emit progress(percent); // отправляем сигнал в GUI
    }

    in_file.close();
    out_file.close();

    if (remove_source_) {
        if (!QFile::remove(input_path_)) {
            emit finished(false, "Cannot remove source file: " + input_path_);
            return;
        }
    }
    emit finished(true, QString());
}

void FileModifier::slotExitRequested() {
    exit_requested_.store(true);
}
