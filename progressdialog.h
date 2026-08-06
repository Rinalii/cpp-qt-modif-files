#ifndef PROGRESSDIALOG_H
#define PROGRESSDIALOG_H

#include <QDialog>

class QProgressBar;
class QLabel;
class QPushButton;

/**
 * @brief Диалог отображения прогресса обработки файлов.
 *
 * @details Показывает текущий прогресс (0–100%), имя обрабатываемого файла и статус.
 *          Позволяет пользователю приостановить/возобновить или полностью остановить операцию.
 *          Диалог модальный, блокирует главное окно до завершения обработки.
 */
class ProgressDialog : public QDialog
{
    Q_OBJECT

public:

    /**
     * @brief Конструктор.
     * @param parent Родительский виджет.
     */
    explicit ProgressDialog(QWidget *parent = nullptr);

public slots:

    /**
     * @brief Обновляет индикатор прогресса.
     * @param percent Процент выполнения (0–100).
     */
    void slotUpdateProgress(int percent);

    /**
     * @brief Обновляет метку с именем текущего обрабатываемого файла.
     * @param filename Имя файла (без пути).
     */
    void slotUpdateCurrentFile(const QString &filename);

    /**
     * @brief Обрабатывает завершение операции.
     * @param success true, если операция выполнена успешно.
     * @param errorMessage Текст ошибки (если success == false).
     *
     * @details При успешном завершении автоматически закрывает диалог через 3 секунды.
     *          При ошибке показывает сообщение и не закрывается до нажатия пользователем.
     */
    void slotFinished(bool success, const QString &errorMessage = QString());

    /**
     * @brief Обрабатывает нажатие кнопки паузы/продолжения.
     * @details Переключает состояние паузы и отправляет соответствующий сигнал.
     */
    void slotPauseResumeClicked();

signals:

    /**
     * @brief Сигнал, отправляемый при запросе остановки (по кнопке или закрытию окна).
     */
    void signalStopRequested();

    /**
     * @brief Сигнал запроса приостановки обработки.
     */
    void signalPauseRequested();

    /**
     * @brief Сигнал запроса возобновления обработки.
     */
    void signalResumeRequested();

protected:

    /**
     * @brief Обрабатывает событие закрытия окна.
     * @param event Указатель на событие закрытия.
     * @details Если обработка ещё не завершена (is_finished_ == false),
     *          показывает диалог подтверждения остановки. При согласии
     *          отправляет signalStopRequested и закрывает диалог.
     *          Если обработка уже завершена, закрывается без подтверждения.
     * @note Метод переопределяет QDialog::closeEvent.
     */
    void closeEvent(QCloseEvent *event) override;

private:
    QProgressBar *progress_bar_;      ///< Индикатор прогресса
    QLabel *status_label_;            ///< Метка со статусом (готов, приостановлено, ошибка и т.д.)
    QLabel *file_label_;              ///< Метка с именем текущего файла
    QPushButton *pause_resume_btn_;   ///< Кнопка паузы/продолжения
    QPushButton *stop_btn_;           ///< Кнопка остановки

    bool is_paused_;                  ///< Флаг, находится ли операция на паузе
    bool is_finished_;                ///< Флаг, завершена ли операция
};

#endif // PROGRESSDIALOG_H
