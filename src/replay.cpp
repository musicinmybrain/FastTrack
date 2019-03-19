#include "replay.h"
#include "ui_replay.h"

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

using namespace cv;
using namespace std;

/**
 * @class Replay
 *
 * @brief
 *
 * @author Benjamin Gallois
 *
 * @version $Revision: 4.1 $
 *
 * Contact: gallois.benjamin08@gmail.com
 *
*/

Replay::Replay(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Replay)
{
    ui->setupUi(this);

    QIcon img = QIcon(":/buttons/open.png");
        ui->replayOpen->setIcon(img);

        img = QIcon(":/buttons/play.png");
        ui->playReplay->setIcon(img);
        ui->playReplay->setIconSize(QSize(ui->playReplay->width(), ui->playReplay->height()));

        img = QIcon(":/buttons/open.png");
        ui->replayOpen->setIcon(img);
        ui->replayOpen->setIconSize(QSize(ui->replayOpen->width(), ui->replayOpen->height()));

        img = QIcon(":/buttons/next.png");
        ui->replayNext->setIcon(img);
        ui->replayNext->setIconSize(QSize(ui->replayNext->width(), ui->replayNext->height()));

        img = QIcon(":/buttons/previous.png");
        ui->replayPrevious->setIcon(img);
        ui->replayPrevious->setIconSize(QSize(ui->replayPrevious->width(), ui->replayPrevious->height()));

        img = QIcon(":/buttons/help.png");
        ui->replayHelp->setIcon(img);
        ui->replayHelp->setIconSize(QSize(ui->replayHelp->width(), ui->replayHelp->height()));

        img = QIcon(":/buttons/save.png");
        ui->replaySave->setIcon(img);
        ui->replaySave->setIconSize(QSize(ui->replaySave->width(), ui->replaySave->height()));

        img = QIcon(":/buttons/refresh.png");
        ui->replayRefresh->setIcon(img);
        ui->replayRefresh->setIconSize(QSize(ui->replayRefresh->width(), ui->replayRefresh->height()));

         // Keyboard shorcut
        // AZERTY keyboard shorcuts are set in the ui
        wShortcut = new QShortcut(QKeySequence("w"), this);
        connect(wShortcut, &QShortcut::activated, [this](){ ui->replayNext->animateClick(); });

        qShortcut = new QShortcut(QKeySequence("q"), this);
        connect(qShortcut, &QShortcut::activated, [this](){ ui->replaySlider->setValue(ui->replaySlider->value() - 1); });

        aShortcut = new QShortcut(QKeySequence("a"), this);
        connect(aShortcut, &QShortcut::activated, [this](){ ui->replaySlider->setValue(ui->replaySlider->value() - 1); });

        dShortcut = new QShortcut(QKeySequence("d"), this);
        connect(dShortcut, &QShortcut::activated, [this](){ ui->replaySlider->setValue(ui->replaySlider->value() + 1); });



        isReplayable = false;
        framerate = new QTimer();
        ui->ellipseBox->addItems({"Head + Tail", "Head", "Tail", "Body"});
        ui->arrowBox->addItems({"Head + Tail", "Head", "Tail", "Body"});

        connect(ui->replayOpen, &QPushButton::clicked, this, &Replay::openReplayFolder);
        connect(ui->replayPath, &QLineEdit::textChanged, this, &Replay::loadReplayFolder);
        connect(ui->replaySlider, &QSlider::valueChanged, this, &Replay::loadFrame);
        connect(ui->replaySlider, &QSlider::valueChanged, [this](const int &newValue) {
          ui->replayNumber->setText(QString::number(newValue));
        });
        connect(ui->replayRefresh, &QPushButton::clicked, [this]() {
          loadReplayFolder(ui->replayPath->text());
        });
        connect(ui->swapReplay, &QPushButton::clicked, this, &Replay::correctTracking);

        connect(framerate, &QTimer::timeout, [this]() {
          ui->replaySlider->setValue(autoPlayerIndex);
          autoPlayerIndex++;
          if (autoPlayerIndex % int(replayFrames.size()) != autoPlayerIndex) {
            autoPlayerIndex = 0;
          }
        });
        connect(ui->replayFps, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [this]() {
          if (isReplayable && ui->playReplay->isChecked()) {
            framerate->stop();
            framerate->start(1000/ui->replayFps->value());
          }
        });
        connect(ui->playReplay, &QPushButton::clicked, this, &Replay::toggleReplayPlay);

        connect(ui->replayNext, &QPushButton::clicked, this, &Replay::nextOcclusionEvent);
        connect(ui->replayPrevious, &QPushButton::clicked, this, &Replay::previousOcclusionEvent);

        connect(ui->replaySave, &QPushButton::clicked, this, &Replay::saveTrackedMovie);

        connect(ui->replayHelp, &QPushButton::clicked, [this]() {
              QMessageBox helpBox(this);
              helpBox.setIconPixmap(QPixmap(":/buttons/helpImg.png"));
              helpBox.exec();
        });

}

