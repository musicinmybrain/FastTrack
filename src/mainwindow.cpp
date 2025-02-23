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

#include "mainwindow.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace std;

/**
 * @class MainWindow
 *
 * @brief The MainWindow class is derived from a QMainWindow widget. It displays the main window of the program.
 *
 * @author Benjamin Gallois
 *
 * @version $Revision: 4.0 $
 *
 * Contact: gallois.benjamin08@gmail.com
 *
 */

/**
 * @brief Constructs the MainWindow QObject and initializes the UI.
 */
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent),
                                          ui(new Ui::MainWindow) {
  QDir::setCurrent(QCoreApplication::applicationDirPath());
  ui->setupUi(this);
  setWindowTitle(qApp->applicationName() + " " + APP_VERSION);
  setWindowState(Qt::WindowActive);
  QTimer::singleShot(1500, this, &QMainWindow::showMaximized);  // Workaround to wait for a showEvent() that defines the screen geometry

  // Tray icon
  trayIcon = new QSystemTrayIcon(QIcon(":/assets/icon.svg"), this);
  QMenu *trayMenu = new QMenu;
  QAction *restore = new QAction(tr("Restore"));
  connect(restore, &QAction::triggered, this, &MainWindow::showNormal);
  trayMenu->addAction(restore);
  QAction *close = new QAction(tr("Close"));
  connect(close, &QAction::triggered, this, &MainWindow::close);
  trayMenu->addAction(close);
  connect(trayIcon, &QSystemTrayIcon::activated, [this](QSystemTrayIcon::ActivationReason reason) {
    switch (reason) {
      case QSystemTrayIcon::Trigger: {
        this->setVisible(this->isHidden());
        break;
      }
      case QSystemTrayIcon::DoubleClick: {
        break;
      }
      default:;
    }
  });
  trayIcon->setContextMenu(trayMenu);
  trayIcon->show();

  QNetworkAccessManager *manager = new QNetworkAccessManager(this);
  connect(manager, &QNetworkAccessManager::finished, [this](QNetworkReply *reply) {
    if (reply->error() != QNetworkReply::NoError) {
      return;
    }

    QByteArray downloadedData = reply->readAll();
    reply->deleteLater();

#ifdef Q_OS_UNIX
    QByteArray lastVersion = downloadedData.mid(downloadedData.indexOf("lin") + 4, 5);
#elif defined(Q_OS_WIN32)
    QByteArray lastVersion = downloadedData.mid(downloadedData.indexOf("win") + 4, 5);
#elif defined(Q_OS_MAC)
    QByteArray lastVersion = downloadedData.mid(downloadedData.indexOf("mac") + 4, 5);
#else
    QByteArray lastVersion = "unknown version";
#endif

    QByteArray message = downloadedData.mid(downloadedData.indexOf("message") + 7, downloadedData.indexOf("!message") - downloadedData.indexOf("message") - 7);
    QByteArray warning = downloadedData.mid(downloadedData.indexOf("warning") + 7, downloadedData.indexOf("!warning") - downloadedData.indexOf("warning") - 7);

    if (lastVersion != APP_VERSION) {
      QMessageBox msgBox;
      msgBox.setWindowTitle("FastTrack Version");
      msgBox.setTextFormat(Qt::RichText);
      msgBox.setIcon(QMessageBox::Information);
      msgBox.setText(QString("<strong>FastTrack version %1 is available!</strong> <br> Please update. <br> <a href='http://www.fasttrack.sh/docs/installation/#update'>Need help to update?</a> <br> %2").arg(lastVersion, message));
      msgBox.exec();
      this->statusBar()->addWidget(new QLabel(QString("FastTrack version %1 is available!").arg(lastVersion)));
    }
    if (!warning.isEmpty()) {
      QMessageBox msgBox;
      msgBox.setWindowTitle("Warning");
      msgBox.setTextFormat(Qt::RichText);
      msgBox.setIcon(QMessageBox::Warning);
      msgBox.setText(warning);
      msgBox.exec();
    }
  });

  manager->get(QNetworkRequest(QUrl("https://www.fasttrack.sh/download/FastTrack/platforms.txt")));

  interactive = new Interactive(this);
  ui->tabWidget->addTab(interactive, tr("Interactive tracking"));
  connect(interactive, &Interactive::status, [this](QString message) {
    trayIcon->showMessage("FastTrack", message, QSystemTrayIcon::Information, 3000);
  });
  connect(interactive, &Interactive::modeChanged, this, &MainWindow::setMode);

  batch = new Batch(this);
  ui->tabWidget->addTab(batch, tr("Batch tracking"));
  connect(batch, &Batch::status, [this](QString message) {
    trayIcon->showMessage("FastTrack", message, QSystemTrayIcon::Information, 3000);
  });

  replay = new Replay(this);
  ui->tabWidget->addTab(replay, tr("Tracking inspector"));

  trackingManager = new TrackingManager(this);
  ui->tabWidget->addTab(trackingManager, tr("Tracking Manager"));
  connect(interactive, &Interactive::log, trackingManager, &TrackingManager::addLogEntry);
  connect(batch, &Batch::log, trackingManager, &TrackingManager::addLogEntry);

#ifndef NO_WEB
  manual = new QWebEngineView(this);
  ui->tabWidget->addTab(manual, tr("User Manual"));
  connect(ui->tabWidget, &QTabWidget::currentChanged, [this](int index) {
    if (index == 4) {
      manual->setUrl(QUrl("https://www.fasttrack.sh/docs/intro"));
    }
  });
  dataset = new QWebEngineView(this);
  ui->tabWidget->addTab(dataset, tr("TD²"));
  connect(ui->tabWidget, &QTabWidget::currentChanged, [this](int index) {
    if (index == 5) {
      dataset->setUrl(QUrl("http://data.ljp.upmc.fr/datasets/TD2/"));
    }
  });

#endif

}  // Constructor

/**
 * @brief Close event reimplemented to ask confirmation before closing.
 */
void MainWindow::closeEvent(QCloseEvent *event) {
  QMessageBox msgBox(this);
  msgBox.setTextFormat(Qt::RichText);
  msgBox.setWindowTitle("Confirmation");
  msgBox.setText("<b>Are you sure you want to quit?</b>");
  msgBox.setIcon(QMessageBox::Question);
  msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
  QPushButton *minimizeButton = msgBox.addButton(tr("Minimize"), QMessageBox::AcceptRole);
  msgBox.setDefaultButton(minimizeButton);
  int reply = msgBox.exec();
  if (reply == QMessageBox::Yes) {
    event->accept();
  }
  else if (reply == QMessageBox::AcceptRole) {
    trayIcon->show();
    this->hide();
    trayIcon->showMessage(tr("Hey!"), tr("I'm there"), QIcon(":/assets/icon.svg"), 1500);
    event->ignore();
  }
  else {
    event->ignore();
  }
}

/**
 * @brief Changes the software mode.
 * @param[in] isExpert True if the software is in expert mode with advanced functions.
 */
void MainWindow::setMode(bool isExpert) {
  ui->tabWidget->tabBar()->setVisible(isExpert);
}

/**
 * @brief Destructs the MainWindow object and saves the previous set of parameters.
 */
MainWindow::~MainWindow() {
  delete ui;
}
