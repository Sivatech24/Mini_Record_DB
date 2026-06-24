#include "mainwindow.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QFileDialog>
#include <QSqlQuery>
#include <QSqlError>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_currentEditId(-1) // Initialize state to "Add Mode"
{
    initDatabase();
    createUI();
    applyStylesheet();
    loadRecordsFromDb();
}

MainWindow::~MainWindow()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
}

void MainWindow::initDatabase()
{
    QString dbPath = QCoreApplication::applicationDirPath() + "/database/records.db";

    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        QMessageBox::critical(this, "Database Error", "Unable to open or create database: " + m_db.lastError().text());
        return;
    }

    QSqlQuery query;
    bool success = query.exec(
        "CREATE TABLE IF NOT EXISTS schedules ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "record_date TEXT, "
        "start_time TEXT, "
        "end_time TEXT, "
        "uploads_count INTEGER, "
        "sched_start_date TEXT, "
        "sched_end_date TEXT)"
        );

    if (!success) {
        QMessageBox::critical(this, "Table Creation Error", query.lastError().text());
    } else {
        query.exec("SELECT COUNT(*) FROM schedules");
        if (query.next() && query.value(0).toInt() == 0) {
            // seedSampleData();
        }
    }
}

/*void MainWindow::seedSampleData()
{
    QSqlQuery query;
    query.prepare("INSERT INTO schedules (record_date, start_time, end_time, uploads_count, sched_start_date, sched_end_date) "
                  "VALUES (:rd, :st, :et, :uc, :ssd, :sed)");

    query.bindValue(":rd", "06/04/2026");
    query.bindValue(":st", "11:10 AM");
    query.bindValue(":et", "11:20 AM");
    query.bindValue(":uc", 6);
    query.bindValue(":ssd", "01/01/2027");
    query.bindValue(":sed", "01/03/2027");
    query.exec();

    query.bindValue(":rd", "06/04/2026");
    query.bindValue(":st", "12:30 PM");
    query.bindValue(":et", "01:30 PM");
    query.bindValue(":uc", 27);
    query.bindValue(":ssd", "01/04/2027");
    query.bindValue(":sed", "01/17/2027");
    query.exec();
} */

