#include "filemodifier.h"

#include <QFile>
#include <QDebug>
#include <QDir>

#include "filesystemutils.h"

FileModifier::FileModifier(QObject *parent)
    : QObject(parent) {
}

void FileModifier::ModifyFiles(const QString &input_path, const QString &output_path, bool modify_filename,
            const QString &mask, const QByteArray &key, bool remove_source) {

    QString error_msg;
    if (!IsKeyValid(key, error_msg)) {
        emit signalFinished(false, error_msg);
        return;
    }

    if (!PrepareProcessing(input_path, output_path, modify_filename, mask, key, remove_source, error_msg)) {
        emit signalFinished(false, error_msg);
        return;
    }

    for (const QString &filename : settings_.filenames_) {
        if (exit_requested_.load()) {
            emit signalFinished(false, "Operation cancelled by user");
            return;
        }

        if (!ProcessOneFile(filename, error_msg)) {
            emit signalFinished(false, error_msg);
            return;
        }
    }

    emit signalFinished(true, QString());
}

bool FileModifier::ProcessOneFile(const QString &filename, QString &error_msg) {

    error_msg.clear();

    if (pause_requested_.load()) {
        is_paused_.store(true);
        WaitForResume();
        if (exit_requested_.load()) {
            error_msg = "Operation cancelled by user";
            return false;
        }
    }

    QString input_file = settings_.input_path_ + "/" + filename;
    QString result_filename;

    if (!FileSystemUtils::GetOutputFilename(settings_.output_path_, filename, settings_.modify_filename_, result_filename, error_msg)) {
        return false;
    }

    QString output_file = settings_.output_path_ + "/" + result_filename;

    emit signalStartFileModification(filename);

    if (!ModifyFile(input_file, output_file, error_msg)) {
        return false;
    }

    if (settings_.remove_source_ && input_file != output_file) {
        QFile::remove(input_file);
    }

    return true;
}

bool FileModifier::ModifyFile(const QString &input_filepath, const QString &output_filepath, QString &error_msg) {
    error_msg.clear();

    QFile in_file(input_filepath);
    QFile out_file(output_filepath);
    qint64 offset = 0;

    if (!OpenFiles(in_file, out_file, input_filepath, output_filepath, offset, error_msg)) {
        return false;
    }

    bool success = ProcessFileData(in_file, out_file, settings_.key_, total_bytes_, bytes_processed_, error_msg);

    in_file.close();
    out_file.close();

    if (!success) {
        QFile::remove(output_filepath);
        return false;
    }

    return true;
}

bool FileModifier::PrepareProcessing(const QString &input_path, const QString &output_path,
                bool modify_filename, const QString &mask, const QByteArray &key, bool remove_source, QString &error_msg) {

    settings_.input_path_ = input_path;
    settings_.output_path_ = output_path;
    settings_.modify_filename_ = modify_filename;
    settings_.remove_source_ = remove_source;
    settings_.key_ = key;

    if (!FileSystemUtils::GetSuitableFileNames(input_path, mask, settings_.filenames_, error_msg)) {
        return false;
    }

    exit_requested_.store(false);
    pause_requested_.store(false);
    is_paused_.store(false);

    bytes_processed_ = 0;
    if (!FileSystemUtils::CalculateTotalBytes(settings_.input_path_, settings_.filenames_, total_bytes_, error_msg)) {
        return false;
    }

    if(total_bytes_ == 0) {
        error_msg = "total_bytes_ == 0";
        return false;
    }

    if (!FileSystemUtils::EnsureDirectoryExists(output_path, error_msg)) {
        return false;
    }

    return true;
}

bool FileModifier::OpenFiles(QFile &in_file, QFile &out_file, const QString &input_filepath,
                const QString &output_filepath, qint64 offset, QString &error_msg) {

    error_msg.clear();

    if (!FileSystemUtils::OpenFile(in_file, QIODevice::ReadOnly, input_filepath, offset, error_msg)) {
        return false;
    }

    QIODevice::OpenMode mode = QIODevice::ReadWrite;
    if (offset == 0) {
        mode |= QIODevice::Truncate;
    }

    if (!FileSystemUtils::OpenFile(out_file, mode, output_filepath, offset, error_msg)) {
        in_file.close();
        return false;
    }
    return true;
}

bool FileModifier::ProcessFileData(QFile &in_file, QFile &out_file, const QByteArray &key, qint64 total_bytes,
                qint64 &bytes_processed, QString &error_msg) {

    error_msg.clear();
    const qint64 buffer_size = 1024 * 1024;
    const int key_size = key.size();
    QByteArray chunk;

    while (!in_file.atEnd()) {
        if (exit_requested_.load()) {
            error_msg = "Operation cancelled by user";
            return false;
        }

        if (pause_requested_.load()) {
            qint64 offset = in_file.pos();
            if (!HandlePause(in_file, out_file, in_file.fileName(), out_file.fileName(), offset, error_msg)) {
                return false;
            }
        }

        chunk = in_file.read(buffer_size);
        for (int i = 0; i < chunk.size(); ++i) {
            chunk[i] ^= key[i % key_size];
        }
        out_file.write(chunk);

        bytes_processed += chunk.size();
        int percent = (bytes_processed * 100) / total_bytes;
        emit signalProgress(percent);
    }
    return true;
}

bool FileModifier::HandlePause(QFile &in_file, QFile &out_file, const QString &input_filepath, const QString &output_filepath,
                qint64 &offset, QString &error_msg) {

    error_msg.clear();

    in_file.close();
    out_file.flush();
    out_file.close();

    is_paused_.store(true);
    WaitForResume();   // блокирует поток до вызова RequestResume()

    if (exit_requested_.load()) {
        error_msg = "Operation cancelled by user";
        return false;
    }

    if (!OpenFiles(in_file, out_file, input_filepath, output_filepath, offset, error_msg)) {
        return false;
    }

    return true;
}

bool FileModifier::IsKeyValid(const QByteArray &key, QString &error_msg) {
    error_msg.clear();
    if (key.isEmpty()) {
        error_msg = "Key cannot be empty";
        return false;
    }
    if (key.size() != 8) {
        error_msg = "Key must be exactly 8 bytes (16 hex digits)";
        return false;
    }
    return true;
}

void FileModifier::WaitForResume()
{
    QMutexLocker locker(&wait_mutex_);
    while (pause_requested_.load() && !exit_requested_.load()) {
        wait_cond_.wait(&wait_mutex_);  //отпускаем мьютекс и блокируем текущий поток до wake
    }
    is_paused_.store(false);
}

void FileModifier::RequestExit() {
    exit_requested_.store(true);
    QMutexLocker locker(&wait_mutex_);
    wait_cond_.wakeAll();
}

void FileModifier::RequestPause() {
    pause_requested_.store(true);
}

void FileModifier::RequestResume() {
    pause_requested_.store(false);
    QMutexLocker locker(&wait_mutex_);
    wait_cond_.wakeAll();
}
