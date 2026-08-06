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

    void slotRunModeChanged(int idx);

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
    QLabel      *period_label_;
    QTimeEdit   *period_edit_;
    QLineEdit   *hex_edit_;

    QPushButton *run_btn_;

    bool is_timer_mode_;
    QTimer *timer_;

    FileModifier *worker_;
    QThread *worker_thread_;
    ProgressDialog *progress_dialog_;

private:
    void CreateMaskSection(QGridLayout *layout);
    void CreateDeleteSection(QGridLayout *layout);
    void CreatePathSection(QGridLayout *layout, int row, const QString &label_text, QLineEdit *&line_edit, const QString &dialog_title);
    void CreateOverwriteSection(QGridLayout *layout);
    void CreateRunModeSection(QGridLayout *layout);
    void CreatePeriodSection(QGridLayout *layout);
    void CreateKeySection(QGridLayout *layout);
    void CreateRunButton(QGridLayout *layout);

    void ShowFolderSelector(QLineEdit *lineEdit, const QString &title);
    void CreateUI();
};
#endif // MAINWINDOW_H