void MainWindow::createUI()
{
    this->setWindowTitle("Schedule Database Manager Dashboard");
    this->resize(1000, 600);

    QWidget *centralWidget = new QWidget(this);
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // --- SIDE NAVIGATION BAR ---
    m_sidebar = new QListWidget(this);
    m_sidebar->setObjectName("sidebar");
    m_sidebar->setViewMode(QListView::ListMode);
    m_sidebar->setFixedWidth(200);

    new QListWidgetItem(QIcon(), "   View Records", m_sidebar);
    new QListWidgetItem(QIcon(), "   Add Record", m_sidebar);
    new QListWidgetItem(QIcon(), "   Tools & Backup", m_sidebar);
    m_sidebar->setCurrentRow(0);

    // --- STACKED CONTAINER PAGES ---
    m_pageContainer = new QStackedWidget(this);

    // PAGE 1: Data Grid View Table Layout
    QWidget *viewPage = new QWidget();
    QVBoxLayout *viewLayout = new QVBoxLayout(viewPage);

    QLabel *viewTitle = new QLabel("Stored Datatable Records", viewPage);
    viewTitle->setStyleSheet("font-size: 18px; font-weight: bold; color: #2c3e50; margin-bottom: 10px;");
    viewLayout->addWidget(viewTitle);

    m_tableWidget = new QTableWidget(this);
    m_tableWidget->setColumnCount(6);
    m_tableWidget->setHorizontalHeaderLabels({
        "Date\n(mm/dd/yyyy)", "Start Time\n(12h hh:mm AM/PM)", "End Time\n(12h hh:mm AM/PM)",
        "Number of\nUploads", "Schedule Start\nDate (mm/dd/yyyy)", "Schedule End\nDate (mm/dd/yyyy)"
    });
    m_tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    viewLayout->addWidget(m_tableWidget);

    // Contextual Action Buttons for View Page
    QHBoxLayout *actionLayout = new QHBoxLayout();
    QPushButton *btnEdit = new QPushButton(" Edit Selected Record", viewPage);
    QPushButton *btnDelete = new QPushButton(" Delete Selected", viewPage);
    btnDelete->setObjectName("btn-delete"); // for styling
    actionLayout->addWidget(btnEdit);
    actionLayout->addWidget(btnDelete);
    actionLayout->addStretch();
    viewLayout->addLayout(actionLayout);

    // PAGE 2: Data Input Collection Entry Form
    QWidget *addPage = new QWidget();
    QVBoxLayout *addLayout = new QVBoxLayout(addPage);

    m_formTitle = new QLabel("Register New Schedule Metrics Entry", addPage);
    m_formTitle->setStyleSheet("font-size: 18px; font-weight: bold; color: #2c3e50; margin-bottom: 15px;");
    addLayout->addWidget(m_formTitle);

    QWidget *formContainer = new QWidget(addPage);
    QFormLayout *formLayout = new QFormLayout(formContainer);
    formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    formLayout->setSpacing(12);

    m_dateEdit = new QDateEdit(QDate::currentDate(), formContainer);
    m_dateEdit->setDisplayFormat("MM/dd/yyyy");
    m_dateEdit->setCalendarPopup(true);

    m_startTimeEdit = new QTimeEdit(QTime::currentTime(), formContainer);
    m_startTimeEdit->setDisplayFormat("hh:mm AP");

    m_endTimeEdit = new QTimeEdit(QTime::currentTime().addSecs(3600), formContainer);
    m_endTimeEdit->setDisplayFormat("hh:mm AP");

    m_uploadsSpinBox = new QSpinBox(formContainer);
    m_uploadsSpinBox->setRange(0, 100000);
    m_uploadsSpinBox->setValue(0);

    m_schedStartEdit = new QDateEdit(QDate::currentDate(), formContainer);
    m_schedStartEdit->setDisplayFormat("MM/dd/yyyy");
    m_schedStartEdit->setCalendarPopup(true);

    m_schedEndEdit = new QDateEdit(QDate::currentDate().addDays(7), formContainer);
    m_schedEndEdit->setDisplayFormat("MM/dd/yyyy");
    m_schedEndEdit->setCalendarPopup(true);

    formLayout->addRow("Record Reference Date:", m_dateEdit);
    formLayout->addRow("Operation Start Time:", m_startTimeEdit);
    formLayout->addRow("Operation End Time:", m_endTimeEdit);
    formLayout->addRow("Total Upload Payload Count:", m_uploadsSpinBox);
    formLayout->addRow("Deployment Target Start Window:", m_schedStartEdit);
    formLayout->addRow("Deployment Target Termination Window:", m_schedEndEdit);

    m_submitButton = new QPushButton("Save Entry Record to DB", formContainer);
    m_submitButton->setObjectName("btn-submit");
    formLayout->addRow("", m_submitButton);

    addLayout->addWidget(formContainer);
    addLayout->addStretch();

    // PAGE 3: Utility Interoperation Tools Panel (Import / Export)
    QWidget *toolsPage = new QWidget();
    QVBoxLayout *toolsLayout = new QVBoxLayout(toolsPage);

    QLabel *toolsTitle = new QLabel("Data Interoperability Portability Toolkit", toolsPage);
    toolsTitle->setStyleSheet("font-size: 18px; font-weight: bold; color: #2c3e50; margin-bottom: 20px;");
    toolsLayout->addWidget(toolsTitle);

    QLabel *exportLbl = new QLabel("Data Export Configurations (Active Pipeline DB to External)", toolsPage);
    exportLbl->setStyleSheet("font-weight: bold; color: #34495e;");
    toolsLayout->addWidget(exportLbl);

    QHBoxLayout *exportBtnLayout = new QHBoxLayout();
    QPushButton *btnExpCSV = new QPushButton("Export Dataset to CSV File", toolsPage);
    QPushButton *btnExpJSON = new QPushButton("Export Dataset to JSON Schema", toolsPage);
    QPushButton *btnExpDB = new QPushButton("Native Database Backup Binary (.db)", toolsPage);
    exportBtnLayout->addWidget(btnExpCSV);
    exportBtnLayout->addWidget(btnExpJSON);
    exportBtnLayout->addWidget(btnExpDB);
    toolsLayout->addLayout(exportBtnLayout);

    toolsLayout->addSpacerItem(new QSpacerItem(20, 30, QSizePolicy::Minimum, QSizePolicy::Fixed));

    QLabel *importLbl = new QLabel("Data Import Configurations (External Source to Pipeline Table)", toolsPage);
    importLbl->setStyleSheet("font-weight: bold; color: #34495e;");
    toolsLayout->addWidget(importLbl);

    QHBoxLayout *importBtnLayout = new QHBoxLayout();
    QPushButton *btnImpCSV = new QPushButton(" Parse and Import CSV Data", toolsPage);
    QPushButton *btnImpJSON = new QPushButton(" Parse and Import JSON Manifest", toolsPage);
    QPushButton *btnImpDB = new QPushButton(" Restore Database Native File (.db)", toolsPage);
    importBtnLayout->addWidget(btnImpCSV);
    importBtnLayout->addWidget(btnImpJSON);
    importBtnLayout->addWidget(btnImpDB);
    toolsLayout->addLayout(importBtnLayout);

    toolsLayout->addStretch();

    m_pageContainer->addWidget(viewPage);
    m_pageContainer->addWidget(addPage);
    m_pageContainer->addWidget(toolsPage);

    mainLayout->addWidget(m_sidebar);
    mainLayout->addWidget(m_pageContainer);
    setCentralWidget(centralWidget);

    // --- SIGNALS & SLOTS EVENT HANDLERS LINKING ---
    connect(m_sidebar, &QListWidget::currentRowChanged, this, &MainWindow::changePage);
    connect(m_submitButton, &QPushButton::clicked, this, &MainWindow::handleSaveRecord);
    connect(btnEdit, &QPushButton::clicked, this, &MainWindow::editSelectedRecord);
    connect(btnDelete, &QPushButton::clicked, this, &MainWindow::deleteSelectedRecord);

    connect(btnExpCSV, &QPushButton::clicked, this, &MainWindow::exportAsCSV);
    connect(btnExpJSON, &QPushButton::clicked, this, &MainWindow::exportAsJSON);
    connect(btnExpDB, &QPushButton::clicked, this, &MainWindow::exportAsDB);
    connect(btnImpCSV, &QPushButton::clicked, this, &MainWindow::importFromCSV);
    connect(btnImpJSON, &QPushButton::clicked, this, &MainWindow::importFromJSON);
    connect(btnImpDB, &QPushButton::clicked, this, &MainWindow::importFromDB);
}

