#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QTimer>
#include <QFile>
#include <QFileInfo>
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <string>
#include <functions.h>


using namespace cv;
using namespace std;


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setFixedSize(1600, 1000);
    setWindowTitle("Fishy Tracking");

    QMessageBox welcomeBox;
    welcomeBox.setText(" Welcome in Fishy Tracking " + version + "! \n To have help hove the relevant parameter. \n Check new version at https://bgallois.github.io/FishyTracking/. \n Have a wonderful tracking. \n © Benjamin GALLOIS benjamin.gallois@upmc.fr.");
    welcomeBox.exec();



    QStringList defParameters;
    QFile file("conf.txt");

    if(QFileInfo("conf.txt").exists()){
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            return;


        QTextStream in(&file);

        while(!in.atEnd()) {
            QString line = in.readLine();
            defParameters.append(line);
        }

        file.close();
    }

    else{
        int i = 0;
        while(i<15) {
            QString line = "0";
            defParameters.append(line);
            i++;
        }
    }


    QuitButton = new QPushButton("Quit", this);
    QuitButton->move(450, 320);
    QObject::connect(QuitButton, SIGNAL(clicked()), qApp, SLOT(quit()));

    ResetButton = new QPushButton("Reset", this);
    ResetButton->move(550, 320);
    QObject::connect(ResetButton, SIGNAL(clicked()), this, SLOT(Reset()));


    GoButton = new QPushButton("Go", this);
    GoButton->move(50, 320);
    QObject::connect(GoButton, SIGNAL(clicked()), this, SLOT(Go()));



    DefaultButton = new QPushButton("Set as default", this);
    DefaultButton->move(350, 320);
    QObject::connect(DefaultButton, SIGNAL(clicked()), this, SLOT(Write()));

    ReplayButton = new QPushButton("Replay", this);
    ReplayButton->move(250, 320);
    QObject::connect(ReplayButton, SIGNAL(clicked()), this, SLOT(Replay()));
    ReplayButton ->hide();


    normal = new QCheckBox("Normal", this);
    normal ->move(660, 310);
    normal ->setChecked(1);
    normal ->isChecked();
    normal ->setToolTip(tr("Check this option to display the original image with trajectories of tracked objects."));

    binary = new QCheckBox("Binary", this);
    binary ->move(660, 340);
    binary ->setToolTip(tr("Check this option to display the binary image. Useful to ajust the binary threshold parameter."));




    path = new QLabel(this);
    path->setText("Path:");
    path->move(50, 50);
    path->adjustSize();
    path ->setToolTip(tr("Path to the folder that contains images. Have to add /*extension at the end."));


    pathField = new QLineEdit(this);
    pathField ->move(250,50);
    pathField->setText(defParameters.at(0));
    pathField->adjustSize();


    numLabel = new QLabel(this);
    numLabel->setText("Number of objects:");
    numLabel->move(50, 100);
    numLabel->adjustSize();
    numLabel->setToolTip(tr("Number of objects to track, can't be changed during the tracking phase."));


    numField = new QLineEdit(this);
    numField ->move(250,100);
    numField->setText(defParameters.at(1));


    maxArea = new QLabel(this);
    maxArea->setText("Maximal area:");
    maxArea->move(50, 150);
    maxArea->adjustSize();
    maxArea->setToolTip(tr("Maximal area in px*px of objects to track."));

    maxAreaField = new QLineEdit(this);
    maxAreaField ->move(250,150);
    maxAreaField->setText(defParameters.at(2));


    minArea = new QLabel(this);
    minArea->setText("Minimal area:");
    minArea->move(50, 200);
    minArea->adjustSize();
    minArea->setToolTip(tr("Minimal area in px*px of objects to track."));


    minAreaField = new QLineEdit(this);
    minAreaField ->move(250,200);
    minAreaField->setText(defParameters.at(3));


    thresh = new QLabel(this);
    thresh->setText("Binary threshold:");
    thresh->move(50, 250);
    thresh->adjustSize();
    thresh->setToolTip(tr("Binary threshold, image[i,j] < thresh = 0, image[i,j] > thresh = 1."));


    threshField = new QLineEdit(this);
    threshField ->move(250,250);
    threshField->setText(defParameters.at(9));
    threshField->adjustSize();


    length = new QLabel(this);
    length->setText("Max displacement:");
    length->move(500, 50);
    length->adjustSize();
    length->setToolTip(tr("Maximal displacement possible of an object between two frames in px."));



    lengthField = new QLineEdit(this);
    lengthField ->move(700,50);
    lengthField->setText(defParameters.at(4));


    angle = new QLabel(this);
    angle->setText("Max angle:");
    angle->move(500, 100);
    angle->adjustSize();
    angle->setToolTip(tr("Maximal orientation change of an object between two frames in degree."));


    angleField = new QLineEdit(this);
    angleField ->move(700,100);
    angleField->setText(defParameters.at(5));



    lo = new QLabel(this);
    lo->setText("Max occlusion:");
    lo->move(500, 150);
    lo->adjustSize();
    lo->setToolTip(tr("Maximum distance in px that an object can move when it is occulted by another object."));



    loField = new QLineEdit(this);
    loField ->move(700,150);
    loField->setText(defParameters.at(6));


    w = new QLabel(this);
    w->setText("Weight:");
    w->move(500, 200);
    w->adjustSize();
    w->setToolTip(tr("Importance of orientation and distance for the tracking. Closer to 0 the tracking is more sensitive to the orientation of objects and closer to 1 is more sensitive to the distance."));



    wField = new QLineEdit(this);
    wField ->move(700,200);
    wField->setText(defParameters.at(7));


    nBack = new QLabel(this);
    nBack->setText("Background images:");
    nBack->move(500, 250);
    nBack->adjustSize();
    nBack->setToolTip(tr("Number of images to compute the background."));



    nBackField = new QLineEdit(this);
    nBackField ->move(700,250);
    nBackField->setText(defParameters.at(8));


    save = new QLabel(this);
    save->setText("Save to:");
    save->move(900, 50);
    save->adjustSize();
    save->setToolTip(tr("Path to a .txt file where the result of the tracking will be saved. Saved in the reference frame of the croped image."));


    saveField = new QLineEdit(this);
    saveField ->move(1200,50);
    saveField->setText(defParameters.at(10));



    x1ROI = new QLabel(this);
    x1ROI->setText("Top corner x position for the ROI:");
    x1ROI->move(900, 100);
    x1ROI->adjustSize();
    x1ROI->setToolTip(tr("Horizontale position of the top right corner of the region of interest."));



    x1ROIField = new QLineEdit(this);
    x1ROIField ->move(1200,100);
    x1ROIField->setText(defParameters.at(11));



    x2ROI = new QLabel(this);
    x2ROI->setText("Bottom corner x position for the ROI:");
    x2ROI->move(900, 150);
    x2ROI->adjustSize();
    x2ROI->setToolTip(tr("Horizontale position of the bottom left corner of the region of interest."));


    x2ROIField = new QLineEdit(this);
    x2ROIField ->move(1200,150);
    x2ROIField->setText(defParameters.at(12));



    y1ROI = new QLabel(this);
    y1ROI->setText("Top corner y position for the ROI:");
    y1ROI->move(900, 200);
    y1ROI->adjustSize();
    y1ROI->setToolTip(tr("Verticale position of the top right corner of the region of interest."));


    y1ROIField = new QLineEdit(this);
    y1ROIField ->move(1200,200);
    y1ROIField->setText(defParameters.at(13));



    y2ROI = new QLabel(this);
    y2ROI->setText("Bottom corner y position for the ROI:");
    y2ROI->move(900, 250);
    y2ROI->adjustSize();
    y2ROI->setToolTip(tr("Verticale position of the bottom left corner of the region of interest."));



    y2ROIField = new QLineEdit(this);
    y2ROIField ->move(1200,250);
    y2ROIField->setText(defParameters.at(14));


    arrow = new QLabel(this);
    arrow -> move(1400, 50);
    arrow -> setText("Arrow size:");


    arrowField = new QLCDNumber(this);
    arrowField ->move(1490,50);
    arrowField ->display(2);


    arrowSlider = new QSlider(Qt::Horizontal, this);
    arrowSlider ->setGeometry(10, 60, 100, 20);
    arrowSlider -> move(1400, 100);
    arrowSlider -> setMinimum(2);

    QObject::connect(arrowSlider, SIGNAL(valueChanged(int)), arrowField, SLOT(display(int))) ;

    fps = new QLabel(this);
    fps -> move(1400, 200);
    fps -> setText("FPS:");
    fps -> hide();


    fpsField = new QLCDNumber(this);
    fpsField ->move(1490,200);
    fpsField ->display(20);
    fpsField -> hide();


    fpsSlider = new QSlider(Qt::Horizontal, this);
    fpsSlider ->setGeometry(10, 60, 100, 20);
    fpsSlider -> move(1400, 250);
    fpsSlider -> setMinimum(1);
    fpsSlider -> setMaximum(300);
    fpsSlider-> hide();

    QObject::connect(fpsSlider, SIGNAL(valueChanged(int)), fpsField, SLOT(display(int)));

    trackingSpotLabel = new QLabel(this);
    trackingSpotLabel ->move(1400, 150);
    trackingSpotLabel ->setText("Spot to track:");

    trackingSpot = new QComboBox(this);
    trackingSpot ->move(1400, 180);
    trackingSpot ->addItem(tr("Body head"));
    trackingSpot ->addItem(tr("Body tail"));
    trackingSpot ->addItem(tr("Body center of mass"));
    trackingSpot ->adjustSize();



    display = new QLabel(this);
    display ->move(50, 400);
    display ->setScaledContents(true);
    display->adjustSize();



    display2 = new QLabel(this);
    display2 ->move(50, 400);
    display2 ->setScaledContents(true);
    display2->adjustSize();



    progressBar = new QProgressBar(this);
    progressBar ->move(50, 370);
    progressBar->setFixedWidth(1400);






    im = 0;
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(Go()));
    imr = 0;
    timerReplay = new QTimer(this);
    connect(timerReplay, SIGNAL(timeout()), this, SLOT(Replay()));



}