Replay::~Replay()
{
    delete ui;
}


/**
  * @brief Opens a dialogue to select a folder.
*/
void Replay::openReplayFolder() {
    QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"), memoryDir, QFileDialog::ShowDirsOnly);

    if(dir.right(15) == "Tracking_Result") {
      dir.truncate(dir.size() - 15);
    }

    memoryDir = dir;
    ui->replayPath->setText(dir + QDir::separator());
}


/**
  * @brief Loads a folder containing an image sequence and the tracking data if it exists. Triggerred when ui->pathButton is pressed.
  * @arg[in] dir Path to the folder where the image sequence is stored.
*/
void Replay::loadReplayFolder(QString dir) {

    // Delete existing data
    replayFrames.clear();
    occlusionEvents.clear();
    ui->object1Replay->clear();
    ui->object2Replay->clear();
    framerate->stop();
    object = true;
    if (dir.length()) {

        // Finds image format
        QList<QString> extensions = { "pgm", "png", "jpeg", "jpg", "tiff", "tif", "bmp", "dib", "jpe", "jp2", "webp", "pbm", "ppm", "sr", "ras", "tif" };
        QDirIterator it(dir, QStringList(), QDir::NoFilter);
        QString extension;
        while (it.hasNext()) {
          extension = it.next().section('.', -1);
          if( extensions.contains(extension) ) break;
        }

        try{

          // Gets the paths to all the frames in the folder and puts it in a vector.
          // Setups the ui by setting maximum and minimum of the slider bar.
          string path = (dir + QDir::separator() + "*." + extension).toStdString();
          glob(path, replayFrames, false); // Gets all the paths to frames
          ui->replaySlider->setMinimum(0);
          ui->replaySlider->setMaximum(replayFrames.size() - 1);
          Mat frame = imread(replayFrames.at(0), IMREAD_COLOR);
          originalImageSize.setWidth(frame.cols);
          originalImageSize.setHeight(frame.rows);
          isReplayable = true;

          trackingData = new Data(dir);

          // Generates a color map.
          // TO REDO
          double a,b,c;
          srand (time(NULL));
          for (int j = 0; j < 9000 ; ++j)  {
            a = rand() % 255;
            b = rand() % 255;
            c = rand() % 255;
            colorMap.push_back(Point3f(a, b, c));
          }
        loadFrame(0);
        }
        catch(...){
          isReplayable = false;
        }
    }
}



