#include "mainwindow.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLabel>
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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
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
    // Create schema tracking requested values exactly
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
        // Seed initial sample data rows if table is empty
        query.exec("SELECT COUNT(*) FROM schedules");
        if (query.next() && query.value(0).toInt() == 0) {
            seedSampleData();
        }
    }
}

void MainWindow::seedSampleData()
{
    QSqlQuery query;
    query.prepare("INSERT INTO schedules (record_date, start_time, end_time, uploads_count, sched_start_date, sched_end_date) "
                  "VALUES (:rd, :st, :et, :uc, :ssd, :sed)");

    // Seed Row 1
    query.bindValue(":rd", "06/04/2026");
    query.bindValue(":st", "11:10 AM");
    query.bindValue(":et", "11:20 AM");
    query.bindValue(":uc", 6);
    query.bindValue(":ssd", "01/01/2027");
    query.bindValue(":sed", "01/03/2027");
    query.exec();

    // Seed Row 2
    query.bindValue(":rd", "06/04/2026");
    query.bindValue(":st", "12:30 PM");
    query.bindValue(":et", "01:30 PM");
    query.bindValue(":uc", 27);
    query.bindValue(":ssd", "01/04/2027");
    query.bindValue(":sed", "01/17/2027");
    query.exec();
}

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

    new QListWidgetItem(QIcon(), "  📋 View Records", m_sidebar);
    new QListWidgetItem(QIcon(), "  ➕ Add Record", m_sidebar);
    new QListWidgetItem(QIcon(), "  ⚙️ Tools & Backup", m_sidebar);
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

    // PAGE 2: Data Input Collection Entry Form
    QWidget *addPage = new QWidget();
    QVBoxLayout *addLayout = new QVBoxLayout(addPage);

    QLabel *formTitle = new QLabel("Register New Schedule Metrics Entry", addPage);
    formTitle->setStyleSheet("font-size: 18px; font-weight: bold; color: #2c3e50; margin-bottom: 15px;");
    addLayout->addWidget(formTitle);

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

    QPushButton *submitButton = new QPushButton("Save Entry Record to DB", formContainer);
    submitButton->setObjectName("btn-submit");
    formLayout->addRow("", submitButton);

    addLayout->addWidget(formContainer);
    addLayout->addStretch();

    // PAGE 3: Utility Interoperation Tools Panel (Import / Export)
    QWidget *toolsPage = new QWidget();
    QVBoxLayout *toolsLayout = new QVBoxLayout(toolsPage);

    QLabel *toolsTitle = new QLabel("Data Interoperability Portability Toolkit", toolsPage);
    toolsTitle->setStyleSheet("font-size: 18px; font-weight: bold; color: #2c3e50; margin-bottom: 20px;");
    toolsLayout->addWidget(toolsTitle);

    // Export Structural Blocks
    QLabel *exportLbl = new QLabel("Data Export Configurations (Active Pipeline DB to External)", toolsPage);
    exportLbl->setStyleSheet("font-weight: bold; color: #34495e;");
    toolsLayout->addWidget(exportLbl);

    QHBoxLayout *exportBtnLayout = new QHBoxLayout();
    QPushButton *btnExpCSV = new QPushButton("📤 Export Dataset to CSV File", toolsPage);
    QPushButton *btnExpJSON = new QPushButton("📤 Export Dataset to JSON Schema", toolsPage);
    QPushButton *btnExpDB = new QPushButton("📦 Native Database Backup Binary (.db)", toolsPage);
    exportBtnLayout->addWidget(btnExpCSV);
    exportBtnLayout->addWidget(btnExpJSON);
    exportBtnLayout->addWidget(btnExpDB);
    toolsLayout->addLayout(exportBtnLayout);

    //toolsLayout->addSpaceritem(new QSpacerItem(20, 30, QSizePolicy::Minimum, QSizePolicy::Fixed));
    // Change the lowercase 'i' to an uppercase 'I'
    toolsLayout->addSpacerItem(new QSpacerItem(20, 30, QSizePolicy::Minimum, QSizePolicy::Fixed));

    // Import Structural Blocks
    QLabel *importLbl = new QLabel("Data Import Configurations (External Source to Pipeline Table)", toolsPage);
    importLbl->setStyleSheet("font-weight: bold; color: #34495e;");
    toolsLayout->addWidget(importLbl);

    QHBoxLayout *importBtnLayout = new QHBoxLayout();
    QPushButton *btnImpCSV = new QPushButton("📥 Parse and Import CSV Data", toolsPage);
    QPushButton *btnImpJSON = new QPushButton("📥 Parse and Import JSON Manifest", toolsPage);
    QPushButton *btnImpDB = new QPushButton("📦 Restore Database Native File (.db)", toolsPage);
    importBtnLayout->addWidget(btnImpCSV);
    importBtnLayout->addWidget(btnImpJSON);
    importBtnLayout->addWidget(btnImpDB);
    toolsLayout->addLayout(importBtnLayout);

    toolsLayout->addStretch();

    // Assemble Indexed Pages to Engine Stack Container
    m_pageContainer->addWidget(viewPage);
    m_pageContainer->addWidget(addPage);
    m_pageContainer->addWidget(toolsPage);

    // Bind Base Containers to Global Interface Core Shell
    mainLayout->addWidget(m_sidebar);
    mainLayout->addWidget(m_pageContainer);
    setCentralWidget(centralWidget);

    // --- SIGNALS & SLOTS EVENT HANDLERS LINKING ---
    connect(m_sidebar, &QListWidget::currentRowChanged, this, &MainWindow::changePage);
    connect(submitButton, &QPushButton::clicked, this, &MainWindow::handleAddRecord);
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
        "QPushButton#btn-submit:hover { background-color: #2c5282; }";

    this->setStyleSheet(styles);
}

