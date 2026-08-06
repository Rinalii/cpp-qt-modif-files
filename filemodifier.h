#ifndef FILEMODIFIER_H
#define FILEMODIFIER_H

#include <QString>
#include <QObject>
#include <atomic>
#include <QMutex>
#include <QWaitCondition>
#include <QFile>

/**
 * @brief Класс для модификации файлов с поддержкой паузы, отмены и прогресса.
 *
 * @details Выполняет XOR-шифрование файлов с заданным 8-байтовым ключом.
 */
class FileModifier : public QObject
{
    Q_OBJECT
public:

    /**
     * @brief Конструктор.
     * @param parent Родительский объект Qt.
     */
    explicit FileModifier(QObject *parent = nullptr);

    /**
     * @brief Запускает процесс модификации файлов.
     * @param input_path Путь к директории с исходными файлами.
     * @param output_path Путь к директории для сохранения обработанных файлов.
     * @param modify_filename Флаг: если true, имена выходных файлов будут изменены путём добавления суффикса "_number".
     * @param mask Маска для фильтрации файлов (например, "*.txt").
     * @param key Ключ шифрования (должен быть ровно 8 байт).
     * @param remove_source Если true, исходные файлы удаляются после успешной обработки.
     * @note Функция блокирует поток до завершения или ошибки.
     *       Прогресс и результаты отправляются через сигналы.
     */
    void ModifyFiles(const QString &input_path, const QString &output_path, bool modify_filename,
                const QString& mask, const QByteArray &key, bool remove_source);

    /**
     * @brief Запрашивает принудительный выход из операции.
     * @details Устанавливает флаг exit_requested_, пробуждает ожидающий поток.
     *          Обработка будет прервана при первой возможности.
     */
    void RequestExit();

    /**
     * @brief Запрашивает приостановку операции.
     * @details Устанавливает флаг pause_requested_. Обработка приостановится
     *          на границе чанка (после завершения текущего чтения/записи).
     */
    void RequestPause();

    /**
     * @brief Запрашивает возобновление операции после паузы.
     * @details Сбрасывает флаг pause_requested_ и пробуждает поток.
     */
    void RequestResume();

signals:

    /**
     * @brief Сигнал о прогрессе обработки.
     * @param percent Процент завершения (от 0 до 100) по всем файлам.
     */
    void signalProgress(int percent);

    /**
     * @brief Сигнал о завершении операции.
     * @param success true, если все файлы обработаны успешно.
     * @param error_message Текст ошибки (пустая строка при успехе).
     */
    void signalFinished(bool success, const QString &error_message);

    /**
     * @brief Сигнал о начале обработки конкретного файла.
     * @param filename Имя обрабатываемого файла (без пути).
     */
    void signalStartFileModification(const QString &filename);

private:

    /**
     * @brief Структура для хранения настроек обработки.
     */
    struct Settings {
        QString input_path_;            ///< Входная директория
        QString output_path_;           ///< Выходная директория
        QStringList filenames_;         ///< Список файлов для обработки
        bool modify_filename_ = false;  ///< Флаг модификации имён
        QByteArray key_;                ///< Ключ шифрования
        bool remove_source_ = false;    ///< Флаг удаления исходников

        /**
         * @brief Очищает все поля структуры.
         */
        void Clear() {
            input_path_.clear();
            output_path_.clear();
            filenames_.clear();
            modify_filename_ = false;
            key_.clear();
            remove_source_ = false;
        }
    };

    /**
     * @brief Проверяет корректность ключа (не пустой и ровно 8 байт).
     * @param key Проверяемый ключ.
     * @param error_msg Строка для сообщения об ошибке.
     * @return true, если ключ валиден.
     */
    static bool IsKeyValid(const QByteArray &key, QString &error_msg);

