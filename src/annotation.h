#ifndef ANNOTATION_H
#define ANNOTATION_H

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QMap>
#include <QMapIterator>
#include <QMessageBox>
#include <QString>
#include <QTextStream>
#include <QUndoCommand>
#include <QWidget>

class Annotation : public QWidget {
  Q_OBJECT

 private:
  QFile *annotationFile;
  QMap<int, QString> *annotations;
  void writeToFile();
  QList<int> findIndexes;
  int findIndex;

 public slots:
  void clear();
  void write(int index, const QString &text);
  void read(int index);
  void find(const QString &expression);
  int next();
  int prev();

 signals:
  /**
   * @brief Emitted when a new annotation is read.
   * @param text Text of the requested annotation.
   */
  void annotationText(const QString &text);

 public:
  Annotation();
  Annotation(const QString &annotationFile);
  Annotation(const Annotation &T) = delete;
  Annotation &operator=(const Annotation &T) = delete;
  ~Annotation();
  bool setPath(const QString &annotationFile);
  bool isActive;
};

#endif