/**
  * @brief Displays the image and the tracking data in the ui->displayReplay. Triggered when the ui->replaySlider value is changed.
*/
void Replay::loadFrame(int frameIndex) {

    if ( isReplayable ) {

      ui->object1Replay->clear();
      ui->object2Replay->clear();

      Mat frame = imread(replayFrames.at(frameIndex), IMREAD_COLOR);
      int scale = ui->replaySize->value();

      // Takes the tracking data corresponding to the replayed frame and parse data to display
      QList<int> idList = trackingData->getId(frameIndex);
      for (auto const &a: idList) {

        QMap<QString, double> coordinate = trackingData->getData(frameIndex, a);
        int id = a;

        ui->object1Replay->addItem(QString::number(id));
        ui->object2Replay->addItem(QString::number(id));

        if (ui->replayEllipses->isChecked()) {

          switch(ui->ellipseBox->currentIndex()) {

            case 0 : // Head + Tail
              cv::ellipse(frame, Point( coordinate.value("xHead"), coordinate.value("yHead") ), Size( coordinate.value("headMajorAxisLength"), coordinate.value("headMinorAxisLength") ), 180 - (coordinate.value("tHead")*180)/M_PI, 0, 360,  Scalar(colorMap.at(id).x, colorMap.at(id).y, colorMap.at(id).z), scale, 8 );
              cv::ellipse(frame, Point( coordinate.value("xTail"), coordinate.value("yTail") ), Size( coordinate.value("tailMajorAxisLength"), coordinate.value("tailMinorAxisLength") ), 180 - (coordinate.value("tTail")*180)/M_PI, 0, 360,  Scalar(colorMap.at(id).x, colorMap.at(id).y, colorMap.at(id).z), scale, cv::LINE_AA );
              break;

            case 1 : // Head
              cv::ellipse(frame, Point( coordinate.value("xHead"), coordinate.value("yHead") ), Size( coordinate.value("headMajorAxisLength"), coordinate.value("headMinorAxisLength") ), 180 - (coordinate.value("tHead")*180)/M_PI, 0, 360,  Scalar(colorMap.at(id).x, colorMap.at(id).y, colorMap.at(id).z), scale, 8 );
              break;

            case 2 : // Tail
              cv::ellipse(frame, Point( coordinate.value("xTail"), coordinate.value("yTail") ), Size( coordinate.value("tailMajorAxisLength"), coordinate.value("tailMinorAxisLength") ), 180 - (coordinate.value("tTail")*180)/M_PI, 0, 360,  Scalar(colorMap.at(id).x, colorMap.at(id).y, colorMap.at(id).z), scale, cv::LINE_AA );
              break;

            case 3 : // Body
              cv::ellipse(frame, Point( coordinate.value("xBody"), coordinate.value("yBody") ), Size( coordinate.value("bodyMajorAxisLength"), coordinate.value("bodyMinorAxisLength") ), 180 - (coordinate.value("tBody")*180)/M_PI, 0, 360,  Scalar(colorMap.at(id).x, colorMap.at(id).y, colorMap.at(id).z), scale, 8 );
              break;
          }
        }

        if (ui->replayArrows->isChecked()) {

          switch(ui->arrowBox->currentIndex()) {

          case 0 :
            cv::arrowedLine(frame, Point(coordinate.value("xHead"), coordinate.value("yHead")), Point(coordinate.value("xHead") + coordinate.value("headMajorAxisLength")*cos(coordinate.value("tHead")), coordinate.value("yHead") - coordinate.value("headMajorAxisLength")*sin(coordinate.value("tHead"))), Scalar(colorMap.at(id).x, colorMap.at(id).y, colorMap.at(id).z), scale, cv::LINE_AA, 0, double(scale)/10);
            cv::arrowedLine(frame, Point(coordinate.value("xTail"), coordinate.value("yTail")), Point(coordinate.value("xTail") + coordinate.value("tailMajorAxisLength")*cos(coordinate.value("tTail")), coordinate.value("yTail") - coordinate.value("tailMajorAxisLength")*sin(coordinate.value("tTail"))), Scalar(colorMap.at(id).x, colorMap.at(id).y, colorMap.at(id).z), scale, cv::LINE_AA, 0, double(scale)/10);
            break;

          case 1 :
            cv::arrowedLine(frame, Point(coordinate.value("xHead"), coordinate.value("yHead")), Point(coordinate.value("xHead") + coordinate.value("headMajorAxisLength")*cos(coordinate.value("tHead")), coordinate.value("yHead") - coordinate.value("headMajorAxisLength")*sin(coordinate.value("tHead"))), Scalar(colorMap.at(id).x, colorMap.at(id).y, colorMap.at(id).z), scale, cv::LINE_AA, 0, double(scale)/10);
            break;

          case 2 :
            cv::arrowedLine(frame, Point(coordinate.value("xTail"), coordinate.value("yTail")), Point(coordinate.value("xTail") + coordinate.value("tailMajorAxisLength")*cos(coordinate.value("tTail")), coordinate.value("yTail") - coordinate.value("tailMajorAxisLength")*sin(coordinate.value("tTail"))), Scalar(colorMap.at(id).x, colorMap.at(id).y, colorMap.at(id).z), scale, cv::LINE_AA, 0, double(scale)/10);
            break;

          case 3 :
            cv::arrowedLine(frame, Point(coordinate.value("xBody"), coordinate.value("yBody")), Point(coordinate.value("xBody") + coordinate.value("bodyMajorAxisLength")*cos(coordinate.value("tBody")), coordinate.value("yBody") - coordinate.value("bodyMajorAxisLength")*sin(coordinate.value("tBody"))), Scalar(colorMap.at(id).x, colorMap.at(id).y, colorMap.at(id).z), scale, cv::LINE_AA, 0, double(scale)/10);
            break;

          }
        }

        if (ui->replayNumbers->isChecked()) {
          cv::putText(frame, to_string(id), Point(coordinate.value("xHead") + coordinate.value("headMajorAxisLength")*cos(coordinate.value("tHead")), coordinate.value("yHead") - coordinate.value("headMajorAxisLength")*sin(coordinate.value("tHead")) ), cv::FONT_HERSHEY_SIMPLEX, double(scale)*0.5, Scalar(colorMap.at(id).x, colorMap.at(id).y, colorMap.at(id).z), scale*1.2, cv::LINE_AA);
        }

        if (ui->replayTrace->isChecked()) {
          vector<Point> memory;
          for (int j = frameIndex - 50; j < frameIndex; j++) {
            if (j > 0) {
                QMap<QString, double> coordinate = trackingData->getData(j, a);
                if (coordinate.contains("xBody")) {
                  memory.push_back(Point(coordinate.value("xBody"), coordinate.value("yBody")));
                }
              }
          }
          cv::polylines(frame, memory, false, Scalar(colorMap.at(id).x, colorMap.at(id).y, colorMap.at(id).z), scale*1.2, cv::LINE_AA);
        }
      }

      double w = ui->replayDisplay->width();
      double h = ui->replayDisplay->height();
      QPixmap resizedPix = (QPixmap::fromImage(QImage(frame.data, frame.cols, frame.rows, frame.step, QImage::Format_RGB888)).scaled(w, h, Qt::KeepAspectRatio));
      ui->replayDisplay->setPixmap(resizedPix);
      resizedFrame.setWidth(resizedPix.width());
      resizedFrame.setHeight(resizedPix.height());
    }
}