    /**
     * @brief Открывает входной и выходной файлы с возможностью дозаписи.
     * @param in_file Ссылка на объект QFile для входного файла.
     * @param out_file Ссылка на объект QFile для выходного файла.
     * @param input_filepath Путь к входному файлу.
     * @param output_filepath Путь к выходному файлу.
     * @param offset Смещение в байтах, с которого начинать чтение/запись.
     * @param error_msg Строка для сообщения об ошибке.
     * @return true, если оба файла открыты успешно.
     */
    static bool OpenFiles(QFile &in_file, QFile &out_file, const QString &input_filepath,
                const QString &output_filepath, qint64 offset, QString &error_msg);

    /**
     * @brief Подготавливает обработку: проверяет пути, собирает список файлов,
     *        вычисляет общий размер, создаёт выходную директорию.
     * @param input_path Входная директория.
     * @param output_path Выходная директория.
     * @param modify_filename Флаг модификации имён.
     * @param mask Маска файлов.
     * @param key Ключ.
     * @param remove_source Флаг удаления исходников.
     * @param error_msg Строка для сообщения об ошибке.
     * @return true, если подготовка прошла успешно.
     */
    bool PrepareProcessing(const QString &input_path, const QString &output_path,
                bool modify_filename, const QString &mask, const QByteArray &key, bool remove_source, QString &error_msg);

    /**
     * @brief Обрабатывает один файл: определяет имя выходного файла,
     *        вызывает ModifyFile и при необходимости удаляет источник.
     * @param filename Имя файла (без пути).
     * @param error_msg Строка для сообщения об ошибке.
     * @return true, если файл обработан успешно.
     */
    bool ProcessOneFile(const QString &filename, QString &error_msg);

    /**
     * @brief Выполняет непосредственное XOR-преобразование одного файла.
     * @param input_filepath Путь к входному файлу.
     * @param output_filepath Путь к выходному файлу.
     * @param error_msg Строка для сообщения об ошибке.
     * @return true, если преобразование выполнено без ошибок.
     */
    bool ModifyFile(const QString &input_filepath, const QString &output_filepath, QString &error_msg);

    /**
     * @brief Ожидает снятия паузы (блокирует текущий поток).
     * @details Использует QWaitCondition и мьютекс. Поток блокируется,
     *          пока pause_requested_ == true и exit_requested_ == false.
     */
    void WaitForResume();

    /**
     * @brief Обрабатывает данные файла: читает блоками, применяет XOR, записывает.
     * @param in_file Ссылка на открытый входной файл.
     * @param out_file Ссылка на открытый выходной файл.
     * @param error_msg Строка для сообщения об ошибке.
     * @return true, если все данные обработаны успешно.
     */
    bool ProcessFileData(QFile &in_file, QFile &out_file, QString &error_msg);

    /**
     * @brief Обрабатывает запрос паузы: закрывает файлы, ждёт возобновления,
     *        затем переоткрывает файлы с сохранённой позиции.
     * @param in_file Ссылка на входной файл.
     * @param out_file Ссылка на выходной файл.
     * @param input_filepath Путь к входному файлу.
     * @param output_filepath Путь к выходному файлу.
     * @param offset Текущее смещение, будет сохранено и восстановлено.
     * @param error_msg Строка для сообщения об ошибке.
     * @return true, если пауза обработана и работа продолжена.
     */
    bool HandlePause(QFile &in_file, QFile &out_file, const QString &input_filepath, const QString &output_filepath,
                qint64 &offset, QString &error_msg);

private:
    // Настройки для обработки файлов
    Settings settings_;                  ///< Настройки текущей операции

    qint64 bytes_processed_;             ///< Количество уже обработанных байт
    qint64 total_bytes_;                 ///< Общий размер всех обрабатываемых файлов

    // Выход / пауза
    std::atomic<bool> exit_requested_{false};   ///< Флаг запроса отмены
    std::atomic<bool> pause_requested_{false};  ///< Флаг запроса паузы
    std::atomic<bool> is_paused_{false};        ///< Флаг, указывающий, что поток находится в состоянии паузы

    // Для приостановки потока, чтобы он не нагружал систему
    QMutex wait_mutex_;                  ///< Мьютекс для синхронизации с QWaitCondition
    QWaitCondition wait_cond_;           ///< Условная переменная для блокировки/пробуждения
};

#endif // FILEMODIFIER_H