void MainWindow::applyStylesheet()
{
    QString styles =
        "QMainWindow { background-color: #f8f9fa; }"
        "QListWidget#sidebar { background-color: #2c3e50; border: none; font-size: 14px; }"
        "QListWidget#sidebar::item { color: #ecf0f1; padding: 15px 10px; border-bottom: 1px solid qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #34495e, stop:1 #2c3e50); }"
        "QListWidget#sidebar::item:selected { background-color: #1abc9c; color: white; font-weight: bold; }"
        "QListWidget#sidebar::item:hover { background-color: #34495e; }"
        "QStackedWidget { padding: 25px; background-color: #ffffff; }"
        "QTableWidget { border: 1px solid #e2e8f0; gridline-color: #edf2f7; background-color: white; selection-background-color: #e2f0fd; selection-color: #2b6cb0; font-size: 12px; }"
        "QHeaderView::section { background-color: #f7fafc; padding: 6px; border: 1px solid #e2e8f0; font-weight: bold; color: #4a5568; text-align: center; }"
        "QLineEdit, QDateEdit, QTimeEdit, QSpinBox { border: 1px solid #cbd5e0; border-radius: 4px; padding: 6px 12px; min-width: 250px; background-color: #fff; font-size: 13px; }"
        "QLineEdit:focus, QDateEdit:focus, QTimeEdit:focus, QSpinBox:focus { border: 1px solid #3182ce; }"
        "QPushButton { background-color: #4a5568; color: white; border: none; padding: 10px 18px; border-radius: 4px; font-weight: bold; font-size: 13px; }"
        "QPushButton:hover { background-color: #2d3748; }"
        "QPushButton#btn-submit { background-color: #2b6cb0; padding: 12px; }"
        "QPushButton#btn-submit:hover { background-color: #2c5282; }"
        "QPushButton#btn-delete { background-color: #e53e3e; }"
        "QPushButton#btn-delete:hover { background-color: #c53030; }";

    this->setStyleSheet(styles);
}

void MainWindow::changePage(int index)
{
    m_pageContainer->setCurrentIndex(index);
    if(index == 0) {
        loadRecordsFromDb();
    } else if (index == 1) {
        // If user clicks "Add Record" from sidebar, ensure form is clean and in Add Mode
        resetFormToAddMode();
    }
}

