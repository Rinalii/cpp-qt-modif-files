#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QLineEdit;
class QCheckBox;
class QButtonGroup;
class QComboBox;
class QPushButton;
class QThread;

class FileModifier;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void slotStart();

signals:
    void signalStartProcessing(const QString &inputPath, const QString &outputPath, bool modify_filename, const QString &mask,
        const QByteArray &key, bool removeSource);

private:
    QLineEdit   *mask_edit_;
    QCheckBox   *del_ifile_chck_;
    QLineEdit   *out_path_edit_;
    QLineEdit   *in_path_edit_;
    QButtonGroup *out_file_name_gr_;
    QComboBox   *run_mode_cmb_;
    QLineEdit   *period_edit_;
    QLineEdit   *hex_edit_;

    QPushButton *run_btn_;

    FileModifier *worker_;
    QThread *worker_thread_;

    void CreateUI();
};
#endif // MAINWINDOW_H
