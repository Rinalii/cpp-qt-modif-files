#include "mainwindow.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QThread>
#include <QTimer>
#include <QVBoxLayout>
#include <QTimeEdit>

#include "filemodifier.h"
#include "progressdialog.h"

void MainWindow::CreateUI() {
    QWidget *central = new QWidget(this);
    QVBoxLayout *main_layout = new QVBoxLayout(central);

    mask_edit_ = new QLineEdit();
    del_ifile_chck_ = new QCheckBox();

    QHBoxLayout *out_layout = new QHBoxLayout();
    out_path_edit_ = new QLineEdit();
    QPushButton *out_path_btn = new QPushButton("Обзор...");

    out_layout->addWidget(out_path_edit_);
    out_layout->addWidget(out_path_btn);

    QHBoxLayout *in_layout = new QHBoxLayout();
    in_path_edit_ = new QLineEdit();
    QPushButton *in_path_btn = new QPushButton("Обзор...");

    in_layout->addWidget(in_path_edit_);
    in_layout->addWidget(in_path_btn);

    QRadioButton *overwriting_btn = new QRadioButton("Перезапись");
    QRadioButton *modification_btn = new QRadioButton("Модификация");

    out_file_name_gr_ = new QButtonGroup(this);
    out_file_name_gr_->addButton(overwriting_btn);
    out_file_name_gr_->addButton(modification_btn);

    run_mode_cmb_ = new QComboBox();

    run_mode_cmb_->addItem("Разовый запуск");
    run_mode_cmb_->addItem("Работа по таймеру");

    period_edit_ = new QTimeEdit();
    period_edit_->setDisplayFormat("HH:mm:ss");
    period_edit_->setTime(QTime(0, 5, 0));

    // Устанавливаем начальную видимость
    slotRunModeChanged(run_mode_cmb_->currentIndex());

    // Подключаем сигнал изменения режима
    connect(run_mode_cmb_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::slotRunModeChanged);

    hex_edit_ = new QLineEdit();
    hex_edit_->setInputMask("HHHHHHHHHHHHHHHH");

    main_layout->addWidget(mask_edit_);
    main_layout->addWidget(del_ifile_chck_);
    main_layout->addLayout(out_layout);
    main_layout->addLayout(in_layout);
    main_layout->addWidget(overwriting_btn);
    main_layout->addWidget(modification_btn);
    main_layout->addWidget(run_mode_cmb_);
    main_layout->addWidget(period_edit_);
    main_layout->addWidget(hex_edit_);

    run_btn_ = new QPushButton("Начать");
    main_layout->addWidget(run_btn_);
    setCentralWidget(central);
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , is_timer_mode_(false)
    , progress_dialog_(nullptr) {
    CreateUI();

    // Создаём воркер без родителя, чтобы безопасно переместить в поток
    worker_ = new FileModifier(nullptr);
    // Создаём поток
    worker_thread_ = new QThread(this);
    // Перемещаем воркер в поток
    worker_->moveToThread(worker_thread_);

    // Соединяем сигнал запуска из GUI со слотом воркера
    connect(this, &MainWindow::signalStartProcessing, worker_, &FileModifier::ModifyFiles);

    // Запускаем поток
    worker_thread_->start();

    timer_ = new QTimer(this);
    connect(timer_, &QTimer::timeout, this, &MainWindow::slotStart);

    connect(run_btn_, &QPushButton::clicked, this, &MainWindow::slotStart);
}

MainWindow::~MainWindow() {
    if (progress_dialog_) {
        progress_dialog_->close();
        progress_dialog_->deleteLater();
    }

    // Просим воркер остановиться (если он ещё работает)
    worker_->RequestExit();

    // Завершаем поток и ждём его остановки
    worker_thread_->quit();
    worker_thread_->wait();

    delete worker_;
}

void MainWindow::slotStart() {
    if (progress_dialog_) {
        return;
    }

    QString input = in_path_edit_->text();
    QString output = out_path_edit_->text();
    QString mask = mask_edit_->text();
    QByteArray key = QByteArray::fromHex(hex_edit_->text().toUtf8());
    bool remove = del_ifile_chck_->isChecked();
    bool modify_filename = (out_file_name_gr_->checkedId() == 1);

    // Проверка обязательных полей
    if (input.isEmpty() || output.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Укажите входную и выходную папки");
        return;
    }

    if (key.isEmpty() || key.size() != 8) {
        QMessageBox::warning(this, "Ошибка", "Ключ должен быть 16 шестнадцатеричных символов");
        return;
    }

    // Проверка режима таймера
    is_timer_mode_ = (run_mode_cmb_->currentIndex() == 1);
    if (is_timer_mode_) {
        QTime time = period_edit_->time();
        int period_sec = time.hour() * 3600 + time.minute() * 60 + time.second();
        if (period_sec <= 0) {
            QMessageBox::warning(this, "Ошибка", "Некорректный период таймера");
            return;
        }
        timer_->start(period_sec * 1000);
    } else {
        timer_->stop();
    }

    // Блокируем кнопку запуска
    run_btn_->setEnabled(false);

    // Создаём диалог прогресса
    progress_dialog_ = new ProgressDialog(this);

    // Подключаем сигналы диалога
    connect(progress_dialog_, &ProgressDialog::signalStopRequested, this, &MainWindow::slotExitRequested);
    connect(progress_dialog_, &ProgressDialog::signalPauseRequested, this, &MainWindow::slotPauseRequested);
    connect(progress_dialog_, &ProgressDialog::signalResumeRequested, this, &MainWindow::slotResumeRequested);

    // Подключаем сигналы из воркера в GUI
    connect(worker_, &FileModifier::signalProgress, progress_dialog_, &ProgressDialog::slotUpdateProgress);
    connect(worker_, &FileModifier::signalStartFileModification, progress_dialog_, &ProgressDialog::slotUpdateCurrentFile);
    connect(worker_, &FileModifier::signalFinished, progress_dialog_, &ProgressDialog::slotFinished);

    // Подключаем сигнал завершения диалога
    connect(progress_dialog_, &QDialog::finished, this, &MainWindow::slotDialogFinished);

    // Запускаем обработку
    emit signalStartProcessing(input, output, modify_filename, mask, key, remove);

    // Показываем диалог (блокирует главное окно)
    progress_dialog_->show();
}

void MainWindow::slotDialogFinished() {
    if (progress_dialog_) {
        disconnect(worker_, nullptr, progress_dialog_, nullptr);
        progress_dialog_->deleteLater();
        progress_dialog_ = nullptr;
    }

    // Разблокируем кнопку запуска
    run_btn_->setEnabled(true);
}

void MainWindow::slotExitRequested() {
    worker_->RequestExit();
}

void MainWindow::slotPauseRequested() {
    worker_->RequestPause();
}

void MainWindow::slotResumeRequested() {
    worker_->RequestResume();
}

void MainWindow::slotRunModeChanged(int idx) {
    period_edit_->setVisible(idx == 1);
}
