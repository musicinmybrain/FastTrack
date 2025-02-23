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

#include "tracking.h"
#include "Hungarian.h"

using namespace cv;
using namespace std;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/**
 * @class Tracking
 *
 * @brief This class is intended to execute a tracking analysis on an image sequence. It is initialized with the path to the folder where images are stored. This class can be used inside an application by creating a new thread and calling the method startProcess. The tracking workflow can be changed by reimplementing the method startProcess and imageProcessing. This class can also be used as a library by constructing Tracking tracking("", "") to access the public class members and builds a new workflow.
 *
 * @author Benjamin Gallois
 *
 * @version $Revision: 4.0 $
 *
 * Contact: gallois.benjamin08@gmail.com
 *
 */

/**
 * @brief Computes the center of the curvature, defined as the intersection of the minor axis of the head ellipse with the minor axis of the tail ellipse of the object.
 * @param[in] tail The parameters of the tail ellipse: coordinate and direction of the major axis.
 * @param[in] head The parameters of the head ellipse: coordinate and direction of the major axis.
 * @return Coordinate of the curvature center.
 */
Point2d Tracking::curvatureCenter(const Point3d &tail, const Point3d &head) const {
  Point2d center;

  // Computes the equation of the slope of the two minors axis of each ellipse
  // from the coordinates and direction of each ellipse.
  // ie angle + pi*0.5
  // the y axis is pointing downward!
  Point2d p1 = Point2d(cos(tail.z + 0.5 * M_PI), -sin(tail.z + 0.5 * M_PI));
  Point2d p2 = Point2d(cos(head.z + 0.5 * M_PI), -sin(head.z + 0.5 * M_PI));

  double a = -p1.y / p1.x;
  double c = -p2.y / p2.x;

  // Solves the equation system by computing the determinant. If the determinant
  // is different of zeros, the two slopes intersect.
  if (abs(a - c) < 10E-10) {  // Determinant == 0, no unique solution, no intersection
    center = Point2d(0, 0);
  }
  else {  // Unique solution
    if (tail.z == 0 || tail.z == M_PI) {
      double d = head.y - c * head.x;
      center = Point2d(tail.x, c * tail.x + d);
    }
    else if (head.z == 0 || head.z == M_PI) {
      double b = tail.y - a * tail.x;
      center = Point2d(head.x, a * head.x + b);
    }
    else {
      double b = tail.y - a * tail.x;
      double d = head.y - c * head.x;
      center = Point2d((d - b) / (a - c), a * ((d + b) / (a - c)) + b);
    }
  }

  return center;
}

/**
 * @brief Computes the radius of curvature of the object defined as the inverse of the mean distance between each pixel of the object, and the center of the curvature. The center of curvature is defined as the intersection of the two minor axes of the head and tail ellipse.
 * @param[in] center Center of the curvature.
 * @param[in] image Binary image CV_8U.
 * @return Radius of curvature.
 */
double Tracking::curvature(Point2d center, const Mat &image) const {
  double d = 0;
  double count = 0;

  image.forEach<uchar>(
      [&d, &count, center](uchar &pixel, const int position[]) -> void {
        if (pixel == 255) {  // if inside object
          d += pow(pow(center.x - float(position[0]), 2) + pow(center.y - float(position[1]), 2), 0.5);
          count += 1;
        }
      });
  return count / d;
}

/**
 * @brief Computes the usual mathematical modulo 2*PI of an angle.
 * @param[in] angle Input angle.
 * @return Output angle.
 */
double Tracking::modul(double angle) {
  return angle - 2 * M_PI * floor(angle / (2 * M_PI));
}

/**
 * @brief Computes the float division and handle the division by 0 by returning 0.
 * @param[in] a Dividend.
 * @param[in] a Divisor.
 * @return Division result, 0 if b = 0.
 */
double Tracking::divide(double a, double b) const {
  if (b != 0) {
    return a / b;
  }
  else {
    return 0;
  }
}

/**
 * @brief Computes the least difference between two angles, alpha - beta. The difference is oriented in the trigonometric convention.
 * @param[in] alpha Input angle.
 * @param[in] beta Input angle.
 * @return Least difference.
 */
double Tracking::angleDifference(double alpha, double beta) {
  alpha = modul(alpha);
  beta = modul(beta);
  return -(modul(alpha - beta + M_PI) - M_PI);
}

/**
 * @brief Computes the equivalent ellipse of an object by computing the moments of the image. If the image is a circle, return nan as the orientation.
 * @param[in] image Binary image CV_8U.
 * @return The equivalent ellipse parameters: the object center of mass coordinate and its orientation.
 * @note: This function computes the object orientation, not its direction.
 */
vector<double> Tracking::objectInformation(const UMat &image) const {
  Moments moment = moments(image);

  double x = moment.m10 / moment.m00;
  double y = moment.m01 / moment.m00;

  double i = moment.mu20;
  double j = moment.mu11;
  double k = moment.mu02;

  double orientation = 0;
  if (i + j - k != 0) {
    orientation = (0.5 * atan((2 * j) / (i - k)) + (i < k) * (M_PI * 0.5));
    orientation += 2 * M_PI * (orientation < 0);
    orientation = (2 * M_PI - orientation);
  }

  double majAxis = 2 * pow((((i + k) + pow((i - k) * (i - k) + 4 * j * j, 0.5)) * 0.5) / moment.m00, 0.5);
  double minAxis = 2 * pow((((i + k) - pow((i - k) * (i - k) + 4 * j * j, 0.5)) * 0.5) / moment.m00, 0.5);

  return {x, y, orientation, majAxis, minAxis};
}

/**
 * @brief Computes the direction of the object from the object parameter (coordinate of the center of mass and orientation). To use this function, the object major axis has to be the horizontal axis of the image. Therefore, it is necessary to rotate the image before calling objectDirection.
 * @param[in] image Binary image CV_8U.
 * @param[in, out] information The parameters of the object (x coordinate, y coordinate, orientation).
 * @return True if the direction angle is the orientation angle. False if the direction angle is the orientation angle plus pi.
 */
