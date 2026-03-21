#ifndef MBPOLL_DESKTOP_MAINWINDOW_H
#define MBPOLL_DESKTOP_MAINWINDOW_H

#include <QMainWindow>
#include "mbpoll-runtime.h"

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QSpinBox;
class QTableWidget;
class QTimer;
class QStackedWidget;

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow (QWidget * parent = nullptr);

private slots:
  void updateTransportUi ();
  void updateFormatUi ();
  void updateSummary ();
  void readOnce ();
  void writeOnce ();
  void startPolling ();
  void stopPolling ();
  void pollTick ();

private:
  enum TransportPage {
    TransportTcp = 0,
    TransportRtu = 1
  };

  void buildUi ();
  void populateFunctionItems ();
  void populateFormatItems ();
  void setStatusMessage (const QString & text, bool isError);
  void appendLog (const QString & text);
  void applyUiToContext (xMbPollContext * ctx) const;
  bool prepareReadContext (xMbPollContext * ctx, QString * error) const;
  bool prepareWriteContext (xMbPollContext * ctx, QString * error) const;
  bool parseWriteValues (xMbPollContext * ctx, QString * error) const;
  void renderReadResult (const xMbPollReadResult & result);
  int currentFunction () const;
  int currentFormat () const;
  bool writeSupported () const;

  QComboBox * modeCombo = nullptr;
  QStackedWidget * transportStack = nullptr;
  QLineEdit * tcpHostEdit = nullptr;
  QSpinBox * tcpPortSpin = nullptr;
  QLineEdit * rtuDeviceEdit = nullptr;
  QComboBox * baudrateCombo = nullptr;
  QComboBox * parityCombo = nullptr;
  QComboBox * databitsCombo = nullptr;
  QComboBox * stopbitsCombo = nullptr;
  QSpinBox * slaveSpin = nullptr;
  QComboBox * functionCombo = nullptr;
  QComboBox * formatCombo = nullptr;
  QSpinBox * startRefSpin = nullptr;
  QSpinBox * countSpin = nullptr;
  QDoubleSpinBox * timeoutSpin = nullptr;
  QSpinBox * pollRateSpin = nullptr;
  QCheckBox * bigEndianCheck = nullptr;
  QCheckBox * writeMultipleCheck = nullptr;
  QPlainTextEdit * writeValuesEdit = nullptr;
  QLabel * summaryLabel = nullptr;
  QLabel * typeLabel = nullptr;
  QLabel * statusLabel = nullptr;
  QTableWidget * resultTable = nullptr;
  QPlainTextEdit * logEdit = nullptr;
  QPushButton * readButton = nullptr;
  QPushButton * startPollButton = nullptr;
  QPushButton * stopPollButton = nullptr;
  QPushButton * writeButton = nullptr;
  QTimer * pollTimer = nullptr;
};

#endif /* MBPOLL_DESKTOP_MAINWINDOW_H */