/**
  * @brief Starts the autoplay of the replay. Triggered when ui->playreplay is clicked.
*/
void Replay::toggleReplayPlay() {

    if (isReplayable && ui->playReplay->isChecked()) {
      QIcon img(":/buttons/pause.png");
      ui->playReplay->setIcon(img);
      ui->playReplay->setIconSize(QSize(ui->playReplay->width(), ui->playReplay->height()));
      framerate->start(1000/ui->replayFps->value());
      autoPlayerIndex = ui->replaySlider->value();
    }
    else if (isReplayable && !ui->playReplay->isChecked()) {
      QIcon img(":/buttons/resume.png");
      ui->playReplay->setIcon(img);
      ui->playReplay->setIconSize(QSize(ui->playReplay->width(), ui->playReplay->height()));
      framerate->stop();
    }
}


/**
  * @brief Gets the mouse coordinate in the frame of reference of the widget where the user has clicked.
  * @param[in] event Describes the mouse event.
*/
void Replay::mousePressEvent(QMouseEvent* event) {

    // Left click event
    if (event->buttons() == Qt::LeftButton) {


      // Finds user click coordinate
      double xTop = ((double(ui->replayDisplay->mapFrom(this, event->pos()).x())- 0.5*( ui->replayDisplay->width() - resizedFrame.width()))*double(originalImageSize.width()))/double(resizedFrame.width()) ;
      double yTop = ((double(ui->replayDisplay->mapFrom(this, event->pos()).y()) - 0.5*( ui->replayDisplay->height() - resizedFrame.height()))*double(originalImageSize.height()))/double(resizedFrame.height()) ;


      // Finds the id of the closest object
      int frameIndex = ui->replaySlider->value();
      QList<int> idList = trackingData->getId(frameIndex);

      if ( !idList.isEmpty() ) {

        QVector<double> distance;
        for (auto const &a: idList) {

        QMap<QString, double> coordinate = trackingData->getData(frameIndex, a);
        distance.append( pow( coordinate.value("xBody") - xTop, 2 ) + pow( coordinate.value("yBody") - yTop, 2) );
      }

      // Finds the minimal distance and updates the UI
      int min = idList.at(std::min_element(distance.begin(), distance.end()) - distance.begin());
      if (object) {
        ui->object1Replay->setCurrentIndex(ui->object1Replay->findText(QString::number(min)));
        ui->object1Replay->setStyleSheet("QComboBox { background-color: rgb(" + QString::number(colorMap.at(min).x) + "," + QString::number(colorMap.at(min).y) + "," + QString::number(colorMap.at(min).z) + "); }");
        object = false;
      }
      else {
        ui->object2Replay->setCurrentIndex(ui->object2Replay->findText(QString::number(min)));
        ui->object2Replay->setStyleSheet("QComboBox { background-color: rgb(" + QString::number(colorMap.at(min).x) + "," + QString::number(colorMap.at(min).y) + "," + QString::number(colorMap.at(min).z) + "); }");
        object = true;
      }
    }
  }

  // Right click event
  else if (event->buttons() == Qt::RightButton) {
    ui->swapReplay->animateClick();
    ui->object1Replay->setStyleSheet("QComboBox { background-color: white; }");
    ui->object2Replay->setStyleSheet("QComboBox { background-color: white; }");
  }
}


