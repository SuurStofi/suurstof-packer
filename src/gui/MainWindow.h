#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidget>
#include <QPushButton>
#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QProgressBar>
#include "../core/common.h"

namespace Packer {

class MainWindow : public QMainWindow {
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    
private slots:
    void onAddFiles();
    void onRemoveFile();
    void onMoveUp();
    void onMoveDown();
    void onBuild();
    void onOutputBrowse();
    void onItemSelectionChanged();
    
private:
    void setupUI();
    void updateButtonStates();
    bool validateInputs();
    void updateExecutionOrder();
    
    // UI Components
    QListWidget* m_fileList;
    QPushButton* m_addButton;
    QPushButton* m_removeButton;
    QPushButton* m_moveUpButton;
    QPushButton* m_moveDownButton;
    QPushButton* m_buildButton;
    QPushButton* m_outputBrowseButton;
    
    QCheckBox* m_waitForPreviousCheckbox;
    QComboBox* m_outputTypeCombo;
    QLineEdit* m_outputPathEdit;
    QProgressBar* m_progressBar;
    
    // Data
    std::vector<PEInfo> m_exeFiles;
};

} // namespace Packer

#endif // MAINWINDOW_H
