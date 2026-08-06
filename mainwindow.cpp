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
#include <QLabel>
#include <QFileDialog>

#include "filemodifier.h"
#include "progressdialog.h"

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

void MainWindow::CreateUI() {
    QWidget *central = new QWidget(this);
    QGridLayout *main_layout = new QGridLayout(central);

    CreateMaskSection(main_layout);
    CreateDeleteSection(main_layout);
    CreatePathSection(main_layout, 2, "Путь выходных файлов", out_path_edit_, "Выберите папку для выходных файлов");
    CreatePathSection(main_layout, 3, "Путь входных файлов", in_path_edit_, "Выберите папку для входных файлов");
    CreateOverwriteSection(main_layout);
    CreateRunModeSection(main_layout);
    CreatePeriodSection(main_layout);
    CreateKeySection(main_layout);
    CreateRunButton(main_layout);

    // Устанавливаем начальную видимость
    slotRunModeChanged(run_mode_cmb_->currentIndex());

    setCentralWidget(central);
}

void MainWindow::CreateMaskSection(QGridLayout *layout) {
    QLabel *mask_label = new QLabel("Маска входных файлов");
    mask_edit_ = new QLineEdit();
    layout->addWidget(mask_label, 0, 0);
    layout->addWidget(mask_edit_, 0, 1);
}

void MainWindow::CreateDeleteSection(QGridLayout *layout) {
    QLabel *del_label = new QLabel("Удалять входные файлы");
    del_ifile_chck_ = new QCheckBox();
    layout->addWidget(del_label, 1, 0);
    layout->addWidget(del_ifile_chck_, 1, 1);
}

void MainWindow::CreatePathSection(QGridLayout *layout, int row, const QString &label_text,
    QLineEdit *&line_edit, const QString &dialog_title) {

    QLabel *label = new QLabel(label_text);
    QHBoxLayout *h_layout = new QHBoxLayout();
    line_edit = new QLineEdit();
    QPushButton *button = new QPushButton("Обзор...");
    h_layout->addWidget(line_edit);
    h_layout->addWidget(button);

    layout->addWidget(label, row, 0);
    layout->addLayout(h_layout, row, 1);

    connect(button, &QPushButton::clicked, [this, line_edit, dialog_title]() {
        ShowFolderSelector(line_edit, dialog_title);
    });
}

void MainWindow::CreateOverwriteSection(QGridLayout *layout) {
    QLabel *radiobutton_label = new QLabel("При повторении выходных файлов");
    QRadioButton *overwriting_btn = new QRadioButton("Перезапись");
    QRadioButton *modification_btn = new QRadioButton("Модификация");
    overwriting_btn->setChecked(true);

    QHBoxLayout *radiobutton_layout = new QHBoxLayout();
    radiobutton_layout->addWidget(overwriting_btn);
    radiobutton_layout->addWidget(modification_btn);

    out_file_name_gr_ = new QButtonGroup(this);
    out_file_name_gr_->addButton(overwriting_btn, 0);   // перезапись
    out_file_name_gr_->addButton(modification_btn, 1);  // модификация

    layout->addWidget(radiobutton_label, 4, 0);
    layout->addLayout(radiobutton_layout, 4, 1);
}

void MainWindow::CreateRunModeSection(QGridLayout *layout) {
    QLabel *run_mode_label = new QLabel("Частота работы");
    run_mode_cmb_ = new QComboBox();
    run_mode_cmb_->addItem("Разовый запуск");
    run_mode_cmb_->addItem("Работа по таймеру");
    layout->addWidget(run_mode_label, 5, 0);
    layout->addWidget(run_mode_cmb_, 5, 1);

    // Подключаем сигнал изменения режима
    connect(run_mode_cmb_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::slotRunModeChanged);
}

void MainWindow::CreatePeriodSection(QGridLayout *layout) {
    period_label_ = new QLabel("Периодичность опроса");
    period_edit_ = new QTimeEdit();
    period_edit_->setDisplayFormat("HH:mm:ss");
    period_edit_->setTime(QTime(0, 5, 0));

    layout->addWidget(period_label_, 6, 0);
    layout->addWidget(period_edit_, 6, 1);

    period_label_->setVisible(false);
    period_edit_->setVisible(false);
}

void MainWindow::CreateKeySection(QGridLayout *layout) {
    QLabel *hex_label = new QLabel("Ключ в формате hex");
    hex_edit_ = new QLineEdit();
    hex_edit_->setInputMask("HHHHHHHHHHHHHHHH");
    layout->addWidget(hex_label, 7, 0);
    layout->addWidget(hex_edit_, 7, 1);
}

void MainWindow::CreateRunButton(QGridLayout *layout) {
    run_btn_ = new QPushButton("Начать");
    layout->addWidget(run_btn_, 8, 0, 1, 2);

    connect(run_btn_, &QPushButton::clicked, this, &MainWindow::slotStart);
}

void MainWindow::ShowFolderSelector(QLineEdit *lineEdit, const QString &title) {
    QString curr_dir = lineEdit->text().isEmpty() ? QDir::homePath() : lineEdit->text();
    QString dir = QFileDialog::getExistingDirectory(this, title, curr_dir,
                                                    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!dir.isEmpty()) {
        lineEdit->setText(dir);
    }
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
    bool is_timer = (idx == 1);
    period_label_->setVisible(is_timer);
    period_edit_->setVisible(is_timer);
}