bool Tracking::objectDirection(const UMat &image, vector<double> &information) const {
  // Computes the distribution of the image on the horizontal axis.

  vector<double> projection;
  reduce(image, projection, 0, REDUCE_SUM);

  vector<double> distribution;  // tmp
  distribution.reserve(projection.size());
  double ccMax = *max_element(projection.begin(), projection.end()) / 100;
  for (size_t it = 0; it < projection.size(); ++it) {
    int cc = static_cast<int>(projection[it]);
    for (int jt = 0; jt < cc / ccMax; ++jt) {
      distribution.push_back((double)(it + 1));
    }
  }

  double mean = accumulate(distribution.begin(), distribution.end(), 0) / double(distribution.size());

  double sd = 0, skew = 0;

  for (size_t it = 0; it < distribution.size(); ++it) {
    sd += pow(distribution[it] - mean, 2);
    skew += pow(distribution[it] - mean, 3);
  }

  sd = pow(sd / ((double)distribution.size() - 1), 0.5);
  skew *= (1 / (((double)distribution.size() - 1) * pow(sd, 3)));

  if (skew > 0) {
    information[2] = modul(information[2] - M_PI);
    return true;
  }
  return false;
}

/**
  * @brief Computes the background of an image sequence by averaging n images.
  * @param[in] VideoReader A VideoReader object containing the movie.
  * @param[in] n The number of images to average to computes the background.
  * @param[in] Method 0: minimal projection, 1: maximal projection, 2: average projection.
  * @param[in] registrationMethod Method of registration.
  * @param[in] isError Is a least one error as occured during the process.
  * @return The background image.
  TO DO: currently opening all the frames, to speed-up the process and if step is large can skip frames by replacing nextImage by getImage.
*/
UMat Tracking::backgroundExtraction(VideoReader &video, int n, const int method, const int registrationMethod) {
  int imageCount = video.getImageCount();
  if (n > imageCount) {
    n = imageCount;
  }

  UMat background;
  UMat img0;
  video.getImage(0, background);
  video.getImage(0, img0);
  if (background.channels() >= 3) {
    cvtColor(background, background, COLOR_BGR2GRAY);
    cvtColor(img0, img0, COLOR_BGR2GRAY);
  }
  background.convertTo(background, CV_32FC1);
  img0.convertTo(img0, CV_32FC1);

  UMat cameraFrameReg;
  video.getImage(0, cameraFrameReg);
  Mat H;
  int step = imageCount / n;
  int count = 1;
  int i = 1;

  while (i < imageCount) {
    if (i % step == 0) {
      if (!video.getNext(cameraFrameReg)) {
        throw std::runtime_error("Background computation error: image" + std::to_string(i) + " can not be read. The background was computed ignoring them.");
        i++;
        continue;
      }
      if (registrationMethod != 0) registration(img0, cameraFrameReg, registrationMethod - 1);
      if (cameraFrameReg.channels() >= 3) {
        cvtColor(cameraFrameReg, cameraFrameReg, COLOR_BGR2GRAY);
      }
      cameraFrameReg.convertTo(cameraFrameReg, CV_32FC1);
      switch (method) {
        case 0:
          cv::min(background, cameraFrameReg, background);
          break;

        case 1:
          cv::max(background, cameraFrameReg, background);
          break;

        case 2:
          accumulate(cameraFrameReg, background);
          break;
        default:
          cv::max(background, cameraFrameReg, background);
      }
      count++;
    }
    else {
      video.grab();
    }
    i++;
  }
  if (method == 2) {
    background.convertTo(background, CV_8U, 1. / count);
  }
  else {
    background.convertTo(background, CV_8U);
  }
  return background;
}

/**
 * @brief Register two images. To speed-up, the registration is made in a pyramidal way: the images are downsampled then registered to have a an approximate transformation then upslampled to have the precise transformation.
 * @param[in] imageReference The reference image for the registration.
 * @param[in, out] frame The image to register.
 * @param[in] method The method of registration: 0 = simple (phase correlation), 1 = ECC, 2 = Features based.
 */
void Tracking::registration(UMat imageReference, UMat &frame, const int method) {
  switch (method) {
    // Simple registration by phase correlation
    case 0: {
      frame.convertTo(frame, CV_32FC1);
      imageReference.convertTo(imageReference, CV_32FC1);

      // Downsamples the image to accelerate the registration
      vector<UMat> framesDownSampled;
      vector<UMat> imagesReferenceDownSampled;
      buildPyramid(frame, framesDownSampled, 4);
      buildPyramid(imageReference, imagesReferenceDownSampled, 4);

      for (size_t i = framesDownSampled.size(); i > 0; i--) {
        // Simple phase correlation registration
        Point2d shift = phaseCorrelate(framesDownSampled[i - 1], imagesReferenceDownSampled[i - 1]);
        Mat H = (Mat_<float>(2, 3) << 1.0, 0.0, shift.x, 0.0, 1.0, shift.y);
        warpAffine(framesDownSampled[i - 1], framesDownSampled[i - 1], H, framesDownSampled[i - 1].size());
      }
      frame.convertTo(frame, CV_8U);
      break;
    }
      // ECC images alignment
      // !!! This can throw an error if the algo do not converge
    case 1: {
      frame.convertTo(frame, CV_32FC1);
      imageReference.convertTo(imageReference, CV_32FC1);

      // Downsamples the image to accelerate the registration
      vector<UMat> framesDownSampled;
      vector<UMat> imagesReferenceDownSampled;
      buildPyramid(frame, framesDownSampled, 4);
      buildPyramid(imageReference, imagesReferenceDownSampled, 4);

      for (size_t i = framesDownSampled.size(); i > 0; i--) {
        // Simple phase correlation registration
        const int warpMode = MOTION_EUCLIDEAN;
        Mat warpMat = Mat::eye(2, 3, CV_32F);
        TermCriteria criteria(TermCriteria::COUNT + TermCriteria::EPS, 5000, 1e-5);
        findTransformECC(imagesReferenceDownSampled[i - 1], framesDownSampled[i - 1], warpMat, warpMode, criteria);
        // Gets the transformation from downsampled images
        warpAffine(framesDownSampled[i - 1], framesDownSampled[i - 1], warpMat, framesDownSampled[i - 1].size(), INTER_LINEAR + WARP_INVERSE_MAP);
      }
      frame.convertTo(frame, CV_8U);
      break;
    }
    // Features based registration
    // To do: make an extra argument to the function to add directly a descriptor set avoiding to recalculate the same if the image of reference doesn't change
    case 2: {
      frame.convertTo(frame, CV_8U);
      imageReference.convertTo(imageReference, CV_8U);

      vector<KeyPoint> keypointsFrame, keypointsRef;
      Mat descriptorsFrame, descriptorsRef;
      vector<DMatch> matches;
      const double featureNumber = 500;

      Ptr<Feature2D> orb = ORB::create(featureNumber);
      orb->detectAndCompute(frame, Mat(), keypointsFrame, descriptorsFrame);
      orb->detectAndCompute(imageReference, Mat(), keypointsRef, descriptorsRef);

      Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create("BruteForce-Hamming");
      matcher->match(descriptorsFrame, descriptorsRef, matches, Mat());

      vector<Point2f> pointsFrame, pointsRef;
      pointsFrame.reserve(featureNumber);
      pointsRef.reserve(featureNumber);
      for (size_t i = 0; i < matches.size(); i++) {
        pointsFrame.push_back(keypointsFrame[matches[i].queryIdx].pt);
        pointsRef.push_back(keypointsRef[matches[i].trainIdx].pt);
      }

      Mat h = findHomography(pointsFrame, pointsRef, RANSAC);
      warpPerspective(frame, frame, h, frame.size());

      frame.convertTo(frame, CV_8U);
      break;
    }
  }
}

