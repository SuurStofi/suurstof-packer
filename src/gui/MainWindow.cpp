#include "MainWindow.h"
#include "../core/PEParser.h"
#include "../core/ResourceEmbedder.h"
#include "../core/Obfuscator.h"
#include "../core/StubGenerator.h"
#include "../utils/FileTypeDetector.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include <QStatusBar>
#include <fstream>

namespace Packer {

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {
    setupUI();
    updateButtonStates();
}

MainWindow::~MainWindow() {
}

void MainWindow::setupUI() {
    setWindowTitle("SuurStof-Packer");
    resize(600, 500);  // Smaller default size
    setMinimumSize(500, 400);  // Set minimum size for better usability
    
    // Central widget
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    
    // File list section
    QGroupBox* fileGroup = new QGroupBox("Files (Top = First to Execute)", this);
    QVBoxLayout* fileLayout = new QVBoxLayout(fileGroup);
    
    m_fileList = new QListWidget(this);
    m_fileList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_fileList->setDragDropMode(QAbstractItemView::InternalMove);
    connect(m_fileList, &QListWidget::itemSelectionChanged, 
            this, &MainWindow::onItemSelectionChanged);
    
    fileLayout->addWidget(m_fileList);
    
    // File control buttons
    QHBoxLayout* fileButtonLayout = new QHBoxLayout();
    m_addButton = new QPushButton("Add Files", this);
    m_removeButton = new QPushButton("Remove", this);
    m_moveUpButton = new QPushButton("Move Up ↑", this);
    m_moveDownButton = new QPushButton("Move Down ↓", this);
    
    connect(m_addButton, &QPushButton::clicked, this, &MainWindow::onAddFiles);
    connect(m_removeButton, &QPushButton::clicked, this, &MainWindow::onRemoveFile);
    connect(m_moveUpButton, &QPushButton::clicked, this, &MainWindow::onMoveUp);
    connect(m_moveDownButton, &QPushButton::clicked, this, &MainWindow::onMoveDown);
    
    fileButtonLayout->addWidget(m_addButton);
    fileButtonLayout->addWidget(m_removeButton);
    fileButtonLayout->addWidget(m_moveUpButton);
    fileButtonLayout->addWidget(m_moveDownButton);
    fileButtonLayout->addStretch();
    
    fileLayout->addLayout(fileButtonLayout);
    mainLayout->addWidget(fileGroup);
    
    // Options section
    QGroupBox* optionsGroup = new QGroupBox("Options", this);
    QVBoxLayout* optionsLayout = new QVBoxLayout(optionsGroup);
    
    // Execution options
    m_waitForPreviousCheckbox = new QCheckBox("Wait for each program to close before running next", this);
    m_waitForPreviousCheckbox->setChecked(true);
    
    optionsLayout->addWidget(m_waitForPreviousCheckbox);
    
    // Output type
    QHBoxLayout* outputTypeLayout = new QHBoxLayout();
    outputTypeLayout->addWidget(new QLabel("Output Type:", this));
    m_outputTypeCombo = new QComboBox(this);
    m_outputTypeCombo->addItem("EXE");
    m_outputTypeCombo->addItem("DLL");
    outputTypeLayout->addWidget(m_outputTypeCombo);
    outputTypeLayout->addStretch();
    optionsLayout->addLayout(outputTypeLayout);
    
    // Output path
    QHBoxLayout* outputPathLayout = new QHBoxLayout();
    outputPathLayout->addWidget(new QLabel("Output Path:", this));
    m_outputPathEdit = new QLineEdit(this);
    m_outputPathEdit->setPlaceholderText("Select output file location...");
    m_outputBrowseButton = new QPushButton("Browse...", this);
    connect(m_outputBrowseButton, &QPushButton::clicked, this, &MainWindow::onOutputBrowse);
    
    outputPathLayout->addWidget(m_outputPathEdit);
    outputPathLayout->addWidget(m_outputBrowseButton);
    optionsLayout->addLayout(outputPathLayout);
    
    mainLayout->addWidget(optionsGroup);
    
    // Progress bar
    m_progressBar = new QProgressBar(this);
    m_progressBar->setValue(0);
    m_progressBar->setVisible(false);
    mainLayout->addWidget(m_progressBar);
    
    // Build button
    m_buildButton = new QPushButton("Build Packed Executable", this);
    m_buildButton->setMinimumHeight(40);
    connect(m_buildButton, &QPushButton::clicked, this, &MainWindow::onBuild);
    mainLayout->addWidget(m_buildButton);
    
    // Status bar
    statusBar()->showMessage("Ready");
}

void MainWindow::onAddFiles() {
    QStringList fileNames = QFileDialog::getOpenFileNames(
        this,
        "Select Files to Pack",
        "",
        "All Files (*.*);;Executables (*.exe);;Scripts (*.bat *.cmd *.ps1);;Documents (*.txt *.pdf)"
    );
    
    if (fileNames.isEmpty()) {
        return;
    }
    
    for (const QString& fileName : fileNames) {
        FileInfo fileInfo;
        std::wstring filePath = fileName.toStdWString();
        
        // Load file data
        std::ifstream file(filePath.c_str(), std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            QMessageBox::warning(this, "Error", 
                QString("Failed to open file: %1").arg(fileName));
            continue;
        }
        
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        
        fileInfo.fileData.resize(size);
        if (!file.read(reinterpret_cast<char*>(fileInfo.fileData.data()), size)) {
            QMessageBox::warning(this, "Error", 
                QString("Failed to read file: %1").arg(fileName));
            continue;
        }
        
        fileInfo.filePath = filePath;
        fileInfo.fileSize = size;
        fileInfo.executionOrder = static_cast<int>(m_exeFiles.size());
        
        // Extract filename
        size_t lastSlash = filePath.find_last_of(L"\\/");
        fileInfo.originalName = (lastSlash != std::wstring::npos) ? 
            filePath.substr(lastSlash + 1) : filePath;
        
        // Detect file type
        fileInfo.fileType = FileTypeDetector::detectFileType(filePath);
        fileInfo.extension = FileTypeDetector::getExtension(filePath);
        
        // If it's an executable, try to parse PE info
        if (fileInfo.fileType == FileType::EXECUTABLE) {
            PEParser parser;
            PEInfo peInfo;
            if (parser.loadFile(filePath, peInfo)) {
                fileInfo.is64Bit = peInfo.is64Bit;
                fileInfo.entryPoint = peInfo.entryPoint;
                fileInfo.imageBase = peInfo.imageBase;
            }
            fileInfo.obfuscate = false;  // Obfuscation disabled
        }
        
        m_exeFiles.push_back(fileInfo);
        
        QString displayName = QString("[%1] %2 (%3, %4 KB)")
            .arg(fileInfo.executionOrder + 1)
            .arg(QString::fromStdWString(fileInfo.originalName))
            .arg(QString::fromStdWString(FileTypeDetector::getFileTypeString(fileInfo.fileType)))
            .arg(fileInfo.fileSize / 1024);
        
        m_fileList->addItem(displayName);
        
        statusBar()->showMessage(QString("Added: %1")
            .arg(QString::fromStdWString(fileInfo.originalName)), 3000);
    }
    
    updateButtonStates();
}

void MainWindow::onRemoveFile() {
    int currentRow = m_fileList->currentRow();
    if (currentRow < 0) {
        return;
    }
    
    m_exeFiles.erase(m_exeFiles.begin() + currentRow);
    delete m_fileList->takeItem(currentRow);
    
    updateExecutionOrder();
    updateButtonStates();
}

void MainWindow::onMoveUp() {
    int currentRow = m_fileList->currentRow();
    if (currentRow <= 0) {
        return;
    }
    
    // Swap in vector
    std::swap(m_exeFiles[currentRow], m_exeFiles[currentRow - 1]);
    
    // Swap in list
    QListWidgetItem* item = m_fileList->takeItem(currentRow);
    m_fileList->insertItem(currentRow - 1, item);
    m_fileList->setCurrentRow(currentRow - 1);
    
    updateExecutionOrder();
}

void MainWindow::onMoveDown() {
    int currentRow = m_fileList->currentRow();
    if (currentRow < 0 || currentRow >= m_fileList->count() - 1) {
        return;
    }
    
    // Swap in vector
    std::swap(m_exeFiles[currentRow], m_exeFiles[currentRow + 1]);
    
    // Swap in list
    QListWidgetItem* item = m_fileList->takeItem(currentRow);
    m_fileList->insertItem(currentRow + 1, item);
    m_fileList->setCurrentRow(currentRow + 1);
    
    updateExecutionOrder();
}

void MainWindow::onBuild() {
    if (!validateInputs()) {
        return;
    }
    
    m_progressBar->setVisible(true);
    m_progressBar->setValue(0);
    m_buildButton->setEnabled(false);
    statusBar()->showMessage("Building...");
    
    try {
        // Step 1: Generate packed executable
        statusBar()->showMessage("Generating packed executable...");
        StubGenerator stubGen;
        PackerOptions opts;
        opts.outputType = m_outputTypeCombo->currentText() == "EXE" ? 
                         OutputType::EXE : OutputType::DLL;
        opts.outputPath = m_outputPathEdit->text().toStdWString();
        opts.obfuscateFinal = false;
        opts.waitForPrevious = m_waitForPreviousCheckbox->isChecked();
        
        std::vector<uint8_t> finalOutput;
        if (!stubGen.generatePackedExecutable(m_exeFiles, opts, finalOutput)) {
            throw std::runtime_error("Failed to generate packed executable");
        }
        
        m_progressBar->setValue(80);
        QApplication::processEvents();
        
        // Step 2: Write output file
        statusBar()->showMessage("Writing output file...");
        std::string outputPath = m_outputPathEdit->text().toStdString();
        std::ofstream outFile(outputPath, std::ios::binary);
        
        if (!outFile.is_open()) {
            throw std::runtime_error("Failed to open output file for writing");
        }
        
        outFile.write(reinterpret_cast<const char*>(finalOutput.data()), 
                     finalOutput.size());
        outFile.close();
        
        if (!outFile.good()) {
            throw std::runtime_error("Failed to write output file");
        }
        
        m_progressBar->setValue(100);
        statusBar()->showMessage("Build completed successfully!", 5000);
        
        QMessageBox::information(this, "Success", 
            "Packed executable created successfully!");
        
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Error", 
            QString("Build failed: %1").arg(e.what()));
        statusBar()->showMessage("Build failed", 5000);
    }
    
