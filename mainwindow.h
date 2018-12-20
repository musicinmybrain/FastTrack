#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <vector>
#include <QMainWindow>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QProgressBar>
#include <QImage>
#include <QFileDialog>
#include <QMessageBox>
#include <string>
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <fstream>
#include <QCheckBox>
#include <QSlider>
#include <QLCDNumber>
#include <QComboBox>
#include <QThread>
#include <QSpinBox>
#include <stdlib.h>
#include <QLayout>
#include <QWidget>
#include <QMap>
#include <QDir>
#include <QString>
#include <QSettings>
#include <QDir>
#include <QThread>
#include <QDebug>
#include <QTableWidgetItem>
#include "functions.h"
#include "setupwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QTimer>
#include <QFile>
#include <QFileInfo>
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <string>
#include <opencv2/core/ocl.hpp>
#include <ctime>


using namespace std;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();


private:
    Ui::MainWindow *ui;
    QMap<QString, QString> parameterList;

    QThread* thread;
    Tracking* tracking;

    QVector<QString> pathList;
    int pathCounter;

    int frameAnalyzed;


    SetupWindow *setupWindow;

    void loadSettings();
    void saveSettings();
    QSettings *settingsFile;

    // Replay panel
    vector<cv::String> replayFrames;                      // Path to frame to replay
    vector<Point3f> colorMap; 
    QVector<QString> replayTracking;                      // Tracking data to replay
    int replayNumberObject;                              // Number of object tracked to replay
    bool isReplayable;
    QTimer *framerate;
    int autoPlayerIndex;
    bool object;
    QSize replayDisplaySize;
    QSize resizedFrame;
    QSize originalImageSize;

public slots:
    void updateParameterList(QTableWidgetItem* item);
    void startTracking();
    void addPath();
    void removePath();
    void display(UMat&, UMat&);
    void loadReplayFolder();
    void loadFrame(int frameIndex);
    void toggleReplayPlay();
    void mousePressEvent(QMouseEvent* event);
    void swapTrackingData(int firstObject, int secondObject, int from);
    void correctTracking();
signals:
    void newParameterList(const QMap<QString, QString> &);
    void next();
};

#endif // MAINWINDOW_H
