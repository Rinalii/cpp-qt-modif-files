#include "filemodifier.h"

#include <QFile>
#include <QDebug>
#include <QDir>

#include "filesystemutils.h"

FileModifier::FileModifier(QObject *parent)
    : QObject(parent) {
}

bool FileModifier::ModifyFile(const QString &input_filepath, const QString &output_filepath) {
    QFile in_file(input_filepath);
    QFile out_file(output_filepath);
    qint64 offset = 0;

    QString error_msg;
    if (!FileSystemUtils::OpenFile(in_file, QIODevice::ReadOnly, input_filepath, offset, error_msg)) {
        emit signalFinished(false, error_msg);
        return false;
    }

    if (!FileSystemUtils::OpenFile(out_file, QIODevice::WriteOnly | QIODevice::Truncate, output_filepath, offset, error_msg)) {
        in_file.close();
        emit signalFinished(false, error_msg);
        return false;
    }

    const qint64 buffer_size = 1024 * 1024;
    const int key_size = settings_.key_.size();
    QByteArray chunk;

    while (!in_file.atEnd()) {
        if (exit_requested_.load()) {       // ВЫХОД
            out_file.close();
            QFile::remove(output_filepath);
            in_file.close();
            emit signalFinished(false, "Operation cancelled by user");
            return false;
        }

        if (pause_requested_.load()) {      // ПАУЗА
            offset = in_file.pos();

            in_file.close();
            out_file.close();
            is_paused_.store(true);

            WaitForResume();    // Ожидаем возобновления
            if (exit_requested_.load()) {
                QFile::remove(output_filepath);
                emit signalFinished(false, "Operation cancelled by user");
                return false;
            }
            // После возобновления открываем файлы заново с той же позиции
            if (!FileSystemUtils::OpenFile(in_file, QIODevice::ReadOnly, input_filepath, offset, error_msg)) {
                emit signalFinished(false, error_msg);
                return false;
            }

            if (!FileSystemUtils::OpenFile(out_file, QIODevice::WriteOnly, output_filepath, offset, error_msg)) {
                in_file.close();
                emit signalFinished(false, error_msg);
                return false;
            }
        }

        chunk = in_file.read(buffer_size);
        for (int i = 0; i < chunk.size(); ++i) {
            chunk[i] ^= settings_.key_[i % key_size];
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

void FileModifier::WaitForResume()
{
    QMutexLocker locker(&wait_mutex_);
    while (pause_requested_.load() && !exit_requested_.load()) {
        wait_cond_.wait(&wait_mutex_);  //отпускаем мьютекс и блокируем текущий поток до wake
    }
    is_paused_.store(false);
}

void FileModifier::ModifyFiles(const QString &input_path, const QString &output_path,
                               bool modify_filename, const QString &mask,
                               const QByteArray &key, bool remove_source) {
    if (!IsKeyValid(key)) return;

    QString error_msg;

    settings_.input_path_ = input_path;
    settings_.output_path_ = output_path;
    settings_.modify_filename_ = modify_filename;
    settings_.remove_source_ = remove_source;
    settings_.key_ = key;
    if(!FileSystemUtils::GetSuitableFileNames(input_path, mask, settings_.filenames_, error_msg)) {
        emit signalFinished(false, error_msg);
        return;
    }

    exit_requested_.store(false);
    pause_requested_.store(false);
    is_paused_.store(false);

    bytes_processed_ = 0;
    if(!FileSystemUtils::CalculateTotalBytes(settings_.input_path_, settings_.filenames_, total_bytes_, error_msg)) {
        emit signalFinished(false, error_msg);
        return;
    }

    if (!FileSystemUtils::EnsureDirectoryExists(output_path, error_msg)) {
        emit signalFinished(false, error_msg);
        return;
    }

    for (int i = 0; i < settings_.filenames_.size(); ++i) {
        if (exit_requested_.load()) {
            emit signalFinished(false, "Operation cancelled by user");
            return;
        }

        // Проверка паузы перед началом нового файла
        if (pause_requested_.load()) {
            is_paused_.store(true);
            // Ожидаем снятия паузы
            WaitForResume();
            if (exit_requested_.load()) {
                emit signalFinished(false, "Operation cancelled by user");
                return;
            }
        }

        QString filename = settings_.filenames_[i];
        QString input_file = settings_.input_path_ + "/" + filename;

        QString result_filename;
        if(!FileSystemUtils::GetOutputFilename(settings_.output_path_, filename, settings_.modify_filename_, result_filename, error_msg)) {
            emit signalFinished(false, error_msg);
            return;
        }
        QString output_file = settings_.output_path_ + "/" + result_filename;

        emit signalStartFileModification(filename);
        if (!ModifyFile(input_file, output_file)) {
            return;
        }

        // Удаление исходного файла, если требуется
        if (settings_.remove_source_ && input_file != output_file) {
            QFile::remove(input_file);
        }
    }

    emit signalFinished(true, QString());
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
    wait_cond_.wakeOne(); // Будим поток
}