void MainWindow::changePage(int index)
{
    m_pageContainer->setCurrentIndex(index);
    if(index == 0) {
        loadRecordsFromDb(); // Instantly sync grid visibility if switched to viewing panel
    }
}

void MainWindow::loadRecordsFromDb()
{
    m_tableWidget->setRowCount(0); // Clean staging viewport

    QSqlQuery query("SELECT record_date, start_time, end_time, uploads_count, sched_start_date, sched_end_date FROM schedules");
    int row = 0;

    while(query.next()) {
        m_tableWidget->insertRow(row);
        for(int col = 0; col < 6; ++col) {
            QTableWidgetItem *item = new QTableWidgetItem(query.value(col).toString());
            item->setTextAlignment(Qt::AlignCenter);
            m_tableWidget->setItem(row, col, item);
        }
        row++;
    }
}

void MainWindow::handleAddRecord()
{
    // Extract formatted strings mirroring exactly requested masks
    QString recDate = m_dateEdit->date().toString("MM/dd/yyyy");
    QString sTime = m_startTimeEdit->time().toString("hh:mm AP");
    QString eTime = m_endTimeEdit->time().toString("hh:mm AP");
    int uploads = m_uploadsSpinBox->value();
    QString schStart = m_schedStartEdit->date().toString("MM/dd/yyyy");
    QString schEnd = m_schedEndEdit->date().toString("MM/dd/yyyy");

    QSqlQuery query;
    query.prepare("INSERT INTO schedules (record_date, start_time, end_time, uploads_count, sched_start_date, sched_end_date) "
                  "VALUES (:rd, :st, :et, :uc, :ssd, :sed)");
    query.bindValue(":rd", recDate);
    query.bindValue(":st", sTime);
    query.bindValue(":et", eTime);
    query.bindValue(":uc", uploads);
    query.bindValue(":ssd", schStart);
    query.bindValue(":sed", schEnd);

    if(!query.exec()) {
        QMessageBox::critical(this, "Insertion Failed", "Database commit failure error logs: " + query.lastError().text());
    } else {
        QMessageBox::information(this, "Success", "Record successfully logged inside structural Database.");
        // Reset inputs fields contextively
        m_uploadsSpinBox->setValue(0);
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
    // Write Structured Column Headers Line
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

    // Must checkpoint temporary connections safely by ensuring cache flushes or simple native copy mechanics
    m_db.close();

    QFile::remove(destPath); // Sweep existing targets to rewrite clean raw copy safely
    bool ok = QFile::copy(currentDbPath, destPath);

    if(!m_db.open()) { // Re-establish active binding connection pipeline to running engine execution track
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
    QString headerLine = in.readLine(); // Pop and dismiss first schema identification header safely

    QSqlQuery query;
    m_db.transaction(); // Wrap insertion stack sequences dynamically under accelerated memory pipelines transaction block

    while(!in.atEnd()) {
        QString line = in.readLine();
        if(line.trimmed().isEmpty()) continue;

        // Basic parser variant checking explicitly for string delimiters removal
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

    m_db.close(); // Detach structural pipelines engine hooks cleanly
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