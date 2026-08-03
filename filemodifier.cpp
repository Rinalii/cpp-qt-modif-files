#include "filemodifier.h"

#include <QFile>
#include <QDebug>
#include <QDir>

FileModifier::FileModifier(QObject *parent)
    : QObject(parent) {
}

bool FileModifier::ModifyFile(const QString &input_filepath, const QString &output_filepath)
{
    QFile in_file(input_filepath);
    QFile out_file(output_filepath);
    qint64 offset = 0;

    // Если есть сохранённое смещение для этого файла – используем его
    if (recovery_settings_.curr_input_file_ == input_filepath && recovery_settings_.curr_file_offset_ > 0) {
        offset = recovery_settings_.curr_file_offset_;
    }

    QString error_msg;
    if (!OpenFile(in_file, QIODevice::ReadOnly, input_filepath, offset, error_msg)) {
        emit signalFinished(false, error_msg);
        return false;
    }

    if (!OpenFile(out_file, QIODevice::ReadWrite, output_filepath, offset, error_msg)) {
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
            recovery_settings_.curr_file_offset_ = in_file.pos();
            recovery_settings_.curr_input_file_ = input_filepath;
            recovery_settings_.curr_output_file_ = output_filepath;

            in_file.close();
            out_file.close();
            is_paused_.store(true);

            WaitForResume();    // Ожидаем возобновления
            if (exit_requested_.load()) {
                emit signalFinished(false, "Operation cancelled by user");
                return false;
            }
            // После возобновления открываем файлы заново с той же позиции
            if (!OpenFile(in_file, QIODevice::ReadOnly, input_filepath, offset, error_msg)) {
                emit signalFinished(false, error_msg);
                return false;
            }

            if (!OpenFile(out_file, QIODevice::ReadWrite, output_filepath, offset, error_msg)) {
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

    recovery_settings_.Clear();

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

bool FileModifier::OpenFile(QFile &file, QIODeviceBase::OpenMode mode, const QString &filepath, qint64 offset, QString &error_msg) {
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

void FileModifier::ModifyFiles(const QString &input_path, const QString &output_path,
                               bool modify_filename, const QString &mask,
                               const QByteArray &key, bool remove_source) {
    if (!IsKeyValid(key)) return;

    settings_.input_path_ = input_path;
    settings_.output_path_ = output_path;
    settings_.modify_filename_ = modify_filename;
    settings_.remove_source_ = remove_source;
    settings_.key_ = key;
    settings_.filenames_ = GetSuitableFileNames(input_path, mask);

    exit_requested_.store(false);
    pause_requested_.store(false);
    is_paused_.store(false);

    bytes_processed_ = 0;
    if (!CalculateTotalBytes()) return;

    recovery_settings_.Clear();

    for (int i = recovery_settings_.curr_file_index_; i < settings_.filenames_.size(); ++i) {
        if (exit_requested_.load()) {
            emit signalFinished(false, "Operation cancelled by user");
            return;
        }

        // Проверка паузы перед началом нового файла
        if (pause_requested_.load()) {
            // Сохраняем индекс текущего файла
            recovery_settings_.curr_file_index_ = i;
            recovery_settings_.curr_input_file_ = settings_.input_path_ + "/" + settings_.filenames_[i];
            recovery_settings_.curr_output_file_ = settings_.output_path_ + "/" + GetOutputFilename(settings_.output_path_, settings_.filenames_[i], settings_.modify_filename_);
            recovery_settings_.curr_file_offset_ = 0; // начало файла
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
        QString output_file = settings_.output_path_ + "/" + GetOutputFilename(settings_.output_path_, filename, settings_.modify_filename_);

        if (!ModifyFile(input_file, output_file)) {
            return;
        }
    }

    emit signalFinished(true, QString());
}

void FileModifier::slotExitRequested() {
    exit_requested_.store(true);
}

void FileModifier::slotPauseRequested() {
    pause_requested_.store(true);
}

void FileModifier::slotResumeRequested() {
    pause_requested_.store(false);

    QMutexLocker locker(&wait_mutex_);
    wait_cond_.wakeOne(); // Будим поток
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
    total_bytes_ = 0;
    for (const QString &filename : settings_.filenames_) {
        QString input_file = settings_.input_path_ + "/" + filename;
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