/**
  * @brief Gets the index of the two selected objects, the start index, swaps the data from the start index to the end, and saves the new tracking data. Triggered when ui->swapButton is pressed or a right-click event is registered inside the replayDisplay.
*/
void Replay::correctTracking() {

    // Swaps the data
    int firstObject = ui->object1Replay->currentText().toInt();
    int secondObject = ui->object2Replay->currentText().toInt();
    int start = ui->replaySlider->value();
    trackingData->swapData(firstObject, secondObject, start);

    // Saves new tracking data
    trackingData->save();
    loadFrame(ui->replaySlider->value());
}


/**
  * @brief Finds and displays the next occlusion event on the ui->replayDisplay. Triggered when ui->nextReplay is pressed.
*/
void Replay::nextOcclusionEvent() {
    if( !occlusionEvents.isEmpty() ){
      int current = ui->replaySlider->value();
      int nextOcclusion = *std::upper_bound(occlusionEvents.begin(), occlusionEvents.end(), current);
      ui->replaySlider->setValue(nextOcclusion);
    }
}


/**
  * @brief Finds and displays the previous occlusion event on the ui->replayDisplay. Triggered when ui->previousReplay is pressed.
*/
void Replay::previousOcclusionEvent() {
    if( !occlusionEvents.isEmpty() ){
      int current = ui->replaySlider->value();
      int previousOcclusion = occlusionEvents.at(std::upper_bound(occlusionEvents.begin(), occlusionEvents.end(), current) - occlusionEvents.begin() - 2);
      ui->replaySlider->setValue(previousOcclusion);
    }
}