void MainWindow::resetFormToAddMode()
{
    m_currentEditId = -1; // Reset tracker
    m_formTitle->setText("Register New Schedule Metrics Entry");
    m_submitButton->setText("Save Entry Record to DB");

    // Reset fields to default
    m_dateEdit->setDate(QDate::currentDate());
    m_startTimeEdit->setTime(QTime::currentTime());
    m_endTimeEdit->setTime(QTime::currentTime().addSecs(3600));
    m_uploadsSpinBox->setValue(0);
    m_schedStartEdit->setDate(QDate::currentDate());
    m_schedEndEdit->setDate(QDate::currentDate().addDays(7));
}

void MainWindow::loadRecordsFromDb()
{
    m_tableWidget->setRowCount(0);

    // Notice we are now fetching 'id' as well
    QSqlQuery query("SELECT id, record_date, start_time, end_time, uploads_count, sched_start_date, sched_end_date FROM schedules");
    int row = 0;

    while(query.next()) {
        m_tableWidget->insertRow(row);

        for(int col = 0; col < 6; ++col) {
            // Visual columns correspond to SQL index col + 1 (since id is at index 0)
            QTableWidgetItem *item = new QTableWidgetItem(query.value(col + 1).toString());
            item->setTextAlignment(Qt::AlignCenter);

            // On the first visual column, securely stash the Database ID out of sight
            if(col == 0) {
                item->setData(Qt::UserRole, query.value(0).toInt());
            }

            m_tableWidget->setItem(row, col, item);
        }
        row++;
    }
}

void MainWindow::deleteSelectedRecord()
{
    int currentRow = m_tableWidget->currentRow();
    if (currentRow < 0) {
        QMessageBox::warning(this, "Selection Required", "Please select a record from the table to delete.");
        return;
    }

    // Retrieve the securely stored ID
    int idToDelete = m_tableWidget->item(currentRow, 0)->data(Qt::UserRole).toInt();

    QMessageBox::StandardButton reply = QMessageBox::question(this, "Confirm Deletion",
                                                              "Are you sure you want to permanently delete this record?",
                                                              QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        QSqlQuery query;
        query.prepare("DELETE FROM schedules WHERE id = :id");
        query.bindValue(":id", idToDelete);

        if(query.exec()) {
            loadRecordsFromDb(); // Refresh Grid
        } else {
            QMessageBox::critical(this, "Error", "Failed to delete record: " + query.lastError().text());
        }
    }
}

void MainWindow::editSelectedRecord()
{
    int currentRow = m_tableWidget->currentRow();
    if (currentRow < 0) {
        QMessageBox::warning(this, "Selection Required", "Please select a record from the table to edit.");
        return;
    }

    // 1. Get the DB ID
    m_currentEditId = m_tableWidget->item(currentRow, 0)->data(Qt::UserRole).toInt();

    // 2. Parse existing data back into form inputs
    m_dateEdit->setDate(QDate::fromString(m_tableWidget->item(currentRow, 0)->text(), "MM/dd/yyyy"));
    m_startTimeEdit->setTime(QTime::fromString(m_tableWidget->item(currentRow, 1)->text(), "hh:mm AP"));
    m_endTimeEdit->setTime(QTime::fromString(m_tableWidget->item(currentRow, 2)->text(), "hh:mm AP"));
    m_uploadsSpinBox->setValue(m_tableWidget->item(currentRow, 3)->text().toInt());
    m_schedStartEdit->setDate(QDate::fromString(m_tableWidget->item(currentRow, 4)->text(), "MM/dd/yyyy"));
    m_schedEndEdit->setDate(QDate::fromString(m_tableWidget->item(currentRow, 5)->text(), "MM/dd/yyyy"));

    // 3. Update Form UI to reflect Update Mode
    m_formTitle->setText("Modify Existing Schedule Entry");
    m_submitButton->setText("Update Existing Record");

    // 4. Navigate the application to the form page visually
    m_pageContainer->setCurrentIndex(1);
    m_sidebar->setCurrentRow(1); // Keep sidebar highlight consistent
}

