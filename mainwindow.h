#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QLineEdit;
class QCheckBox;
class QButtonGroup;
class QComboBox;
class QTimeEdit;
class QPushButton;
class QThread;
class QTimer;
class QLabel;
class QGridLayout;

class FileModifier;
class ProgressDialog;

/**
 * @brief Главное окно приложения.
 *
 * @details Управляет пользовательским интерфейсом, настройками и запуском процесса
 *          модификации файлов. Поддерживает два режима работы: разовый запуск и
 *          периодический (по таймеру). Для длительных операций используется отдельный
 *          поток и диалог прогресса.
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:

    /**
     * @brief Конструктор.
     * @param parent Родительский виджет.
     */
    MainWindow(QWidget *parent = nullptr);

    /**
     * @brief Деструктор.
     * @details Останавливает поток, завершает обработку и освобождает ресурсы.
     */
    ~MainWindow();

public slots:

    /**
     * @brief Запускает процесс модификации файлов.
     * @details Считывает параметры из полей ввода, проверяет их корректность,
     *          настраивает таймер (при необходимости), создаёт диалог прогресса
     *          и запускает воркер в отдельном потоке.
     */
    void slotStart();

    /**
     * @brief Обрабатывает закрытие диалога прогресса.
     * @details Отключает сигналы, удаляет диалог и разблокирует кнопку запуска.
     */
    void slotDialogFinished();

    /**
     * @brief Запрашивает остановку обработки (по запросу пользователя).
     */
    void slotExitRequested();

    /**
     * @brief Запрашивает приостановку обработки.
     */
    void slotPauseRequested();

    /**
     * @brief Запрашивает возобновление обработки.
     */
    void slotResumeRequested();

    /**
     * @brief Обрабатывает изменение режима работы (разовый/по таймеру).
     *          Показывает или скрывает элементы управления периодом.
     * @param idx Индекс выбранного режима (0 – разовый, 1 – таймер).
     */
    void slotRunModeChanged(int idx);

signals:

    /**
     * @brief Сигнал для запуска обработки в воркере.
     * @param input_path Путь к входной директории.
     * @param output_path Путь к выходной директории.
     * @param modify_filename Флаг переименования при конфликте имён.
     * @param mask Маска файлов.
     * @param key Ключ шифрования (8 байт).
     * @param remove_source Флаг удаления исходных файлов.
     */
    void signalStartProcessing(const QString &input_path, const QString &output_path, bool modify_filename, const QString &mask,
        const QByteArray &key, bool remove_source);

private:

    /**
     * @brief Создаёт секцию ввода маски файлов.
     * @param layout Указатель на сеточный компоновщик.
     */
    void CreateMaskSection(QGridLayout *layout);

    /**
     * @brief Создаёт секцию с флагом удаления исходных файлов.
     * @param layout Указатель на сеточный компоновщик.
     */
    void CreateDeleteSection(QGridLayout *layout);

    /**
     * @brief Создаёт секцию выбора пути (входного или выходного) с кнопкой "Обзор".
     * @param layout Указатель на сеточный компоновщик.
     * @param row Номер строки в сетке.
     * @param label_text Текст подписи.
     * @param line_edit Ссылка на указатель QLineEdit, который будет создан.
     * @param dialog_title Заголовок диалога выбора папки.
     */
    void CreatePathSection(QGridLayout *layout, int row, const QString &label_text, QLineEdit *&line_edit, const QString &dialog_title);

    /**
     * @brief Создаёт секцию выбора стратегии при конфликте имён выходных файлов.
     * @param layout Указатель на сеточный компоновщик.
     */
    void CreateOverwriteSection(QGridLayout *layout);

    /**
     * @brief Создаёт секцию выбора режима работы (разовый/по таймеру).
     * @param layout Указатель на сеточный компоновщик.
     */
    void CreateRunModeSection(QGridLayout *layout);

    /**
     * @brief Создаёт секцию настройки периода таймера.
     * @param layout Указатель на сеточный компоновщик.
     */
    void CreatePeriodSection(QGridLayout *layout);

    /**
     * @brief Создаёт секцию ввода ключа (в шестнадцатеричном формате).
     * @param layout Указатель на сеточный компоновщик.
     */
    void CreateKeySection(QGridLayout *layout);

    /**
     * @brief Создаёт кнопку запуска.
     * @param layout Указатель на сеточный компоновщик.
     */
    void CreateRunButton(QGridLayout *layout);

    /**
     * @brief Открывает диалог выбора папки и устанавливает выбранный путь в поле.
     * @param lineEdit Указатель на QLineEdit для установки пути.
     * @param title Заголовок диалога.
     */
    void ShowFolderSelector(QLineEdit *lineEdit, const QString &title);

    /**
     * @brief Создаёт весь пользовательский интерфейс.
     */
    void CreateUI();

private:
    // Виджеты UI
    QLineEdit   *mask_edit_;            ///< Поле ввода маски файлов
    QCheckBox   *del_ifile_chck_;       ///< Чекбокс удаления исходных файлов
    QLineEdit   *out_path_edit_;        ///< Поле пути выходной директории
    QLineEdit   *in_path_edit_;         ///< Поле пути входной директории
    QButtonGroup *out_file_name_gr_;    ///< Группа радиокнопок для стратегии именования
    QComboBox   *run_mode_cmb_;         ///< Выбор режима работы
    QLabel      *period_label_;         ///< Подпись для периода
    QTimeEdit   *period_edit_;          ///< Редактор времени периода
    QLineEdit   *hex_edit_;             ///< Поле ввода ключа в hex

    QPushButton *run_btn_;              ///< Кнопка запуска

    bool is_timer_mode_;                ///< Флаг, включён ли таймерный режим
    QTimer *timer_;                     ///< Таймер для периодического запуска

    FileModifier *worker_;              ///< Объект-воркер для обработки файлов
    QThread *worker_thread_;            ///< Поток, в котором работает воркер
    ProgressDialog *progress_dialog_;   ///< Диалог отображения прогресса
};
#endif // MAINWINDOW_H
