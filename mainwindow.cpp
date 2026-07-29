#include "mainwindow.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
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

    main_layout->addWidget(mask_edit_);
    main_layout->addWidget(del_ifile_chck_);
    main_layout->addLayout(out_layout);
    main_layout->addLayout(in_layout);
    main_layout->addWidget(overwriting_btn);
    main_layout->addWidget(modification_btn);
    main_layout->addWidget(run_mode_cmb_);
    main_layout->addWidget(period_edit_);
    main_layout->addWidget(hex_edit_);

    QPushButton *run_btn = new QPushButton("Начать");
    main_layout->addWidget(run_btn);
    setCentralWidget(central);
}

MainWindow::~MainWindow() {}