/**
 * @brief Binarizes the image by thresholding.
 * @param[in, out] frame The image to binarize.
 * @param[in] backgroundColor If equals to 'w' the thresholded image will be inverted, if equal to 'b' it will not be inverted.
 * @param[in] value The value at which to threshold the image.
 */
void Tracking::binarisation(UMat &frame, char backgroundColor, int value) {
  frame.convertTo(frame, CV_8U);

  if (backgroundColor == 'b') {
    threshold(frame, frame, value, 255, THRESH_BINARY);
  }

  if (backgroundColor == 'w') {
    threshold(frame, frame, value, 255, THRESH_BINARY_INV);
  }
}

/**
 * @brief Computes the positions of the objects and extracts the object's features.
 * @param[in] frame Binary image CV_8U.
 * @param[in] minSize The minimal size of an object.
 * @param[in] maxSize: The maximal size of an object.
 * @return All the parameters of all the objects formated as follows: one vector, inside of this vector, four vectors for parameters of the head, tail, body and features with number of object size. {  { Point(xHead, yHead, thetaHead), ...}, Point({xTail, yTail, thetaHead), ...}, {Point(xBody, yBody, thetaBody), ...}, {Point(curvature, 0, 0), ...}}
 */
vector<vector<Point3d>> Tracking::objectPosition(const UMat &frame, int minSize, int maxSize) const {
  vector<vector<Point>> contours;
  vector<Point3d> positionHead;
  vector<Point3d> positionTail;
  vector<Point3d> positionFull;
  vector<Point3d> ellipseHead;
  vector<Point3d> ellipseTail;
  vector<Point3d> ellipseBody;
  vector<Point3d> globalParam;
  UMat dst;
  Rect roiFull, bbox;
  UMat RoiFull, RoiHead, RoiTail, rotate;
  Mat rotMatrix, p, pp;
  vector<double> parameter;
  vector<double> parameterHead;
  vector<double> parameterTail;
  Point2d radiusCurv;

  findContours(frame, contours, RETR_EXTERNAL, CHAIN_APPROX_NONE);

  size_t reserve = contours.size();
  positionHead.reserve(reserve);
  positionTail.reserve(reserve);
  positionFull.reserve(reserve);
  ellipseHead.reserve(reserve);
  ellipseTail.reserve(reserve);
  ellipseBody.reserve(reserve);
  globalParam.reserve(reserve);

  for (size_t i = 0; i < contours.size(); i++) {
    double a = contourArea(contours[i]);
    if (a > minSize && a < maxSize) {  // Only selects objects minArea << objectArea <<maxArea

      // Draws the object in a temporary black image to avoid selecting a
      // part of another object if two objects are very close.
      dst = UMat::zeros(frame.size(), CV_8U);
      drawContours(dst, contours, static_cast<int>(i), Scalar(255, 255, 255), FILLED, 8);

      // Computes the x, y and orientation of the object, in the
      // frame of reference of ROIFull image.
      roiFull = boundingRect(contours[i]);
      RoiFull = dst(roiFull);
      parameter = objectInformation(RoiFull);

      // Checks if the direction is defined. In the case of a perfect circle the direction can be computed and arbitrary set to 0
      if (parameter[2] != parameter[2]) {
        parameter[2] = 0;
      }

      // Rotates the image without cropping to have the object orientation as the x axis
      Point2d center = Point2d(0.5 * RoiFull.cols, 0.5 * RoiFull.rows);
      rotMatrix = getRotationMatrix2D(center, -(parameter[2] * 180) / M_PI, 1);
      bbox = RotatedRect(center, RoiFull.size(), static_cast<float>(-(parameter[2] * 180) / M_PI)).boundingRect();
      rotMatrix.at<double>(0, 2) += bbox.width * 0.5 - center.x;
      rotMatrix.at<double>(1, 2) += bbox.height * 0.5 - center.y;
      warpAffine(RoiFull, rotate, rotMatrix, bbox.size());

      // Computes the coordinate of the center of mass of the object in the rotated
      // image frame of reference.
      p = (Mat_<double>(3, 1) << parameter[0], parameter[1], 1);
      pp = rotMatrix * p;

      // Computes the direction of the object. If objectDirection return true, the
      // head is at the left and the tail at the right.
      Rect roiHead, roiTail;
      if (objectDirection(rotate, parameter)) {
        // Head ellipse. Parameters in the frame of reference of the RoiHead image.
        roiHead = Rect(0, 0, static_cast<int>(pp.at<double>(0, 0)), rotate.rows);
        RoiHead = rotate(roiHead);
        parameterHead = objectInformation(RoiHead);

        // Tail ellipse. Parameters in the frame of reference of ROITail image.
        roiTail = Rect(static_cast<int>(pp.at<double>(0, 0)), 0, static_cast<int>(rotate.cols - pp.at<double>(0, 0)), rotate.rows);
        RoiTail = rotate(roiTail);
        parameterTail = objectInformation(RoiTail);
      }
      else {
        // Head ellipse. Parameters in the frame of reference of the RoiHead image.
        roiHead = Rect(static_cast<int>(pp.at<double>(0, 0)), 0, static_cast<int>(rotate.cols - pp.at<double>(0, 0)), rotate.rows);
        RoiHead = rotate(roiHead);
        parameterHead = objectInformation(RoiHead);

        // Tail ellipse. Parameters in the frame of reference of RoiTail image.
        roiTail = Rect(0, 0, static_cast<int>(pp.at<double>(0, 0)), rotate.rows);
        RoiTail = rotate(roiTail);
        parameterTail = objectInformation(RoiTail);
      }

      // Gets all the parameters in the frame of reference of RoiFull image.
      invertAffineTransform(rotMatrix, rotMatrix);
      p = (Mat_<double>(3, 1) << parameterHead[0] + roiHead.tl().x, parameterHead[1] + roiHead.tl().y, 1);
      pp = rotMatrix * p;

      double xHead = pp.at<double>(0, 0) + roiFull.tl().x;
      double yHead = pp.at<double>(1, 0) + roiFull.tl().y;
      double angleHead = parameterHead[2] - M_PI * (parameterHead[2] > M_PI);
      angleHead = modul(angleHead + parameter[2] + M_PI * (abs(angleHead) > 0.5 * M_PI));  // Computes the direction

      p = (Mat_<double>(3, 1) << parameterTail[0] + roiTail.tl().x, parameterTail[1] + roiTail.tl().y, 1);
      pp = rotMatrix * p;
      double xTail = pp.at<double>(0, 0) + roiFull.tl().x;
      double yTail = pp.at<double>(1, 0) + roiFull.tl().y;
      double angleTail = parameterTail[2] - M_PI * (parameterTail[2] > M_PI);
      angleTail = modul(angleTail + parameter[2] + M_PI * (abs(angleTail) > 0.5 * M_PI));  // Computes the direction

      // Computes the curvature of the object
      double curv = 1. / 1e-16;
      radiusCurv = curvatureCenter(Point3d(xTail, yTail, angleTail), Point3d(xHead, yHead, angleHead));
      if (radiusCurv.x != NAN) {  //
        curv = curvature(radiusCurv, RoiFull.getMat(ACCESS_READ));
      }

      positionHead.push_back(Point3d(xHead + m_ROI.tl().x, yHead + m_ROI.tl().y, angleHead));
      positionTail.push_back(Point3d(xTail + m_ROI.tl().x, yTail + m_ROI.tl().y, angleTail));
      positionFull.push_back(Point3d(parameter[0] + roiFull.tl().x + m_ROI.tl().x, parameter[1] + roiFull.tl().y + m_ROI.tl().y, parameter[2]));
      ellipseHead.push_back(Point3d(parameterHead[3], parameterHead[4], pow(1 - (parameterHead[4] * parameterHead[4]) / (parameterHead[3] * parameterHead[3]), 0.5)));
      ellipseTail.push_back(Point3d(parameterTail[3], parameterTail[4], pow(1 - (parameterTail[4] * parameterTail[4]) / (parameterTail[3] * parameterTail[3]), 0.5)));
      ellipseBody.push_back(Point3d(parameter[3], parameter[4], pow(1 - (parameter[4] * parameter[4]) / (parameter[3] * parameter[3]), 0.5)));

      globalParam.push_back(Point3d(curv, a, arcLength(contours[i], true)));
    }

    else if (contourArea(contours[i]) >= maxSize && contourArea(contours[i]) < 3 * maxSize) {
    }
  }

  return {positionHead, positionTail, positionFull, globalParam, ellipseHead, ellipseTail, ellipseBody};
}

