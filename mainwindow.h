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

class FileModifier;
class ProgressDialog;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void slotStart();
    void slotDialogFinished();

    void slotExitRequested();
    void slotPauseRequested();
    void slotResumeRequested();

signals:
    void signalStartProcessing(const QString &input_path, const QString &output_path, bool modify_filename, const QString &mask,
        const QByteArray &key, bool remove_source);

private:
    QLineEdit   *mask_edit_;
    QCheckBox   *del_ifile_chck_;
    QLineEdit   *out_path_edit_;
    QLineEdit   *in_path_edit_;
    QButtonGroup *out_file_name_gr_;
    QComboBox   *run_mode_cmb_;
    QTimeEdit   *period_edit_;
    QLineEdit   *hex_edit_;

    QPushButton *run_btn_;

    bool is_timer_mode_;
    QTimer *timer_;

    FileModifier *worker_;
    QThread *worker_thread_;
    ProgressDialog *progress_dialog_;

    void CreateUI();
};
#endif // MAINWINDOW_H