void MainWindow::Go(){

    // Get parameters
    spot = trackingSpot->currentIndex();
    arrowSize = arrowField-> value();
    MAXAREA = maxAreaField->text().toInt(); //9000; //minimal area of detected objects
    MINAREA = minAreaField->text().toInt(); //3000; //maximal area of detected objects
    LENGTH = lengthField->text().toInt(); //maximal distance covered between two frames
    ANGLE = (angleField->text().toDouble()*M_PI)/180; //maximal angle between two frames
    NUMBER = numField->text().toInt(); //number of objects to track
    WEIGHT = wField->text().toDouble();// weight of the orientation/angle in the cost function, 0 = orientation, 1 = distance
    LO = loField->text().toInt(); // maximum occlusion distance
    nBackground = nBackField->text().toInt();
    threshValue = threshField->text().toInt();
    savePath = saveField->text().toStdString();

    pathField->setDisabled(true);
    numField->setDisabled(true);
    nBackField->setDisabled(true);
    saveField->setDisabled(true);




    if(im == 0){ // Initialization

        timer->start(1);
        string folder = pathField->text().toStdString();
        glob(folder, files, false);
        sort(files.begin(), files.end());
        a = files.begin();
        string name = *a;
        progressBar ->setRange(0, files.size());
        cameraFrame = imread(name, IMREAD_GRAYSCALE);
        visu = imread(name);
        img0 = imread(name, IMREAD_GRAYSCALE);
        background = BackgroundExtraction(files, nBackground);
        vector<vector<Point> > tmp(NUMBER, vector<Point>());
        memory = tmp;
        colorMap = Color(NUMBER);

    }


    string name = *a;
    savefile.open(savePath);
    Rect ROI(x1ROIField->text().toInt(), y1ROIField->text().toInt(), x2ROIField->text().toInt() - x1ROIField->text().toInt(), y2ROIField->text().toInt() - y1ROIField->text().toInt());
    cameraFrame = imread(name, IMREAD_GRAYSCALE);
    visu = imread(name);
    cameraFrame.convertTo(cameraFrame, CV_8U);
    cameraFrame = background - cameraFrame;
    Binarisation(cameraFrame, 'b', threshValue);
    cameraFrame = cameraFrame(ROI);
    visu = visu(ROI);


    // Position computation
    out = ObjectPosition(cameraFrame, MINAREA, MAXAREA);


    if(im == 0){ // First frame initialization

        if(out.at(0).size() <= NUMBER + 1 && out.at(0).size() != 0){
            while((out.at(0).size() - NUMBER) > 0){
                out.at(0).push_back(Point3f(0, 0, 0));
                out.at(1).push_back(Point3f(0, 0, 0));
                out.at(2).push_back(Point3f(0, 0, 0));
                out.at(3).push_back(Point3f(0, 0, 0));
            }
            outPrev = out;
        }

        else if(out.at(0).size() == 0){ // Error message
            timer->stop();
            QMessageBox errorBox;
            msgBox.setText("No fish detected! Change parameters!");
            msgBox.exec();

        }
    }



    else if(im > 0){ // Matching fish identity with the previous frame
        identity = CostFunc(outPrev.at(spot), out.at(spot), LENGTH, ANGLE, WEIGHT, LO);
        out.at(0) = Reassignment(outPrev.at(0), out.at(0), identity);
        out.at(1) = Reassignment(outPrev.at(1), out.at(1), identity);
        out.at(2) = Reassignment(outPrev.at(2), out.at(2), identity);
        out.at(3) = Reassignment(outPrev.at(3), out.at(3), identity);
        out.at(0) = Prevision(outPrev.at(0), out.at(0));
        outPrev = out;
    }



    // Visualization & saving
   for(unsigned int l = 0; l < out.at(0).size(); l++){
        Point3f coord;
        coord = out.at(spot).at(l);
        //putText(visu, to_string(l), Point(coord.x, coord.y), FONT_HERSHEY_DUPLEX, .5, Scalar(colorMap.at(l).x, colorMap.at(l).y, colorMap.at(l).z), 1);
        //circle(visu, Point(coord.x, coord.y), 1, Scalar(colorMap.at(l).x, colorMap.at(l).y, colorMap.at(l).z), 2, 2, 0);
        arrowedLine(visu, Point(coord.x, coord.y), Point(coord.x + 5*arrowSize*cos(coord.z), coord.y - 5*arrowSize*sin(coord.z)), Scalar(colorMap.at(l).x, colorMap.at(l).y, colorMap.at(l).z), arrowSize, 10*arrowSize, 0);

        if((im > 5)){
            polylines(visu, memory.at(l), false, Scalar(colorMap.at(l).x, colorMap.at(l).y, colorMap.at(l).z), arrowSize, 8, 0);
            memory.at(l).push_back(Point((int)coord.x, (int)coord.y));
            if(im>50){
                memory.at(l).erase(memory.at(l).begin());
                }
        }



        // Saving
        internalSaving.push_back(coord);
        ofstream savefile;
        savefile.open(savePath, ios::out | ios::app );
        if(im == 0){
            savefile << "xHead" << "   " << "yHead" << "   " << "tHead" << "   "  << "xTail" << "   " << "yTail" << "   " << "tTail"   <<  "   " << "xBody" << "   " << "yBody" << "   " << "tBody"   <<  "   " << "curvature" <<  "   " << "imageNumber" << "\n";
        }

        else{
            savefile << out.at(0).at(l).x << "   " << out.at(0).at(l).y << "   " << out.at(0).at(l).z << "   "  << out.at(1).at(l).x << "   " << out.at(1).at(l).y << "   " << out.at(1).at(l).z  <<  "   " << out.at(2).at(l).x << "   " << out.at(2).at(l).y << "   " << out.at(2).at(l).z <<  "   " << out.at(3).at(l).x <<  "   " << im << "\n";
        }

    }


    if (normal->isChecked() && !binary->isChecked()){
        cvtColor(visu,visu,CV_BGR2RGB);
        Size size = visu.size();

        if(size.height > 600){
            display->setFixedHeight(600);
            display->setFixedWidth((600*size.height)/(size.width));
        }
        else if(size.width > 1500){
            display->setFixedWidth(1500);
            display->setFixedWidth((1500*size.width)/(size.height));
        }
        else{
            display->setFixedHeight(size.height);
            display->setFixedWidth(size.width);
        }

        display->setPixmap(QPixmap::fromImage(QImage(visu.data, visu.cols, visu.rows, visu.step, QImage::Format_RGB888)));
        display2->clear();
    }

    else if (!normal->isChecked() && binary->isChecked()){
        Size size = cameraFrame.size();

        if(size.height > 600){
            display2->setFixedHeight(600);
            display2->setFixedWidth((600*size.height)/(size.width));
        }
        else if(size.width > 1500){
            display2->setFixedWidth(1500);
            display2->setFixedWidth((1500*size.width)/(size.height));
        }
        else{
            display2->setFixedHeight(size.height);
            display2->setFixedWidth(size.width);
        }
        display2->move(50, 400);
        display2->setPixmap(QPixmap::fromImage(QImage(cameraFrame.data, cameraFrame.cols, cameraFrame.rows, cameraFrame.step, QImage::Format_Grayscale8)));
        display->clear();
    }

    else if (normal->isChecked() && binary->isChecked()){
        Size size = cameraFrame.size();

        if(size.height > 600){
            display->setFixedHeight(600);
            display->setFixedWidth((600*size.height)/(size.width));
        }
        else if(size.width > 1500){
            display->setFixedWidth(1500/2);
            display->setFixedWidth((1500*size.width)/(size.height*2));
        }
        else{
            display->setFixedHeight(size.height/2);
            display->setFixedWidth(size.width/2);
        }
        display2->move(800, 400);
        display2->setFixedHeight(display->height());
        display2->setFixedWidth(display->width());
        display2->setPixmap(QPixmap::fromImage(QImage(visu.data, visu.cols, visu.rows, visu.step, QImage::Format_RGB888)));
        display->setPixmap(QPixmap::fromImage(QImage(cameraFrame.data, cameraFrame.cols, cameraFrame.rows, cameraFrame.step, QImage::Format_Grayscale8)));
    }




    if(im < files.size() - 1){
        a ++;
        im ++;
        progressBar->setValue(im);
     }

    else{
        timer->stop();
        ReplayButton->show();
        fps ->show();
        fpsField ->show();
        fpsSlider->show();
        im = 0;
        QMessageBox msgBox;
        msgBox.setText("The tracking is done, go to work now!!! \n You can replay the tracking by clicking the replay button.");
        msgBox.exec();
    }

}