void MainWindow::handleSaveRecord()
{
    QString recDate = m_dateEdit->date().toString("MM/dd/yyyy");
    QString sTime = m_startTimeEdit->time().toString("hh:mm AP");
    QString eTime = m_endTimeEdit->time().toString("hh:mm AP");
    int uploads = m_uploadsSpinBox->value();
    QString schStart = m_schedStartEdit->date().toString("MM/dd/yyyy");
    QString schEnd = m_schedEndEdit->date().toString("MM/dd/yyyy");

    QSqlQuery query;

    // Check our tracker to see if we are INSERTING (New) or UPDATING (Existing)
    if (m_currentEditId == -1) {
        query.prepare("INSERT INTO schedules (record_date, start_time, end_time, uploads_count, sched_start_date, sched_end_date) "
                      "VALUES (:rd, :st, :et, :uc, :ssd, :sed)");
    } else {
        query.prepare("UPDATE schedules SET "
                      "record_date = :rd, start_time = :st, end_time = :et, "
                      "uploads_count = :uc, sched_start_date = :ssd, sched_end_date = :sed "
                      "WHERE id = :id");
        query.bindValue(":id", m_currentEditId);
    }

    query.bindValue(":rd", recDate);
    query.bindValue(":st", sTime);
    query.bindValue(":et", eTime);
    query.bindValue(":uc", uploads);
    query.bindValue(":ssd", schStart);
    query.bindValue(":sed", schEnd);

    if(!query.exec()) {
        QMessageBox::critical(this, "Execution Failed", "Database commit failure error logs: " + query.lastError().text());
    } else {
        QMessageBox::information(this, "Success", m_currentEditId == -1 ? "New record successfully logged." : "Existing record successfully updated.");
        resetFormToAddMode();
        m_sidebar->setCurrentRow(0); // Auto-navigate user to updated data overview panel
    }
}

// --- PORTABILITY CORE ALGORITHMS MODULE ---

void MainWindow::exportAsCSV()
{
    QString filePath = QFileDialog::getSaveFileName(this, "Export CSV Target", "", "Comma Separated Values (*.csv)");
    if(filePath.isEmpty()) return;

    QFile file(filePath);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "I/O Error", "Could not write file destination target access denied.");
        return;
    }

    QTextStream out(&file);
    out << "Date(mm/dd/yyyy),Start_time(12_Hours hh:mm am or pm),End_time(12_Hours hh:mm am or pm),Number_of_uploades,Schedule_Start_Date(mm/dd/yyyy),Schedule_End_Date(mm/dd/yyyy)\n";

    QSqlQuery query("SELECT record_date, start_time, end_time, uploads_count, sched_start_date, sched_end_date FROM schedules");
    while(query.next()) {
        out << QString("\"%1\",\"%2\",\"%3\",%4,\"%5\",\"%6\"\n")
        .arg(query.value(0).toString())
            .arg(query.value(1).toString())
            .arg(query.value(2).toString())
            .arg(query.value(3).toInt())
            .arg(query.value(4).toString())
            .arg(query.value(5).toString());
    }
    file.close();
    QMessageBox::information(this, "Export Success", "Data converted to standardized CSV schema.");
}

void MainWindow::exportAsJSON()
{
    QString filePath = QFileDialog::getSaveFileName(this, "Export JSON Target", "", "JSON Schema Document (*.json)");
    if(filePath.isEmpty()) return;

    QSqlQuery query("SELECT record_date, start_time, end_time, uploads_count, sched_start_date, sched_end_date FROM schedules");
    QJsonArray jsonArray;

    while(query.next()) {
        QJsonObject rowObj;
        rowObj["record_date"] = query.value(0).toString();
        rowObj["start_time"] = query.value(1).toString();
        rowObj["end_time"] = query.value(2).toString();
        rowObj["uploads_count"] = query.value(3).toInt();
        rowObj["sched_start_date"] = query.value(4).toString();
        rowObj["sched_end_date"] = query.value(5).toString();
        jsonArray.append(rowObj);
    }

    QJsonDocument doc(jsonArray);
    QFile file(filePath);
    if(!file.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, "I/O Error", "Could not open path destination.");
        return;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    QMessageBox::information(this, "Export Success", "Dataset structure normalized into Object Model JSON.");
}

void MainWindow::exportAsDB()
{
    QString currentDbPath = QCoreApplication::applicationDirPath() + "/database/records.db";
    QString destPath = QFileDialog::getSaveFileName(this, "Backup Active SQLite Base Binary Target", "records_backup.db", "Database Source Files (*.db)");
    if(destPath.isEmpty()) return;

    m_db.close();

    QFile::remove(destPath);
    bool ok = QFile::copy(currentDbPath, destPath);

    if(!m_db.open()) {
        qDebug() << "Re-opening failed post structural backup duplication.";
    }

    if(ok) {
        QMessageBox::information(this, "Backup Succeeded", "Engine SQLite binary layer replicated natively to safety zone destination.");
    } else {
        QMessageBox::critical(this, "Failure", "Copy operation exception framework failed execution logic sequence.");
    }
}

