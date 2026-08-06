#ifndef FILESYSTEMUTILS_H
#define FILESYSTEMUTILS_H

#include <QFile>
#include <QStringList>

/**
 * @brief Пространство имён с утилитами для работы с файловой системой.
 *
 * @details Содержит вспомогательные функции для фильтрации файлов, создания директорий,
 *          формирования имён, открытия файлов и подсчёта общего размера.
 */
namespace FileSystemUtils {

    /**
     * @brief Получает список имён файлов в директории, соответствующих маске.
     *
     * @param input_path Путь к директории для поиска.
     * @param mask Маска фильтрации. Может содержать несколько шаблонов, разделённых запятыми.
     *             Поддерживаются символы '*' и '?', а также имена файлов или расширения.
     *             Если маска пуста, возвращаются все файлы.
     * @param result_filenames [out] Список имён файлов (без пути), удовлетворяющих маске.
     * @param error_msg [out] Сообщение об ошибке в случае неудачи.
     * @return true, если список успешно получен; false в случае ошибки (например, директория не существует).
     */
    bool GetSuitableFileNames(const QString &input_path, const QString &mask, QStringList &result_filenames, QString &error_msg);

    /**
     * @brief Создаёт директорию, если она не существует (включая все промежуточные).
     *
     * @param path Путь к создаваемой директории.
     * @param error_msg [out] Сообщение об ошибке в случае неудачи.
     * @return true, если директория существует или была успешно создана; false при ошибке.
     */
    bool EnsureDirectoryExists(const QString &path, QString &error_msg);

    /**
     * @brief Формирует имя выходного файла с учётом возможного переименования при конфликте.
     *
     * @param output_path Путь к выходной директории.
     * @param curr_filename Исходное имя файла (без пути).
     * @param modify_filename_if_exists Если true, то при совпадении имени с существующим файлом
     *                                   добавляется числовой суффикс (_1, _2, ...).
     * @param result_filename [out] Результирующее имя файла для записи.
     * @param error_msg [out] Сообщение об ошибке, если входные параметры некорректны.
     * @return true, если имя сформировано успешно; false при ошибке (пустые пути или имя).
     */
    bool GetOutputFilename(const QString &output_path, const QString &curr_filename, bool modify_filename_if_exists,
                QString &result_filename, QString &error_msg);

    /**
     * @brief Открывает файл с указанным режимом и при необходимости устанавливает смещение.
     *
     * @param file Ссылка на объект QFile, который будет открыт.
     * @param mode Режим открытия (QIODevice::OpenMode).
     * @param filepath Путь к файлу (используется только для сообщений об ошибках).
     * @param offset Смещение в байтах, на которое будет выполнен seek после открытия.
     *               Если offset > 0, позиция устанавливается в это значение.
     * @param error_msg [out] Сообщение об ошибке при неудаче.
     * @return true, если файл открыт и (если нужно) смещение установлено успешно.
     */
    bool OpenFile(QFile &file, QIODeviceBase::OpenMode mode, const QString &filepath, qint64 offset, QString &error_msg);

    /**
     * @brief Вычисляет суммарный размер всех файлов из списка.
     *
     * @param input_path Путь к директории, где находятся файлы.
     * @param filenames Список имён файлов (без пути).
     * @param result_total_bytes [out] Суммарный размер в байтах.
     * @param error_msg [out] Сообщение об ошибке, если какой-либо файл не существует.
     * @return true, если все файлы существуют и размер успешно подсчитан; false при ошибке.
     */
    bool CalculateTotalBytes(const QString &input_path, const QStringList &filenames, qint64 &total_bytes, QString &error_msg);
} // namespace FileSystemUtils

#endif // FILESYSTEMUTILS_H