/**
  * @brief Saves the tracked movie in .avi. Triggered when ui->previousReplay is pressed.
*/
void Replay::saveTrackedMovie() {

    // If tracking data are available, gets the display settings and saves the movie in the
    // selected folder
    if( isReplayable ){
      QString savePath = QFileDialog::getSaveFileName(this, tr("Save File"), "/home/save.avi", tr("Videos (*.avi)"));
      cv::VideoWriter outputVideo(savePath.toStdString(), cv::VideoWriter::fourcc('M', 'J', 'P', 'G'), ui->replayFps->value(), Size(originalImageSize.width(), originalImageSize.height()));
      int scale = ui->replaySize->value();

     for(size_t frameIndex = 0; frameIndex < replayFrames.size(); frameIndex++) {
        Mat frame = imread(replayFrames.at(frameIndex), IMREAD_COLOR);
        // Takes the tracking data corresponding to the replayed frame and parse data to display
        // arrows on tracked objects.
      // Takes the tracking data corresponding to the replayed frame and parse data to display
      QList<int> idList = trackingData->getId(frameIndex);
      for (auto const &a: idList) {

        QMap<QString, double> coordinate = trackingData->getData(frameIndex, a);
        int id = a;

        ui->object1Replay->addItem(QString::number(id));
        ui->object2Replay->addItem(QString::number(id));

        if (ui->replayEllipses->isChecked()) {

          switch(ui->ellipseBox->currentIndex()) {

            case 0 : // Head + Tail
              cv::ellipse(frame, Point( coordinate.value("xHead"), coordinate.value("yHead") ), Size( coordinate.value("headMajorAxisLength"), coordinate.value("headMinorAxisLength") ), 180 - (coordinate.value("tHead")*180)/M_PI, 0, 360,  Scalar(colorMap.at(id).x, colorMap.at(id).y, colorMap.at(id).z), scale, 8 );
              cv::ellipse(frame, Point( coordinate.value("xTail"), coordinate.value("yTail") ), Size( coordinate.value("tailMajorAxisLength"), coordinate.value("tailMinorAxisLength") ), 180 - (coordinate.value("tTail")*180)/M_PI, 0, 360,  Scalar(colorMap.at(id).x, colorMap.at(id).y, colorMap.at(id).z), scale, cv::LINE_AA );
              break;

            case 1 : // Head
              cv::ellipse(frame, Point( coordinate.value("xHead"), coordinate.value("yHead") ), Size( coordinate.value("headMajorAxisLength"), coordinate.value("headMinorAxisLength") ), 180 - (coordinate.value("tHead")*180)/M_PI, 0, 360,  Scalar(colorMap.at(id).x, colorMap.at(id).y, colorMap.at(id).z), scale, 8 );
              break;

            case 2 : // Tail
              cv::ellipse(frame, Point( coordinate.value("xTail"), coordinate.value("yTail") ), Size( coordinate.value("tailMajorAxisLength"), coordinate.value("tailMinorAxisLength") ), 180 - (coordinate.value("tTail")*180)/M_PI, 0, 360,  Scalar(colorMap.at(id).x, colorMap.at(id).y, colorMap.at(id).z), scale, cv::LINE_AA );
              break;

            case 3 : // Body
              cv::ellipse(frame, Point( coordinate.value("xBody"), coordinate.value("yBody") ), Size( coordinate.value("bodyMajorAxisLength"), coordinate.value("bodyMinorAxisLength") ), 180 - (coordinate.value("tBody")*180)/M_PI, 0, 360,  Scalar(colorMap.at(id).x, colorMap.at(id).y, colorMap.at(id).z), scale, 8 );
              break;
          }
        }

        if (ui->replayArrows->isChecked()) {

          switch(ui->arrowBox->currentIndex()) {

          case 0 :
            cv::arrowedLine(frame, Point(coordinate.value("xHead"), coordinate.value("yHead")), Point(coordinate.value("xHead") + coordinate.value("headMajorAxisLength")*cos(coordinate.value("tHead")), coordinate.value("yHead") - coordinate.value("headMajorAxisLength")*sin(coordinate.value("tHead"))), Scalar(colorMap.at(id).x, colorMap.at(id).y, colorMap.at(id).z), scale, cv::LINE_AA, 0, double(scale)/10);
            cv::arrowedLine(frame, Point(coordinate.value("xTail"), coordinate.value("yTail")), Point(coordinate.value("xTail") + coordinate.value("tailMajorAxisLength")*cos(coordinate.value("tTail")), coordinate.value("yTail") - coordinate.value("tailMajorAxisLength")*sin(coordinate.value("tTail"))), Scalar(colorMap.at(id).x, colorMap.at(id).y, colorMap.at(id).z), scale, cv::LINE_AA, 0, double(scale)/10);
            break;

          case 1 :
            cv::arrowedLine(frame, Point(coordinate.value("xHead"), coordinate.value("yHead")), Point(coordinate.value("xHead") + coordinate.value("headMajorAxisLength")*cos(coordinate.value("tHead")), coordinate.value("yHead") - coordinate.value("headMajorAxisLength")*sin(coordinate.value("tHead"))), Scalar(colorMap.at(id).x, colorMap.at(id).y, colorMap.at(id).z), scale, cv::LINE_AA, 0, double(scale)/10);
            break;

          case 2 :
            cv::arrowedLine(frame, Point(coordinate.value("xTail"), coordinate.value("yTail")), Point(coordinate.value("xTail") + coordinate.value("tailMajorAxisLength")*cos(coordinate.value("tTail")), coordinate.value("yTail") - coordinate.value("tailMajorAxisLength")*sin(coordinate.value("tTail"))), Scalar(colorMap.at(id).x, colorMap.at(id).y, colorMap.at(id).z), scale, cv::LINE_AA, 0, double(scale)/10);
            break;

          case 3 :
            cv::arrowedLine(frame, Point(coordinate.value("xBody"), coordinate.value("yBody")), Point(coordinate.value("xBody") + coordinate.value("bodyMajorAxisLength")*cos(coordinate.value("tBody")), coordinate.value("yBody") - coordinate.value("bodyMajorAxisLength")*sin(coordinate.value("tBody"))), Scalar(colorMap.at(id).x, colorMap.at(id).y, colorMap.at(id).z), scale, cv::LINE_AA, 0, double(scale)/10);
            break;

          }
        }

        if (ui->replayNumbers->isChecked()) {
          cv::putText(frame, to_string(id), Point(coordinate.value("xHead") + coordinate.value("headMajorAxisLength")*cos(coordinate.value("tHead")), coordinate.value("yHead") - coordinate.value("headMajorAxisLength")*sin(coordinate.value("tHead")) ), cv::FONT_HERSHEY_SIMPLEX, double(scale)*0.5, Scalar(colorMap.at(id).x, colorMap.at(id).y, colorMap.at(id).z), scale*1.2, cv::LINE_AA);
        }

        if (ui->replayTrace->isChecked()) {
          vector<Point> memory;
          for (int j = frameIndex - 50; j < frameIndex; j++) {
            if (j > 0) {
                QMap<QString, double> coordinate = trackingData->getData(j, a);
                if (coordinate.contains("xBody")) {
                  memory.push_back(Point(coordinate.value("xBody"), coordinate.value("yBody")));
                }
              }
          }
          cv::polylines(frame, memory, false, Scalar(colorMap.at(id).x, colorMap.at(id).y, colorMap.at(id).z), scale*1.2, cv::LINE_AA);
        }
          
      }
      outputVideo.write(frame);
      ui->replaySlider->setValue(frameIndex);
      }
      outputVideo.release();
    }
}
