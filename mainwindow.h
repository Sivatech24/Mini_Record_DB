#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSqlDatabase>
#include <QTableWidget>
#include <QStackedWidget>
#include <QListWidget>
#include <QDateEdit>
#include <QTimeEdit>
#include <QSpinBox>
#include <QPushButton>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void changePage(int index);
    void handleAddRecord();
    void loadRecordsFromDb();
    void exportAsCSV();
    void exportAsJSON();
    void exportAsDB();
    void importFromCSV();
    void importFromJSON();
    void importFromDB();

private:
    void initDatabase();
    void seedSampleData();
    void createUI();
    void applyStylesheet();

    // Database Connection Instance
    QSqlDatabase m_db;

    // Core UI Containers
    QListWidget *m_sidebar;
    QStackedWidget *m_pageContainer;
    QTableWidget *m_tableWidget;

    // Form Entry Fields
    QDateEdit *m_dateEdit;
    QTimeEdit *m_startTimeEdit;
    QTimeEdit *m_endTimeEdit;
    QSpinBox *m_uploadsSpinBox;
    QDateEdit *m_schedStartEdit;
    QDateEdit *m_schedEndEdit;
};

#endif // MAINWINDOW_H