/**
 * @brief Computes a cost function and use a global optimization association to associate targets between images. Method adapted from: "An effective and robust method for Tracking multiple fish in video image based on fish head detection" YQ Chen et al. Uses the Hungarian method implemented by Cong Ma, 2016 "https://github.com/mcximing/hungarian-algorithm-cpp" adapted from the Matlab implementation by Markus Buehren "https://fr.mathworks.com/matlabcentral/fileexchange/6543-functions-for-the-rectangular-assignment-problem".
 * @param[in] prevPos The vector of objects parameters at the previous image.
 * @param[in] pos The vector of objects parameters at the current image that we want to sort in order to conserve objects identity.
 * @param[in] LENGTH The typical displacement of an object in pixels.
 * @param[in] ANGLE The typical reorientation angle in radians.
 * @param[in] LO The maximal assignment distance in pixels.
 * @return The assignment vector containing the new index position to sort the pos vector.
 */
vector<int> Tracking::costFunc(const vector<vector<Point3d>> &prevPos, const vector<vector<Point3d>> &pos, double LENGTH, double ANGLE, double LO, double AREA, double PERIMETER) const {
  int n = static_cast<int>(prevPos[0].size());
  int m = static_cast<int>(pos[0].size());
  vector<int> assignment;

  if (n == 0) {
    assignment = {};
  }
  else {
    vector<vector<double>> costMatrix(n, vector<double>(m));
    vector<pair<int, int>> distances;

    for (int i = 0; i < n; ++i) {  // Loop on previous objects
      Point3d prevCoord = prevPos[param_spot][i];
      Point3d prevData = prevPos[3][i];
      for (int j = 0; j < m; ++j) {  // Loop on current objects
        Point3d coord = pos[param_spot][j];
        Point3d data = pos[3][j];
        double distanceDiff = pow(pow(prevCoord.x - coord.x, 2) + pow(prevCoord.y - coord.y, 2), 0.5);
        double angleDiff = abs(angleDifference(prevCoord.z, coord.z));
        double areaDiff = abs(prevData.y - data.y);
        double perimeterDiff = abs(prevData.z - data.z);
        double c = -1;
        if (distanceDiff < LO) {
          c = divide(distanceDiff, LENGTH) + divide(angleDiff, ANGLE) + divide(areaDiff, AREA) + divide(perimeterDiff, PERIMETER);
          costMatrix[i][j] = c;
          distances.push_back({i, j});
        }
        else {
          costMatrix[i][j] = 2e307;
        }
      }
    }

    // Hungarian algorithm to solve the assignment problem O(n**3)
    HungarianAlgorithm HungAlgo;
    HungAlgo.Solve(costMatrix, assignment);

    // Finds object that are above the LO limit (+inf columns in the cost matrix
    // Puts the assignment number at -1 to signal new objects
    for (size_t i = 0; i < assignment.size(); i++) {
      pair<int, int> p = make_pair(i, assignment[i]);
      if (find(distances.begin(), distances.end(), p) == distances.end()) {
        assignment[i] = -1;
      }
    }
  }

  return assignment;
}

