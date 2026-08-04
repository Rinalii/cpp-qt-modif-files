#ifndef PROGRESSDIALOG_H
#define PROGRESSDIALOG_H

#include <QDialog>

class QProgressBar;
class QLabel;
class QPushButton;

class ProgressDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ProgressDialog(QWidget *parent = nullptr);

public slots:
    void slotUpdateProgress(int percent);
    void slotUpdateCurrentFile(const QString &filename);
    void slotFinished(bool success, const QString &errorMessage = QString());
    void slotPauseResumeClicked();

signals:
    void signalStopRequested();
    void signalPauseRequested();
    void signalResumeRequested();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    QProgressBar *progress_bar_;
    QLabel *status_label_;
    QLabel *file_label_;
    QPushButton *pause_resume_btn_;
    QPushButton *stop_btn_;

    bool is_paused_;
    bool is_finished_;
};

#endif // PROGRESSDIALOG_H
