#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QLineEdit;
class QCheckBox;
class QButtonGroup;
class QComboBox;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    QLineEdit   *mask_edit_;
    QCheckBox   *del_ifile_chck_;
    QLineEdit   *out_path_edit_;
    QLineEdit   *in_path_edit_;
    QButtonGroup *out_file_name_gr_;
    QComboBox   *run_mode_cmb_;
    QLineEdit   *period_edit_;
    QLineEdit   *hex_edit_;
};
#endif // MAINWINDOW_H