/**
 * @brief Finds the objects that are occluded during the tracking.
 * @param[in] assignment The vector with the new indexes that will be used to sort the input vector.
 * @return The vector with the indexes of occluded objects.
 */
vector<int> Tracking::findOcclusion(vector<int> assignment) const {
  vector<int> occlusion;
  vector<int>::iterator index = find(assignment.begin(), assignment.end(), -1);
  while (index != assignment.end()) {
    occlusion.push_back(static_cast<int>(index - assignment.begin()));
    index = find(index + 1, assignment.end(), -1);
  }
  return occlusion;
}

/**
 * @brief Sorts a vector accordingly to a new set of indexes. The sorted vector at index i is the input at index assignment[i].
 * @param[in] past The vector at the previous image.
 * @param[in] input The vector at current image of size m <= n to be sorted.
 * @param[in] assignment The vector with the new indexes that will be used to sort the input vector.
 * @param[in] lostCounter The vector with the number of times each objects are lost consecutively.
 * @param[in] id The vector with the id of the objects.
 * @return The sorted vector.
 */
vector<Point3d> Tracking::reassignment(const vector<Point3d> &past, const vector<Point3d> &input, const vector<int> &assignment) const {
  vector<Point3d> tmp = past;

  // Reassignes matched object
  for (unsigned int i = 0; i < past.size(); i++) {
    if (assignment[i] != -1) {
      tmp[i] = input[assignment[i]];
    }
  }

  // Adds the new objects
  bool stat;
  for (int j = 0; j < int(input.size()); j++) {
    stat = false;
    for (auto &a : assignment) {
      if (j == a) {
        stat = true;
        break;
      }
    }
    if (!stat) {
      tmp.push_back(input[j]);
    }
  }

  return tmp;
}

/**
 * @brief Cleans the data if an object is lost more than a certain time.
 * @param[in] occluded The vector with the index of object missing in the current image.
 * @param[in] input The vector at current image of size m <= n to be sorted.
 * @param[in] lostCounter The vector with the number of times each objects are lost consecutively.
 * @param[in] id The vector with the id of the objects.
 * @param[in] param_maximalTime
 * @return The sorted vector.
 */
void Tracking::cleaning(const vector<int> &occluded, vector<int> &lostCounter, vector<int> &id, vector<vector<Point3d>> &input, double param_maximalTime) const {
  vector<int> counter(lostCounter.size(), 0);

  // Increment the lost counter
  for (auto &a : occluded) {
    counter[a] = lostCounter[a] + 1;
  }

  // Cleans the data beginning at the start of the vector to keep index in place in the vectors
  for (size_t i = counter.size(); i > 0; i--) {
    if (counter.at(i - 1) > param_maximalTime) {
      counter.erase(counter.begin() + i - 1);
      id.erase(id.begin() + i - 1);
      for (size_t j = 0; j < input.size(); j++) {
        input[j].erase(input[j].begin() + i - 1);
      }
    }
  }

  lostCounter = counter;
}

/**
 * @brief Predicts the next position of an object from the previous position.
 * @param past The previous position parameters.
 * @param present: The current position parameters.
 * @return The predicted positions.
 */
vector<Point3d> Tracking::prevision(vector<Point3d> past, vector<Point3d> present) const {
  double l = 0;
  for (unsigned int i = 0; i < past.size(); i++) {
    if (past[i] != present[i]) {
      l = pow(pow(past[i].x - present[i].x, 2) + pow(past[i].y - present[i].y, 2), 0.5);
      break;
    }
  }

  for (unsigned int i = 0; i < past.size(); i++) {
    if (past[i] == present[i]) {
      present[i].x += l * cos(present[i].z);
      present[i].y -= l * sin(present[i].z);
    }
  }
  return present;
}

/**
 * @brief Processes an image from an images sequence and tracks and matchs objects according to the previous image in the sequence. Takes a new image from the image sequence, substracts the background, binarises the image and crops according to the defined region of interest. Detects all the objects in the image and extracts the object features. Then matches detected objects with objects from the previous frame. This function emits a signal to display the images in the user interface.
 */