void MainWindow::importFromCSV()
{
    QString filePath = QFileDialog::getOpenFileName(this, "Parse Target Comma Separated Map", "", "CSV Manifest Maps (*.csv)");
    if(filePath.isEmpty()) return;

    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Access Error", "Target path file context resource read protection violation.");
        return;
    }

    QTextStream in(&file);
    QString headerLine = in.readLine();

    QSqlQuery query;
    m_db.transaction();

    while(!in.atEnd()) {
        QString line = in.readLine();
        if(line.trimmed().isEmpty()) continue;

        QStringList fields;
        QString currentField;
        bool insideQuotes = false;

        for (int i = 0; i < line.length(); ++i) {
            QChar ch = line.at(i);
            if (ch == '"') {
                insideQuotes = !insideQuotes;
            } else if (ch == ',' && !insideQuotes) {
                fields.append(currentField.trimmed());
                currentField.clear();
            } else {
                currentField.append(ch);
            }
        }
        fields.append(currentField.trimmed());

        if(fields.size() >= 6) {
            query.prepare("INSERT INTO schedules (record_date, start_time, end_time, uploads_count, sched_start_date, sched_end_date) "
                          "VALUES (:rd, :st, :et, :uc, :ssd, :sed)");
            query.bindValue(":rd", fields[0]);
            query.bindValue(":st", fields[1]);
            query.bindValue(":et", fields[2]);
            query.bindValue(":uc", fields[3].toInt());
            query.bindValue(":ssd", fields[4]);
            query.bindValue(":sed", fields[5]);
            query.exec();
        }
    }
    m_db.commit();
    file.close();
    loadRecordsFromDb();
    QMessageBox::information(this, "Import Complete", "CSV data parser complete. Main grid database updated.");
}

void MainWindow::importFromJSON()
{
    QString filePath = QFileDialog::getOpenFileName(this, "Parse Document Object Json Pipeline File", "", "Structured JSON Target System (*.json)");
    if(filePath.isEmpty()) return;

    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, "Error", "Failed to access JSON physical source pathway context object.");
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if(!doc.isArray()) {
        QMessageBox::critical(this, "Structural Fault", "The target data layout framework parsing standard must map string root structures explicitly within uniform standard Arrays.");
        return;
    }

    QJsonArray jsonArray = doc.array();
    QSqlQuery query;
    m_db.transaction();

    for(int i=0; i<jsonArray.size(); ++i) {
        QJsonObject rowObj = jsonArray.at(i).toObject();
        query.prepare("INSERT INTO schedules (record_date, start_time, end_time, uploads_count, sched_start_date, sched_end_date) "
                      "VALUES (:rd, :st, :et, :uc, :ssd, :sed)");
        query.bindValue(":rd", rowObj["record_date"].toString());
        query.bindValue(":st", rowObj["start_time"].toString());
        query.bindValue(":et", rowObj["end_time"].toString());
        query.bindValue(":uc", rowObj["uploads_count"].toInt());
        query.bindValue(":ssd", rowObj["sched_start_date"].toString());
        query.bindValue(":sed", rowObj["sched_end_date"].toString());
        query.exec();
    }
    m_db.commit();
    loadRecordsFromDb();
    QMessageBox::information(this, "Import Succeeded", "JSON entities systematically parsing framework committed smoothly into processing core.");
}

void MainWindow::importFromDB()
{
    QString targetBackupPath = QFileDialog::getOpenFileName(this, "Select Master Structural Native Db to Restore Framework", "", "Engine Storage Units (*.db)");
    if(targetBackupPath.isEmpty()) return;

    m_db.close();
    QString activeDbPath = QCoreApplication::applicationDirPath() + "/database/records.db";

    QFile::remove(activeDbPath);
    bool ok = QFile::copy(targetBackupPath, activeDbPath);

    if(!m_db.open()) {
        qDebug() << "Critical system crash during instance fallback reconnect framework sequence lifecycle initialization phase.";
    }

    if(ok) {
        loadRecordsFromDb();
        QMessageBox::information(this, "Restoration Success", "Binary instance replacement hotfix swapping parameters initialized successfully. Records loaded.");
    } else {
        QMessageBox::critical(this, "Critical Fail", "Filesystem replacement execution parameters rejected by context kernel.");
    }
}