void MainWindow::Replay(){

    // Get parameters
    QString path = pathField->text();
    QString num = numField->text();

    int arrowSize = arrowField-> value();
    QString x1ROI = x1ROIField->text();
    QString x2ROI = x2ROIField->text();
    QString y1ROI = y1ROIField->text();
    QString y2ROI = y2ROIField->text();
    Rect ROI(x1ROI.toInt(), y1ROI.toInt(), x2ROI.toInt() - x1ROI.toInt(), y2ROI.toInt() - y1ROI.toInt());

    pathField->setDisabled(true);
    numField->setDisabled(true);
    maxAreaField->setDisabled(true);
     minAreaField->setDisabled(true);
    lengthField->setDisabled(true);
    angleField->setDisabled(true);
    loField->setDisabled(true);
    wField->setDisabled(true);
    nBackField->setDisabled(true);
    threshField->setDisabled(true);
    saveField->setDisabled(true);
    x1ROIField->setDisabled(true);
    x2ROIField->setDisabled(true);
    y1ROIField->setDisabled(true);
    y2ROIField->setDisabled(true);
    trackingSpot->hide();
    trackingSpotLabel->hide();




    // Convert parameters
    unsigned int NUMBER = num.toInt(); //number of objects to track
    int FPS = fpsField->value();
    FPS = int((100.)/(float(FPS)));







    if(imr == 0){ // Initialization

        timerReplay->start(FPS);
        a = files.begin();
        string name = *a;
        progressBar ->setRange(0, files.size());
        visu = imread(name);
        vector<vector<Point> > tmp(NUMBER, vector<Point>());
        memory = tmp;


    }


    string name = *a;
    visu = imread(name);
    cameraFrame.convertTo(cameraFrame, CV_8U);
    visu = visu(ROI);
    timerReplay ->setInterval(FPS);



    // Visualization & saving
   for(unsigned int l = 0; l < NUMBER; l++){
        Point3f coord;
        coord = internalSaving.at(imr*NUMBER + l);
        //putText(visu, to_string(l), Point(coord.x, coord.y), FONT_HERSHEY_DUPLEX, .5, Scalar(colorMap.at(l).x, colorMap.at(l).y, colorMap.at(l).z), 1);
        //circle(visu, Point(coord.x, coord.y), 1, Scalar(colorMap.at(l).x, colorMap.at(l).y, colorMap.at(l).z), 2, 2, 0);
        arrowedLine(visu, Point(coord.x, coord.y), Point(coord.x + 5*arrowSize*cos(coord.z), coord.y - 5*arrowSize*sin(coord.z)), Scalar(colorMap.at(l).x, colorMap.at(l).y, colorMap.at(l).z), arrowSize, 10*arrowSize, 0);

        if((imr > 5)){
            polylines(visu, memory.at(l), false, Scalar(colorMap.at(l).x, colorMap.at(l).y, colorMap.at(l).z), arrowSize, 8, 0);
            memory.at(l).push_back(Point((int)coord.x, (int)coord.y));
            if(imr>50){
                memory.at(l).erase(memory.at(l).begin());
                }
        }


    }



    cvtColor(visu,visu,CV_BGR2RGB);
    Size size = visu.size();

    if(size.height > 600){
        display->setFixedHeight(600);
        display->setFixedWidth((600*size.height)/(size.width));
    }
    else if(size.width > 1500){
        display->setFixedWidth(1500);
        display->setFixedWidth((1500*size.width)/(size.height));
    }
    else{
        display->setFixedHeight(size.height);
        display->setFixedWidth(size.width);
    }

    display->setPixmap(QPixmap::fromImage(QImage(visu.data, visu.cols, visu.rows, visu.step, QImage::Format_RGB888)));
    display2->clear();








    if(imr < files.size() - 1){
        a ++;
        imr ++;
        progressBar->setValue(im);
     }

    else{
        timerReplay->stop();
        imr = 0;
    }

}


void MainWindow::Write(){

    QString filename = "conf.txt";
    QFile file(filename);
    if(file.open(QIODevice::ReadWrite))
    {
        QTextStream stream( &file );
        stream << pathField->text()<< endl;
        stream << numField->text()<< endl;
        stream << maxAreaField->text()<< endl;
        stream << minAreaField->text()<< endl;
        stream << lengthField->text()<< endl;
        stream << angleField->text()<< endl;
        stream << loField->text()<< endl;
        stream << wField->text()<< endl;
        stream << nBackField->text()<< endl;
        stream << threshField->text()<< endl;
        stream << saveField->text()<< endl;
        stream << x1ROIField->text()<< endl;
        stream << x2ROIField->text()<< endl;
        stream << y1ROIField->text()<< endl;
        stream << y2ROIField->text()<< endl;
    }
}



void MainWindow::Reset()
{
    qDebug() << "Performing application reboot...";
    qApp->exit(MainWindow::EXIT_CODE_REBOOT );
}



MainWindow::~MainWindow()
{
    delete ui;
}