void Tracking::imageProcessing() {
  QSqlDatabase outputDb = QSqlDatabase::database(connectionName);
  while (m_im < m_stopImage) {
    try {
      // Reads the next image in the image sequence and applies the image processing workflow
      if (!video->getNext(m_visuFrame)) {
        m_error += QString::number(m_im) + ", ";
        m_im++;
        emit(progress(m_im));
        continue;
      }
      if (param_registration != 0) {
        registration(m_background, m_visuFrame, param_registration - 1);
      }

      (statusBinarisation) ? (subtract(m_background, m_visuFrame, m_binaryFrame)) : (subtract(m_visuFrame, m_background, m_binaryFrame));
      binarisation(m_binaryFrame, 'b', param_thresh);

      if (param_kernelSize != 0 && param_morphOperation != 8) {
        Mat element = getStructuringElement(param_kernelType, Size(2 * param_kernelSize + 1, 2 * param_kernelSize + 1), Point(param_kernelSize, param_kernelSize));
        morphologyEx(m_binaryFrame, m_binaryFrame, param_morphOperation, element);
      }

      if (m_ROI.width != 0 || m_ROI.height != 0) {
        m_binaryFrame = m_binaryFrame(m_ROI);
        m_visuFrame = m_visuFrame(m_ROI);
      }

      // Detects the objects and extracts  parameters
      m_out = objectPosition(m_binaryFrame, param_minArea, param_maxArea);

      // Associates the objets with the previous image
      vector<int> identity = costFunc(m_outPrev, m_out, param_len, param_angle, param_lo, param_area, param_perimeter);
      vector<int> occluded = findOcclusion(identity);

      // Reassignes the m_out vector regarding the identities of the objects
      for (size_t i = 0; i < m_out.size(); i++) {
        m_out[i] = reassignment(m_outPrev[i], m_out[i], identity);
      }

      // Updates id and lost counter
      while (m_out[0].size() - m_id.size() != 0) {
        m_idMax++;
        m_id.push_back(m_idMax);
        m_lost.push_back(0);
      }

      // Save date in the database
      if (m_im % 50 == 0) {  // Performe the transaction every 50 frames to increase INSERT performance
        outputDb.commit();
        outputDb.transaction();
      }
      QSqlQuery query(outputDb);
      for (size_t l = 0; l < m_out[0].size(); l++) {
        // Tracking data are available
        if (find(occluded.begin(), occluded.end(), int(l)) == occluded.end()) {
          query.prepare(
              "INSERT INTO tracking (xHead, yHead, tHead, xTail, yTail, tTail, xBody, yBody, tBody, curvature, areaBody, perimeterBody, headMajorAxisLength, headMinorAxisLength, headExcentricity, tailMajorAxisLength, tailMinorAxisLength, tailExcentricity, bodyMajorAxisLength, bodyMinorAxisLength, bodyExcentricity, imageNumber, id) "
              "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
          for (auto const &a : m_out) {
            query.addBindValue(a[l].x);
            query.addBindValue(a[l].y);
            query.addBindValue(a[l].z);
          }
          query.addBindValue(m_im);
          query.addBindValue(m_id[l]);
          query.exec();
        }
      }

      cleaning(occluded, m_lost, m_id, m_out, param_to);
      m_outPrev = m_out;
      m_im++;
      emit(progress(m_im));
    }
    catch (const std::exception &e) {
      outputDb.commit();
      exportTrackingResult(m_savingPath, outputDb);
      delete timer;
      outputDb.close();
      m_logFile.close();
      qWarning() << QString::fromStdString(e.what()) << " at image " << QString::number(m_im);
      emit(forceFinished(QString::fromStdString(e.what()) + " error during the processing of the image " + QString::number(m_im)));
      return;
    }
    catch (...) {
      outputDb.commit();
      exportTrackingResult(m_savingPath, outputDb);
      delete timer;
      outputDb.close();
      m_logFile.close();
      qWarning() << "Unknown at image " << QString::number(m_im);
      emit(forceFinished("Fatal and unknown error during the processing of the image " + QString::number(m_im)));
      return;
    }
  }
  // Finished successfully
  outputDb.commit();
  bool isSaved = exportTrackingResult(m_savingPath, outputDb);
  outputDb.close();
  emit(statistic(timer->elapsed()));
  delete timer;

  if (isSaved && m_error.isEmpty()) {
    emit(finished());
  }
  else if (isSaved && !m_error.isEmpty()) {
    qWarning() << "Images" << m_error << " where skipped because unreadable";
    emit(forceFinished("Images " + m_error + " where skipped because unreadable."));
  }
  else {
    qWarning() << "Can't write tracking.txt to the disk";
    emit(forceFinished("Can't write tracking.txt to the disk!"));
  }
}

/**
 * @brief Constructs the tracking object from a path to an image sequence and an optional path to a background image.
 * @param[in] path The path to a folder where images are stocked.
 * @param[in] backgroundPath The path to a background image.
 * @param[in] startImage Index of the beginning image.
 * @param[in] stopImage Index of the ending image.
 */
Tracking::Tracking(string path, string backgroundPath, int startImage, int stopImage) : m_path(path), m_backgroundPath(backgroundPath), m_startImage(startImage), m_stopImage(stopImage), connectionName(QString("tracking_%1").arg(QRandomGenerator::global()->generate())) {
  video = new VideoReader(m_path);
}

/**
 * @brief Constructs the tracking object from a list of path, a background image and a range of image.
 * @param[in] imagePath List of path to the images.
 * @param[in] background A background image.
 * @param[in] startImage Index of the beginning image.
 * @param[in] stopImage Index of the ending image.
 */
Tracking::Tracking(string path, UMat background, int startImage, int stopImage) : m_path(path), m_background(background), m_startImage(startImage), m_stopImage(stopImage), connectionName(QString("tracking_%1").arg(QRandomGenerator::global()->generate())) {
  video = new VideoReader(m_path);
}

/**
 * @brief Initializes a tracking analysis and triggers its execution. Constructs from the path to a folder where the image sequence is stored, detects the image format and processes the first image to detect objects. First, it computes the background by averaging images from the sequence, then it subtracts  the background from the first image and then binarizes the resulting image. It detects the objects by contour analysis and extracts features by computing the object moments. It triggers the analysis of the second image of the sequence.
 */
void Tracking::startProcess() {
  try {
    if (!video->isOpened()) {
      throw std::runtime_error("Fatal error, the video can be opened");
    }

    timer = new QElapsedTimer();
    timer->start();
    m_im = m_startImage;
    (m_stopImage == -1) ? (m_stopImage = int(video->getImageCount())) : (m_stopImage = m_stopImage);

    // Loads the background image is provided and check if the image has the correct size
    if (m_background.empty() && m_backgroundPath.empty()) {
      m_background = backgroundExtraction(*video, static_cast<int>(param_nBackground), param_methodBackground, param_methodRegistrationBackground);
    }
    else if (m_background.empty()) {
      try {
        imread(m_backgroundPath, IMREAD_GRAYSCALE).copyTo(m_background);
        UMat test;
        video->getNext(test);
        subtract(test, m_background, test);
      }
      catch (...) {
        throw std::runtime_error("Select background image has the wrong size");
      }
    }

    // First frame
    video->getImage(m_im, m_visuFrame);

    (statusBinarisation) ? (subtract(m_background, m_visuFrame, m_binaryFrame)) : (subtract(m_visuFrame, m_background, m_binaryFrame));

    binarisation(m_binaryFrame, 'b', param_thresh);

    if (param_kernelSize != 0 && param_morphOperation != 8) {
      Mat element = getStructuringElement(param_kernelType, Size(2 * param_kernelSize + 1, 2 * param_kernelSize + 1), Point(param_kernelSize, param_kernelSize));
      morphologyEx(m_binaryFrame, m_binaryFrame, param_morphOperation, element);
    }

    if (m_ROI.width != 0) {
      m_binaryFrame = m_binaryFrame(m_ROI);
      m_visuFrame = m_visuFrame(m_ROI);
    }

    m_out = objectPosition(m_binaryFrame, param_minArea, param_maxArea);

    // Assigns an id and a counter at each object detected
    for (int i = 0; i < static_cast<int>(m_out[0].size()); i++) {
      m_id.push_back(i);
      m_lost.push_back(0);
    }

    if (!m_id.empty())
      m_idMax = int(*max_element(m_id.begin(), m_id.end()));
    else
      m_idMax = -1;

    //  Creates the folder to save result, parameter and background image
    //  If a folder already exist, renames it with the date and time.
    QFileInfo savingInfo(QString::fromStdString(m_path));
    QString savingFilename = savingInfo.baseName();
    m_savingPath = savingInfo.absolutePath();
    if (video->isSequence()) {
      m_savingPath.append(QString("/Tracking_Result"));
    }
    else {
      m_savingPath.append(QString("/Tracking_Result_") + savingFilename);
    }
    QDir r;
    r.rename(m_savingPath, m_savingPath + "_Archive-" + QDate::currentDate().toString("dd-MMM-yyyy-") + QTime::currentTime().toString("hh-mm-ss"));
    QDir().mkdir(m_savingPath);
    m_savingPath.append(QDir::separator());

    QSqlDatabase outputDb = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    outputDb.setDatabaseName(m_savingPath + "tracking.db");
    QSqlQuery query(outputDb);
    if (!outputDb.open()) {
      throw std::runtime_error("Can't write tracking.db to the disk!");
    }

    query.exec("CREATE TABLE parameter ( parameter TEXT, value REAL)");
    QFile parameterFile(m_savingPath + "cfg.toml");
    if (!parameterFile.open(QFile::WriteOnly | QFile::Text)) {
      throw std::runtime_error("Can't write cfg.toml to the disk!");
    }
    else {
      outputDb.transaction();
      QTextStream out(&parameterFile);
      out << "title = \"FastTrack cfg\"\"\n\n[parameters]\n";
      QList<QString> keyList = parameters.keys();
      for (auto a : keyList) {
        out << a << " = " << parameters.value(a) << Qt::endl;
        query.prepare("INSERT INTO parameter (parameter, value) VALUES (?, ?)");
        query.addBindValue(a);
        query.addBindValue(parameters.value(a));
        query.exec();
      }
      outputDb.commit();
    }

    imwrite(m_savingPath.toStdString() + "background.pgm", m_background);

    query.exec("PRAGMA synchronous=OFF");
    query.exec("CREATE TABLE tracking ( xHead REAL, yHead REAL, tHead REAL, xTail REAL, yTail REAL, tTail REAL, xBody REAL, yBody REAL, tBody REAL, curvature REAL, areaBody REAL, perimeterBody REAL, headMajorAxisLength REAL, headMinorAxisLength REAL, headExcentricity REAL, tailMajorAxisLength REAL, tailMinorAxisLength REAL, tailExcentricity REAL, bodyMajorAxisLength REAL, bodyMinorAxisLength REAL, bodyExcentricity REAL, imageNumber INTEGER, id INTEGER)");

    m_logFile.setFileName(m_savingPath + "log");
    if (!m_logFile.open(QFile::WriteOnly | QFile::Text)) {
      throw std::runtime_error("Can't write log to the disk!");
    }
    else {
      connect(this, &Tracking::forceFinished, [this](QString message) {
        QTextStream out(&m_logFile);
        out << QDate::currentDate().toString("dd-MMM-yyyy-") + QTime::currentTime().toString("hh-mm-ss") + '\t' + message.toLower() + '\n';
      });
    }

    // Saving
    outputDb.transaction();
    for (size_t l = 0; l < m_out[0].size(); l++) {
      query.prepare(
          "INSERT INTO tracking (xHead, yHead, tHead, xTail, yTail, tTail, xBody, yBody, tBody, curvature, areaBody, perimeterBody, headMajorAxisLength, headMinorAxisLength, headExcentricity, tailMajorAxisLength, tailMinorAxisLength, tailExcentricity, bodyMajorAxisLength, bodyMinorAxisLength, bodyExcentricity, imageNumber, id) "
          "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
      for (auto const &a : m_out) {
        query.addBindValue(a[l].x);
        query.addBindValue(a[l].y);
        query.addBindValue(a[l].z);
      }
      query.addBindValue(m_im);
      query.addBindValue(m_id[l]);
      query.exec();
    }
    m_outPrev = m_out;
    m_im++;
    connect(this, &Tracking::finishedProcessFrame, this, &Tracking::imageProcessing);

    emit(finishedProcessFrame());
  }
  catch (const std::runtime_error &e) {
    qWarning() << QString::fromStdString(e.what());
    emit(forceFinished(QString::fromStdString(e.what())));
  }
  catch (...) {
    qWarning() << "Fatal error, tracking initialization failed";
    emit(forceFinished("Fatal error, tracking initialization failed"));
  }
}

/**
 * @brief Updates the private members from the external parameters. This function links the tracking logic with the graphical user interface.
 * @param[in] parameterList The list of all the parameters used in the tracking.
 */
void Tracking::updatingParameters(const QMap<QString, QString> &parameterList) {
  parameters = parameterList;
  param_maxArea = parameterList.value("maxArea").toInt();
  param_minArea = parameterList.value("minArea").toInt();
  param_spot = parameterList.value("spot").toInt();
  param_len = parameterList.value("normDist").toDouble();
  param_angle = (M_PI * parameterList.value("normAngle").toDouble() / 180);
  param_lo = parameterList.value("maxDist").toDouble();
  param_to = parameterList.value("maxTime").toDouble();
  param_area = parameterList.value("normArea").toDouble();
  param_perimeter = parameterList.value("normPerim").toDouble();

  param_thresh = parameterList.value("thresh").toInt();
  param_nBackground = parameterList.value("nBack").toDouble();
  param_methodBackground = parameterList.value("methBack").toInt();
  param_methodRegistrationBackground = parameterList.value("regBack").toInt();
  param_x1 = parameterList.value("xTop").toInt();
  param_y1 = parameterList.value("yTop").toInt();
  param_x2 = parameterList.value("xBottom").toInt();
  param_y2 = parameterList.value("yBottom").toInt();
  m_ROI = Rect(param_x1, param_y1, param_x2 - param_x1, param_y2 - param_y1);
  param_registration = parameterList.value("reg").toInt();
  statusBinarisation = (parameterList.value("lightBack") == "0") ? true : false;
  param_morphOperation = parameterList.value("morph").toInt();
  param_kernelSize = parameterList.value("morphSize").toInt();
  param_kernelType = parameterList.value("morphType").toInt();
}

/**
 * @brief Destructs the tracking object.
 */
Tracking::~Tracking() {
  delete video;
  QSqlDatabase::removeDatabase(connectionName);
}

/**
 * @brief Exports the tracking data from the database to a text file.
 * @param[in] path The path to a folder where to write the text file.
 * @param[in] db The database where tracking results are stored already opened.
 */
bool Tracking::exportTrackingResult(const QString path, QSqlDatabase db) {
  QFile outputFile(path + "/tracking.txt");
  if (!outputFile.open(QFile::WriteOnly | QFile::Text)) {
    return false;
  }

  QTextStream savefile;
  savefile.setDevice(&outputFile);
  savefile << "xHead" << '\t' << "yHead" << '\t' << "tHead" << '\t' << "xTail" << '\t' << "yTail" << '\t' << "tTail" << '\t' << "xBody" << '\t' << "yBody" << '\t' << "tBody" << '\t' << "curvature" << '\t' << "areaBody" << '\t' << "perimeterBody" << '\t' << "headMajorAxisLength" << '\t' << "headMinorAxisLength" << '\t' << "headExcentricity" << '\t' << "tailMajorAxisLength" << '\t' << "tailMinorAxisLength" << '\t' << "tailExcentricity" << '\t' << "bodyMajorAxisLength" << '\t' << "bodyMinorAxisLength" << '\t' << "bodyExcentricity" << '\t' << "imageNumber" << '\t' << "id"
           << "\n";
  QSqlQuery query(db);
  query.prepare("SELECT xHead, yHead, tHead, xTail, yTail, tTail, xBody, yBody, tBody, curvature, areaBody, perimeterBody, headMajorAxisLength, headMinorAxisLength, headExcentricity, tailMajorAxisLength, tailMinorAxisLength, tailExcentricity, bodyMajorAxisLength, bodyMinorAxisLength, bodyExcentricity, imageNumber, id FROM tracking");
  query.exec();
  while (query.next()) {
    int size = 23;
    for (int i = 0; i < size; ++i) {
      savefile << query.value(i).toDouble();
      if (i != (size - 1)) {
        savefile << '\t';
      }
    }
    savefile << '\n';
  }
  savefile.flush();
  outputFile.close();
  return true;
}

/**
 * @brief Imports the tracking data from a text file to the database.
 * @param[in] path The path to a folder where to write the text file.
 * @param[in] db The database where tracking results are stored already opened.
 */
bool Tracking::importTrackingResult(const QString path, QSqlDatabase db) {
  QFile file(path + "/tracking.txt");
  if (!file.open(QFile::ReadOnly | QFile::Text)) {
    return false;
  }

  db.transaction();
  QSqlQuery query(db);
  query.exec("CREATE TABLE tracking ( xHead REAL, yHead REAL, tHead REAL, xTail REAL, yTail REAL, tTail REAL, xBody REAL, yBody REAL, tBody REAL, curvature REAL, areaBody REAL, perimeterBody REAL, headMajorAxisLength REAL, headMinorAxisLength REAL, headExcentricity REAL, tailMajorAxisLength REAL, tailMinorAxisLength REAL, tailExcentricity REAL, bodyMajorAxisLength REAL, bodyMinorAxisLength REAL, bodyExcentricity REAL, imageNumber INTEGER, id INTEGER)");

  QTextStream in(&file);
  QString line;
  QStringList values;
  in.readLineInto(&line);  // Ignore header
  while (in.readLineInto(&line)) {
    values = line.split("\t", Qt::SkipEmptyParts);
    query.prepare(
        "INSERT INTO tracking (xHead, yHead, tHead, xTail, yTail, tTail, xBody, yBody, tBody, curvature, areaBody, perimeterBody, headMajorAxisLength, headMinorAxisLength, headExcentricity, tailMajorAxisLength, tailMinorAxisLength, tailExcentricity, bodyMajorAxisLength, bodyMinorAxisLength, bodyExcentricity, imageNumber, id) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
    for (const auto &a : values) {
      query.addBindValue(a.toDouble());
    }
    query.exec();
  }
  db.commit();
  file.close();
  return true;
}
