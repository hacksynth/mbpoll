#include <QApplication>
#include <QFont>
#include <QString>
#include "version-git.h"
#include "mainwindow.h"

int
main (int argc, char * argv[]) {
  QApplication app (argc, argv);
  app.setApplicationName ("mbpoll Desktop");
  app.setApplicationVersion (QString::fromLatin1 (VERSION_SHORT));

  QFont font ("Noto Sans", 10);
  app.setFont (font);

  MainWindow window;
  window.show ();
  return app.exec ();
}