    m_progressBar->setVisible(false);
    m_buildButton->setEnabled(true);
}

void MainWindow::onOutputBrowse() {
    QString extension = m_outputTypeCombo->currentText() == "EXE" ? "exe" : "dll";
    QString filter = QString("%1 Files (*.%2);;All Files (*.*)")
        .arg(extension.toUpper())
        .arg(extension);
    
    QString fileName = QFileDialog::getSaveFileName(
        this,
        "Select Output File",
        QString("packed.%1").arg(extension),
        filter
    );
    
    if (!fileName.isEmpty()) {
        m_outputPathEdit->setText(fileName);
    }
}

void MainWindow::onItemSelectionChanged() {
    updateButtonStates();
}

void MainWindow::updateButtonStates() {
    int currentRow = m_fileList->currentRow();
    int count = m_fileList->count();
    
    m_removeButton->setEnabled(currentRow >= 0);
    m_moveUpButton->setEnabled(currentRow > 0);
    m_moveDownButton->setEnabled(currentRow >= 0 && currentRow < count - 1);
    m_buildButton->setEnabled(count > 0 && !m_outputPathEdit->text().isEmpty());
}

bool MainWindow::validateInputs() {
    if (m_exeFiles.empty()) {
        QMessageBox::warning(this, "Validation Error", 
            "Please add at least one executable file.");
        return false;
    }
    
    if (m_outputPathEdit->text().isEmpty()) {
        QMessageBox::warning(this, "Validation Error", 
            "Please specify output file path.");
        return false;
    }
    
    return true;
}

void MainWindow::updateExecutionOrder() {
    for (size_t i = 0; i < m_exeFiles.size(); i++) {
        m_exeFiles[i].executionOrder = static_cast<int>(i);
        
        QString displayName = QString("[%1] %2 (%3-bit, %4 KB)")
            .arg(i + 1)
            .arg(QString::fromStdWString(m_exeFiles[i].originalName))
            .arg(m_exeFiles[i].is64Bit ? 64 : 32)
            .arg(m_exeFiles[i].fileSize / 1024);
        
        m_fileList->item(static_cast<int>(i))->setText(displayName);
    }
}

} // namespace Packer
