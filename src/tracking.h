/*
This file is part of Fast Track.

    FastTrack is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    FastTrack is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FastTrack.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef TRACKING_H
#define TRACKING_H
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <QDate>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QList>
#include <QMap>
#include <QObject>
#include <QProgressBar>
#include <QRandomGenerator>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QString>
#include <QTextStream>
#include <QThread>
#include <QTimer>
#include <QVector>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <numeric>
#include <opencv2/calib3d.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/video/tracking.hpp>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include "opencv2/features2d/features2d.hpp"
#include "videoreader.h"

using namespace cv;
using namespace std;

class Tracking : public QObject {
  Q_OBJECT

  QElapsedTimer *timer; /*!< Timer that measured the time during the analysis execution. */

  bool statusBinarisation; /*!< True if wite objects on dark background, flase otherwise. */

  string m_path;           /*!< Path to an image sequence. */
  string m_backgroundPath; /*!< Path to an image background. */
  UMat m_background;       /*!< Background image CV_8U. */
  int m_displayTime;       /*!< Binary image CV_8U. */
  QString m_savingPath;    /*!< Folder where to save files. */

  VideoReader *video;
  int m_im;                   /*!< Index of the next image to process in the m_files list. */
  QString m_error;            /*!< QString containing unreadable images. */
  int m_startImage;           /*!< Index of the next image to process in the m_files list. */
  int m_stopImage;            /*!< Index of the next image to process in the m_files list. */
  Rect m_ROI;                 /*!< Rectangular region of interest. */
  QFile m_logFile;            /*!< Path to the file where to save logs. */
  vector<cv::String> m_files; /*!< Vector containing the path for each image in the images sequence. */
  vector<int> m_id;           /*!< Vector containing the objets Id. */
  vector<int> m_lost;         /*!< Vector containing the lost objects. */
  int m_idMax;

  int param_n;                            /*!< Number of objects. */
  int param_maxArea;                      /*!< Maximal area of an object. */
  int param_minArea;                      /*!< Minimal area of an object. */
  int param_spot;                         /*!< Which spot parameters are used to computes the cost function. 0: head, 1: tail, 2: body. */
  double param_len;                       /*!< Maximal length travelled by an object between two images. */
  double param_angle;                     /*!< Maximal change in direction of an object between two images. */
  double param_area;                      /*!< Normalization area. */
  double param_perimeter;                 /*!< Normalization perimeter. */
  double param_lo;                        /*!< Maximal distance allowed by an object to travel during an occlusion event. */
  double param_to;                        /*!< Maximal time. */
  int param_thresh;                       /*!< Value of the threshold to binarize the image. */
  double param_nBackground;               /*!< Number of images to average to compute the background. */
  int param_methodBackground;             /*!< The method used to compute the background. */
  int param_methodRegistrationBackground; /*!< The method used to register the images for the background. */
  int param_registration;                 /*!< Method of registration. */
  int param_x1;                           /*!< Top x corner of the region of interest. */
  int param_y1;                           /*!< Top y corner of the region of interest. */
  int param_x2;                           /*!< Bottom x corner of the region of interest. */
  int param_y2;                           /*!< Bottom y corner of the region of interest. */
  int param_kernelSize;                   /*!< Size of the kernel of the morphological operation. */
  int param_kernelType;                   /*!< Type of the kernel of the morphological operation. */
  int param_morphOperation;               /*!< Type of the morphological operation. */
  QMap<QString, QString> parameters;      /*!< map of all the parameters for the tracking. */

 public:
  Tracking() = default;
  Tracking(string path, string background, int startImage = 0, int stopImage = -1);
  Tracking(string path, UMat background, int startImage = 0, int stopImage = -1);
  Tracking(const Tracking &) = delete;
  Tracking &operator=(const Tracking &) = delete;
  ~Tracking();

  const QString connectionName;
  Point2d curvatureCenter(const Point3d &tail, const Point3d &head) const;
  double curvature(Point2d center, const Mat &image) const;
  double divide(double a, double b) const;
  bool objectDirection(const UMat &image, vector<double> &information) const;
  vector<double> objectInformation(const UMat &image) const;
  vector<Point3d> reassignment(const vector<Point3d> &past, const vector<Point3d> &input, const vector<int> &assignment) const;
  vector<vector<Point3d>> objectPosition(const UMat &frame, int minSize, int maxSize) const;
  vector<int> costFunc(const vector<vector<Point3d>> &prevPos, const vector<vector<Point3d>> &pos, double LENGHT, double ANGLE, double LO, double AREA, double PERIMETER) const;
  void cleaning(const vector<int> &occluded, vector<int> &lostCounter, vector<int> &id, vector<vector<Point3d>> &input, double param_maximalTime) const;
  vector<Point3d> prevision(vector<Point3d> past, vector<Point3d> present) const;
  vector<int> findOcclusion(vector<int> assignment) const;
  static double modul(double angle);
  static double angleDifference(double alpha, double beta);
  static UMat backgroundExtraction(VideoReader &video, int n, const int method, const int registrationMethod);
  static void registration(UMat imageReference, UMat &frame, int method);
  static void binarisation(UMat &frame, char backgroundColor, int value);
  static bool exportTrackingResult(const QString path, QSqlDatabase db);
  static bool importTrackingResult(const QString path, QSqlDatabase db);

  UMat m_binaryFrame;                /*!< Binary image CV_8U */
  UMat m_visuFrame;                  /*!< Image 8 bit CV_8U */
  vector<vector<Point3d>> m_out;     /*!< Objects information at iteration minus one */
  vector<vector<Point3d>> m_outPrev; /*!< Objects information at current iteration */

 public slots:
  virtual void startProcess();
  void updatingParameters(const QMap<QString, QString> &);
  virtual void imageProcessing();

 signals:
  /**
   * @brief Emitted when an image is processed.
   * @param int Index of the processed image.
   */
  void progress(int) const;

  /**
   * @brief Emitted when an image to compute the background is processed.
   * @param int The number of processed image.
   */
  void backgroundProgress(int) const;

  /**
   * @brief Emitted when the first image has been processed to trigger the starting of the analysis.
   */
  void finishedProcessFrame() const;

  /**
   * @brief Emitted when all images have been processed.
   */
  void finished() const;

  /**
   * @brief Emitted when a crash occurs during the analysis.
   */
  void forceFinished(QString message) const;

  /**
   * @brief Emitted at the end of the analysis.
   */
  void statistic(long long int time) const;
};

#endif
