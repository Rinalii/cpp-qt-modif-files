#include "mainwindow.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QThread>
#include <QVBoxLayout>

#include "filemodifier.h"

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

    period_edit_ = new QLineEdit();

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
    : QMainWindow(parent) {
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

    connect(run_btn_, &QPushButton::clicked, this, &MainWindow::slotStart);
}

MainWindow::~MainWindow() {
    // Просим воркер остановиться (если он ещё работает)
    worker_->slotExitRequested();

    // Завершаем поток и ждём его остановки
    worker_thread_->quit();
    worker_thread_->wait();

    delete worker_;
}

void MainWindow::slotStart() {
    QString input = in_path_edit_->text();
    QString output = out_path_edit_->text();
    QString mask = mask_edit_->text();
    QByteArray key = QByteArray::fromHex(hex_edit_->text().toUtf8());
    bool remove = del_ifile_chck_->isChecked();

    // Испускаем сигнал для обработки в потоке воркера
    emit signalStartProcessing(input, output, mask, key, remove);
}
