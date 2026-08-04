#include "progressdialog.h"

#include <QMessageBox>
#include <QCloseEvent>

#include <QProgressBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QTimer>

ProgressDialog::ProgressDialog(QWidget *parent)
    : QDialog(parent)
    , is_paused_(false)
    , is_finished_(false) {
    setWindowTitle("Обработка файлов");
    setModal(true);  // Блокируем главное окно
    setMinimumWidth(450);

    progress_bar_ = new QProgressBar(this);
    progress_bar_->setRange(0, 100);
    progress_bar_->setValue(0);
    progress_bar_->setTextVisible(true);
    progress_bar_->setFormat("%p%");

    file_label_ = new QLabel("Ожидание начала...", this);
    status_label_ = new QLabel("Готов к запуску", this);

    pause_resume_btn_ = new QPushButton("Пауза", this);
    stop_btn_ = new QPushButton("Остановить", this);

    QVBoxLayout *main_layout = new QVBoxLayout(this);
    main_layout->addWidget(progress_bar_);
    main_layout->addWidget(file_label_);
    main_layout->addWidget(status_label_);

    QHBoxLayout *button_layout = new QHBoxLayout();
    button_layout->addWidget(pause_resume_btn_);
    button_layout->addWidget(stop_btn_);
    main_layout->addLayout(button_layout);

    connect(pause_resume_btn_, &QPushButton::clicked, this, &ProgressDialog::slotPauseResumeClicked);
    connect(stop_btn_, &QPushButton::clicked, this, &ProgressDialog::signalStopRequested);
}

void ProgressDialog::slotUpdateProgress(int percent) {
    progress_bar_->setValue(percent);
}

void ProgressDialog::slotUpdateCurrentFile(const QString &filename) {
    file_label_->setText("Файл: " + filename);
}

void ProgressDialog::slotFinished(bool success, const QString &errorMessage) {
    is_finished_ = true;
    pause_resume_btn_->setEnabled(false);
    stop_btn_->setEnabled(false);

    if (success) {
        status_label_->setText("Обработка успешно завершена!");
        progress_bar_->setValue(100);
    } else {
        status_label_->setText("Ошибка: " + errorMessage);
    }

    // Закрываем через 3 сек.
    if (success) {
        QTimer::singleShot(3000, this, &QDialog::accept);
    }
}

void ProgressDialog::slotPauseResumeClicked() {
    if (!is_paused_) {
        is_paused_ = true;
        pause_resume_btn_->setText("Продолжить");
        status_label_->setText("Приостановлено");
        emit signalPauseRequested();
    } else {
        is_paused_ = false;
        pause_resume_btn_->setText("Пауза");
        status_label_->setText("Возобновление...");
        emit signalResumeRequested();
    }
}

void ProgressDialog::closeEvent(QCloseEvent *event) {
    if (!is_finished_) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Подтверждение",
                                      "Остановить обработку?",
                                      QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            emit signalStopRequested();
            event->accept();
        } else {
            event->ignore();
        }
    } else {
        event->accept();
    }
}
