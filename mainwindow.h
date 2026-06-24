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
#include <QLabel>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void changePage(int index);
    void handleSaveRecord(); // Handles BOTH Add and Update
    void loadRecordsFromDb();

    // New Action Slots
    void editSelectedRecord();
    void deleteSelectedRecord();
    void resetFormToAddMode();

    // Portability Slots
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

    // Form Entry Fields & Dynamic UI Elements
    QLabel *m_formTitle;
    QPushButton *m_submitButton;
    QDateEdit *m_dateEdit;
    QTimeEdit *m_startTimeEdit;
    QTimeEdit *m_endTimeEdit;
    QSpinBox *m_uploadsSpinBox;
    QDateEdit *m_schedStartEdit;
    QDateEdit *m_schedEndEdit;

    // State Tracking
    int m_currentEditId; // Tracks the DB ID of the record being edited. -1 means "Adding New"
};

#endif // MAINWINDOW_H