#include "mainwindow.h"

#include <QAbstractItemView>
#include <QByteArray>
#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QSpinBox>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextOption>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <QStringList>
#include <QVariant>
#include "mbpoll-config.h"
#include "serial.h"
#include "version-git.h"

namespace {
const int kErrorBufferSize = 256;
}

MainWindow::MainWindow (QWidget * parent) :
  QMainWindow (parent) {

  buildUi ();
  populateFunctionItems ();
  populateFormatItems ();
  updateTransportUi ();
  updateFormatUi ();
  updateSummary ();
  setStatusMessage ("Ready", false);
  appendLog (QString ("Loaded mbpoll desktop MVP (%1).")
             .arg (QString::fromLatin1 (VERSION_SHORT)));
}

void
MainWindow::buildUi () {
  QWidget * central = new QWidget (this);
  auto * rootLayout = new QVBoxLayout (central);
  auto * topGrid = new QGridLayout ();
  auto * connectionBox = new QGroupBox ("Connection");
  auto * connectionLayout = new QFormLayout (connectionBox);
  auto * operationBox = new QGroupBox ("Operation");
  auto * operationLayout = new QFormLayout (operationBox);
  auto * writeBox = new QGroupBox ("Write Values");
  auto * writeLayout = new QVBoxLayout (writeBox);
  auto * outputBox = new QGroupBox ("Output");
  auto * outputLayout = new QVBoxLayout (outputBox);
  auto * buttonRow = new QHBoxLayout ();
  auto * summaryCard = new QGroupBox ("Current Request");
  auto * summaryLayout = new QVBoxLayout (summaryCard);

  setCentralWidget (central);
  setWindowTitle (QString ("mbpoll Desktop MVP %1")
                  .arg (QString::fromLatin1 (VERSION_SHORT)));
  resize (1180, 780);

  modeCombo = new QComboBox (this);
  modeCombo->addItem ("TCP", eModeTcp);
  modeCombo->addItem ("RTU", eModeRtu);

  transportStack = new QStackedWidget (this);

  auto * tcpPage = new QWidget (this);
  auto * tcpLayout = new QFormLayout (tcpPage);
  tcpHostEdit = new QLineEdit (tcpPage);
  tcpHostEdit->setPlaceholderText ("127.0.0.1");
  tcpPortSpin = new QSpinBox (tcpPage);
  tcpPortSpin->setRange (TCP_PORT_MIN, TCP_PORT_MAX);
  tcpPortSpin->setValue (502);
  tcpLayout->addRow ("Host", tcpHostEdit);
  tcpLayout->addRow ("Port", tcpPortSpin);
  transportStack->addWidget (tcpPage);

  auto * rtuPage = new QWidget (this);
  auto * rtuLayout = new QFormLayout (rtuPage);
  rtuDeviceEdit = new QLineEdit (rtuPage);
  rtuDeviceEdit->setPlaceholderText ("/dev/ttyUSB0");
  baudrateCombo = new QComboBox (rtuPage);
  baudrateCombo->setEditable (true);
  baudrateCombo->addItems ({"1200", "2400", "4800", "9600", "19200", "38400",
                            "57600", "115200"});
  baudrateCombo->setCurrentText ("19200");
  parityCombo = new QComboBox (rtuPage);
  parityCombo->addItem ("Even", SERIAL_PARITY_EVEN);
  parityCombo->addItem ("Odd", SERIAL_PARITY_ODD);
  parityCombo->addItem ("None", SERIAL_PARITY_NONE);
  databitsCombo = new QComboBox (rtuPage);
  databitsCombo->addItem ("8", SERIAL_DATABIT_8);
  databitsCombo->addItem ("7", SERIAL_DATABIT_7);
  stopbitsCombo = new QComboBox (rtuPage);
  stopbitsCombo->addItem ("1", SERIAL_STOPBIT_ONE);
  stopbitsCombo->addItem ("2", SERIAL_STOPBIT_TWO);
  rtuLayout->addRow ("Device", rtuDeviceEdit);
  rtuLayout->addRow ("Baudrate", baudrateCombo);
  rtuLayout->addRow ("Parity", parityCombo);
  rtuLayout->addRow ("Databits", databitsCombo);
  rtuLayout->addRow ("Stopbits", stopbitsCombo);
  transportStack->addWidget (rtuPage);

  slaveSpin = new QSpinBox (this);
  slaveSpin->setRange (0, SLAVEADDR_MAX);
  slaveSpin->setValue (DEFAULT_SLAVEADDR);

  functionCombo = new QComboBox (this);
  formatCombo = new QComboBox (this);

  startRefSpin = new QSpinBox (this);
  startRefSpin->setRange (STARTREF_MIN, STARTREF_MAX);
  startRefSpin->setValue (DEFAULT_STARTREF);

  countSpin = new QSpinBox (this);
  countSpin->setRange (NUMOFVALUES_MIN, NUMOFVALUES_MAX);
  countSpin->setValue (DEFAULT_NUMOFVALUES);

  timeoutSpin = new QDoubleSpinBox (this);
  timeoutSpin->setRange (TIMEOUT_MIN, TIMEOUT_MAX);
  timeoutSpin->setSingleStep (0.1);
  timeoutSpin->setDecimals (2);
  timeoutSpin->setValue (DEFAULT_TIMEOUT);

  pollRateSpin = new QSpinBox (this);
  pollRateSpin->setRange (POLLRATE_MIN, 600000);
  pollRateSpin->setValue (DEFAULT_POLLRATE);

  bigEndianCheck = new QCheckBox ("Big-endian 32-bit words", this);
  writeMultipleCheck = new QCheckBox ("Use write-multiple for single register", this);

  connectionLayout->addRow ("Mode", modeCombo);
  connectionLayout->addRow ("Transport", transportStack);
  connectionLayout->addRow ("Slave address", slaveSpin);

  operationLayout->addRow ("Function", functionCombo);
  operationLayout->addRow ("Type", formatCombo);
  operationLayout->addRow ("Start reference", startRefSpin);
  operationLayout->addRow ("Count", countSpin);
  operationLayout->addRow ("Timeout (s)", timeoutSpin);
  operationLayout->addRow ("Poll rate (ms)", pollRateSpin);
  operationLayout->addRow (bigEndianCheck);
  operationLayout->addRow (writeMultipleCheck);

  writeValuesEdit = new QPlainTextEdit (this);
  writeValuesEdit->setPlaceholderText ("Examples: 1 or 0,1,1 for coils; 123 or 12,34 for holding registers");
  writeValuesEdit->setMaximumBlockCount (6);
  writeValuesEdit->setWordWrapMode (QTextOption::WrapAnywhere);
  writeLayout->addWidget (writeValuesEdit);

  readButton = new QPushButton ("Read Once", this);
  startPollButton = new QPushButton ("Start Polling", this);
  stopPollButton = new QPushButton ("Stop Polling", this);
  writeButton = new QPushButton ("Write", this);
  stopPollButton->setEnabled (false);

  buttonRow->addWidget (readButton);
  buttonRow->addWidget (startPollButton);
  buttonRow->addWidget (stopPollButton);
  buttonRow->addStretch ();
  buttonRow->addWidget (writeButton);

  summaryLabel = new QLabel (this);
  summaryLabel->setWordWrap (true);
  typeLabel = new QLabel (this);
  typeLabel->setWordWrap (true);
  statusLabel = new QLabel (this);
  statusLabel->setWordWrap (true);
  summaryLayout->addWidget (summaryLabel);
  summaryLayout->addWidget (typeLabel);
  summaryLayout->addWidget (statusLabel);

  resultTable = new QTableWidget (0, 2, this);
  resultTable->setHorizontalHeaderLabels ({"Reference", "Value"});
  resultTable->horizontalHeader ()->setStretchLastSection (true);
  resultTable->horizontalHeader ()->setSectionResizeMode (0, QHeaderView::ResizeToContents);
  resultTable->verticalHeader ()->setVisible (false);
  resultTable->setAlternatingRowColors (true);
  resultTable->setEditTriggers (QAbstractItemView::NoEditTriggers);
  resultTable->setSelectionBehavior (QAbstractItemView::SelectRows);

  logEdit = new QPlainTextEdit (this);
  logEdit->setReadOnly (true);
  logEdit->setMaximumBlockCount (500);

  outputLayout->addWidget (resultTable, 2);
  outputLayout->addWidget (new QLabel ("Operation Log", this));
  outputLayout->addWidget (logEdit, 1);

  topGrid->addWidget (connectionBox, 0, 0);
  topGrid->addWidget (operationBox, 0, 1);
  topGrid->addWidget (summaryCard, 1, 0, 1, 2);
  topGrid->setColumnStretch (0, 1);
  topGrid->setColumnStretch (1, 1);

  rootLayout->addLayout (topGrid);
  rootLayout->addWidget (writeBox);
  rootLayout->addLayout (buttonRow);
  rootLayout->addWidget (outputBox, 1);

  statusBar ()->showMessage ("Ready");

  pollTimer = new QTimer (this);

  connect (modeCombo, qOverload<int> (&QComboBox::currentIndexChanged),
           this, &MainWindow::updateTransportUi);
  connect (functionCombo, qOverload<int> (&QComboBox::currentIndexChanged),
           this, &MainWindow::updateFormatUi);
  connect (formatCombo, qOverload<int> (&QComboBox::currentIndexChanged),
           this, &MainWindow::updateSummary);
  connect (bigEndianCheck, &QCheckBox::toggled,
           this, &MainWindow::updateSummary);
  connect (writeMultipleCheck, &QCheckBox::toggled,
           this, &MainWindow::updateSummary);
  connect (tcpHostEdit, &QLineEdit::textChanged,
           this, &MainWindow::updateSummary);
  connect (rtuDeviceEdit, &QLineEdit::textChanged,
           this, &MainWindow::updateSummary);
  connect (baudrateCombo, &QComboBox::currentTextChanged,
           this, &MainWindow::updateSummary);
  connect (parityCombo, qOverload<int> (&QComboBox::currentIndexChanged),
           this, &MainWindow::updateSummary);
  connect (databitsCombo, qOverload<int> (&QComboBox::currentIndexChanged),
           this, &MainWindow::updateSummary);
  connect (stopbitsCombo, qOverload<int> (&QComboBox::currentIndexChanged),
           this, &MainWindow::updateSummary);
  connect (slaveSpin, qOverload<int> (&QSpinBox::valueChanged),
           this, &MainWindow::updateSummary);
  connect (tcpPortSpin, qOverload<int> (&QSpinBox::valueChanged),
           this, &MainWindow::updateSummary);
  connect (startRefSpin, qOverload<int> (&QSpinBox::valueChanged),
           this, &MainWindow::updateSummary);
  connect (countSpin, qOverload<int> (&QSpinBox::valueChanged),
           this, &MainWindow::updateSummary);
  connect (timeoutSpin, qOverload<double> (&QDoubleSpinBox::valueChanged),
           this, &MainWindow::updateSummary);
  connect (pollRateSpin, qOverload<int> (&QSpinBox::valueChanged), this, [this] {
    if (pollTimer->isActive ()) {
      pollTimer->start (pollRateSpin->value ());
    }
    updateSummary ();
  });

  connect (readButton, &QPushButton::clicked,
           this, &MainWindow::readOnce);
  connect (writeButton, &QPushButton::clicked,
           this, &MainWindow::writeOnce);
  connect (startPollButton, &QPushButton::clicked,
           this, &MainWindow::startPolling);
  connect (stopPollButton, &QPushButton::clicked,
           this, &MainWindow::stopPolling);
  connect (pollTimer, &QTimer::timeout,
           this, &MainWindow::pollTick);

  setStyleSheet (
    "QMainWindow { background: #f5f1e8; }"
    "QGroupBox { background: #fffdf8; border: 1px solid #d9cfbc; border-radius: 10px; margin-top: 12px; padding-top: 10px; font-weight: 600; }"
    "QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 4px; color: #27545c; }"
    "QLabel { color: #213547; }"
    "QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox, QPlainTextEdit, QTableWidget { background: #ffffff; border: 1px solid #c9bea8; border-radius: 6px; padding: 4px; }"
    "QPushButton { background: #27545c; color: white; border-radius: 8px; padding: 8px 14px; }"
    "QPushButton:disabled { background: #a1aab0; }"
    "QTableWidget { alternate-background-color: #f3f6f7; }"
  );
}

void
MainWindow::populateFunctionItems () {

  functionCombo->addItem ("Coil", eFuncCoil);
  functionCombo->addItem ("Discrete Input", eFuncDiscreteInput);
  functionCombo->addItem ("Input Register", eFuncInputReg);
  functionCombo->addItem ("Holding Register", eFuncHoldingReg);
  functionCombo->setCurrentIndex (3);
}

void
MainWindow::populateFormatItems () {

  formatCombo->clear ();
  formatCombo->addItem ("Binary", eFormatBin);
  formatCombo->addItem ("Decimal", eFormatDec);
  formatCombo->addItem ("Signed Int16", eFormatInt16);
  formatCombo->addItem ("Hex", eFormatHex);
  formatCombo->addItem ("String", eFormatString);
  formatCombo->addItem ("32-bit Integer", eFormatInt);
  formatCombo->addItem ("32-bit Float", eFormatFloat);
  formatCombo->setCurrentIndex (1);
}

void
MainWindow::updateTransportUi () {

  const int mode = modeCombo->currentData ().toInt ();

  transportStack->setCurrentIndex (mode == eModeRtu ? TransportRtu : TransportTcp);
  if (mode == eModeRtu) {
    slaveSpin->setMinimum (RTU_SLAVEADDR_MIN);
    if (slaveSpin->value () < RTU_SLAVEADDR_MIN) {
      slaveSpin->setValue (RTU_SLAVEADDR_MIN);
    }
  }
  else {
    slaveSpin->setMinimum (TCP_SLAVEADDR_MIN);
  }
  updateSummary ();
}

void
MainWindow::updateFormatUi () {
  const int function = currentFunction ();
  const bool isBinary = bMbPollFunctionUsesBinary (static_cast<eFunctions> (function));
  const bool canWrite = writeSupported ();
  const int previousFormat = currentFormat ();

  formatCombo->clear ();

  if (isBinary) {
    formatCombo->addItem ("Binary", eFormatBin);
  }
  else {
    formatCombo->addItem ("Decimal", eFormatDec);
    formatCombo->addItem ("Signed Int16", eFormatInt16);
    formatCombo->addItem ("Hex", eFormatHex);
    if (!canWrite) {
      formatCombo->addItem ("String", eFormatString);
    }
    formatCombo->addItem ("32-bit Integer", eFormatInt);
    formatCombo->addItem ("32-bit Float", eFormatFloat);
  }

  const int restoredIndex = formatCombo->findData (previousFormat);
  if (restoredIndex >= 0) {
    formatCombo->setCurrentIndex (restoredIndex);
  }
  else {
    formatCombo->setCurrentIndex (0);
  }

  bigEndianCheck->setEnabled (currentFormat () == eFormatInt ||
                              currentFormat () == eFormatFloat);
  writeMultipleCheck->setEnabled (function == eFuncHoldingReg);
  writeButton->setEnabled (canWrite);

  if (function == eFuncCoil) {
    writeValuesEdit->setPlaceholderText ("0 or 1,0,1");
  }
  else if (function == eFuncHoldingReg && currentFormat () == eFormatFloat) {
    writeValuesEdit->setPlaceholderText ("12.5 or 1.25,3.5");
  }
  else {
    writeValuesEdit->setPlaceholderText ("123 or 10,20,30");
  }
  updateSummary ();
}

void
MainWindow::updateSummary () {
  xMbPollContext ctx;
  char buffer[256];
  char typeBuffer[256];

  vMbPollContextInit (&ctx);
  applyUiToContext (&ctx);
  ctx.bIsWrite = false;
  iMbPollDescribeConnection (&ctx, buffer, sizeof (buffer));
  iMbPollDescribeDataType (&ctx, typeBuffer, sizeof (typeBuffer));

  summaryLabel->setText (QString::fromLocal8Bit (buffer));
  typeLabel->setText (QString ("Type: %1 | slave %2 | start %3 | count %4")
                      .arg (QString::fromLocal8Bit (typeBuffer))
                      .arg (slaveSpin->value ())
                      .arg (startRefSpin->value ())
                      .arg (countSpin->value ()));
}

void
MainWindow::readOnce () {
  xMbPollContext ctx;
  xMbPollReadResult result = {};
  QString error;
  char sError[kErrorBufferSize];

  if (pollTimer->isActive ()) {
    stopPolling ();
  }
  if (!prepareReadContext (&ctx, &error)) {
    setStatusMessage (error, true);
    appendLog (error);
    return;
  }

  if (iMbPollOpen (&ctx, sError, sizeof (sError)) != 0) {
    const QString message = QString::fromLocal8Bit (sError);
    setStatusMessage (message, true);
    appendLog (message);
    return;
  }

  if (iMbPollReadOnce (&ctx, slaveSpin->value (), startRefSpin->value (),
                       &result, sError, sizeof (sError)) != 0) {
    const QString message = QString::fromLocal8Bit (sError);
    setStatusMessage (message, true);
    appendLog (message);
  }
  else {
    renderReadResult (result);
    setStatusMessage ("Read completed", false);
    appendLog (QString ("Read %1 value(s) from slave %2.")
               .arg (countSpin->value ()).arg (slaveSpin->value ()));
  }

  vMbPollReadResultFree (&result);
  vMbPollClose (&ctx);
}

void
MainWindow::writeOnce () {
  xMbPollContext ctx;
  QString error;
  char sError[kErrorBufferSize];

  if (pollTimer->isActive ()) {
    stopPolling ();
  }
  if (!prepareWriteContext (&ctx, &error)) {
    setStatusMessage (error, true);
    appendLog (error);
    return;
  }

  if (iMbPollOpen (&ctx, sError, sizeof (sError)) != 0) {
    const QString message = QString::fromLocal8Bit (sError);
    setStatusMessage (message, true);
    appendLog (message);
    vMbPollFreeData (&ctx);
    return;
  }

  if (iMbPollWriteOnce (&ctx, slaveSpin->value (), startRefSpin->value (),
                        sError, sizeof (sError)) != 0) {
    const QString message = QString::fromLocal8Bit (sError);
    setStatusMessage (message, true);
    appendLog (message);
  }
  else {
    setStatusMessage ("Write completed", false);
    appendLog (QString ("Wrote %1 value(s) to slave %2.")
               .arg (countSpin->value ()).arg (slaveSpin->value ()));
  }

  vMbPollClose (&ctx);
  vMbPollFreeData (&ctx);
}

void
MainWindow::startPolling () {

  if (pollTimer->isActive ()) {
    pollTimer->start (pollRateSpin->value ());
    return;
  }

  pollTimer->start (pollRateSpin->value ());
  startPollButton->setEnabled (false);
  stopPollButton->setEnabled (true);
  setStatusMessage ("Polling started", false);
  appendLog (QString ("Polling every %1 ms.").arg (pollRateSpin->value ()));
  pollTick ();
}

void
MainWindow::stopPolling () {

  pollTimer->stop ();
  startPollButton->setEnabled (true);
  stopPollButton->setEnabled (false);
  setStatusMessage ("Polling stopped", false);
  appendLog ("Polling stopped.");
}

void
MainWindow::pollTick () {
  xMbPollContext ctx;
  xMbPollReadResult result = {};
  QString error;
  char sError[kErrorBufferSize];

  if (!prepareReadContext (&ctx, &error)) {
    setStatusMessage (error, true);
    appendLog (error);
    stopPolling ();
    return;
  }

  if (iMbPollOpen (&ctx, sError, sizeof (sError)) != 0) {
    const QString message = QString::fromLocal8Bit (sError);
    setStatusMessage (message, true);
    appendLog (QString ("Polling error: %1").arg (message));
    return;
  }

  if (iMbPollReadOnce (&ctx, slaveSpin->value (), startRefSpin->value (),
                       &result, sError, sizeof (sError)) != 0) {
    const QString message = QString::fromLocal8Bit (sError);
    setStatusMessage (message, true);
    appendLog (QString ("Polling error: %1").arg (message));
  }
  else {
    renderReadResult (result);
    setStatusMessage ("Polling tick completed", false);
  }

  vMbPollReadResultFree (&result);
  vMbPollClose (&ctx);
}

void
MainWindow::setStatusMessage (const QString & text, bool isError) {

  statusLabel->setText (text);
  statusLabel->setStyleSheet (isError
                              ? "color: #9f1d20; font-weight: 700;"
                              : "color: #1f6f43; font-weight: 700;");
  statusBar ()->showMessage (text, 5000);
}

void
MainWindow::appendLog (const QString & text) {

  const QString stamp = QDateTime::currentDateTime ().toString ("yyyy-MM-dd HH:mm:ss");
  logEdit->appendPlainText (QString ("[%1] %2").arg (stamp, text));
}

void
MainWindow::applyUiToContext (xMbPollContext * ctx) const {
  static QByteArray tcpPortBuffer;
  static QByteArray deviceBuffer;

  ctx->eMode = static_cast<eModes> (modeCombo->currentData ().toInt ());
  ctx->eFunction = static_cast<eFunctions> (currentFunction ());
  ctx->eFormat = static_cast<eFormats> (currentFormat ());
  ctx->bIsBigEndian = bigEndianCheck->isChecked ();
  ctx->dTimeout = timeoutSpin->value ();
  ctx->iPollRate = pollRateSpin->value ();
  ctx->iPduOffset = 1;
  ctx->iCount = countSpin->value ();
  ctx->bWriteSingleAsMany = writeMultipleCheck->isChecked ();

  if (ctx->eMode == eModeTcp) {
    deviceBuffer = tcpHostEdit->text ().trimmed ().toLocal8Bit ();
    tcpPortBuffer = QByteArray::number (tcpPortSpin->value ());
    ctx->sDevice = deviceBuffer.data ();
    ctx->sTcpPort = tcpPortBuffer.data ();
  }
  else {
    deviceBuffer = rtuDeviceEdit->text ().trimmed ().toLocal8Bit ();
    ctx->sDevice = deviceBuffer.data ();
  }

  if (bMbPollFunctionUsesBinary (ctx->eFunction)) {
    ctx->eFormat = eFormatBin;
  }
}

bool
MainWindow::prepareReadContext (xMbPollContext * ctx, QString * error) const {

  vMbPollContextInit (ctx);
  applyUiToContext (ctx);
  ctx->bIsWrite = false;

  if (ctx->sDevice == nullptr || ctx->sDevice[0] == '\0') {
    *error = ctx->eMode == eModeTcp ? "TCP host is required." : "RTU device is required.";
    return false;
  }
  if (ctx->eMode == eModeRtu &&
      (ctx->xRtu.baud < RTU_BAUDRATE_MIN || ctx->xRtu.baud > RTU_BAUDRATE_MAX)) {
    *error = QString ("Baudrate must be between %1 and %2.")
             .arg (RTU_BAUDRATE_MIN).arg (RTU_BAUDRATE_MAX);
    return false;
  }
  if ( (ctx->eFunction == eFuncInputReg || ctx->eFunction == eFuncHoldingReg) &&
       (ctx->eFormat == eFormatInt || ctx->eFormat == eFormatFloat) &&
       ctx->iCount > 62) {
    *error = "32-bit register types are limited to 62 values per request.";
    return false;
  }
  return true;
}

bool
MainWindow::prepareWriteContext (xMbPollContext * ctx, QString * error) const {

  if (!writeSupported ()) {
    *error = "Only coil and holding register writes are supported in the desktop MVP.";
    return false;
  }
  if (!prepareReadContext (ctx, error)) {
    return false;
  }

  ctx->bIsWrite = true;
  if (!parseWriteValues (ctx, error)) {
    return false;
  }
  return true;
}

bool
MainWindow::parseWriteValues (xMbPollContext * ctx, QString * error) const {
  const QString raw = writeValuesEdit->toPlainText ().trimmed ();
  const QStringList parts =
    raw.split (QRegularExpression ("[,;\\s]+"), Qt::SkipEmptyParts);
  char sError[kErrorBufferSize];

  if (parts.isEmpty ()) {
    *error = "Provide at least one value to write.";
    return false;
  }

  ctx->iCount = parts.size ();
  if (iMbPollAllocateData (ctx, sError, sizeof (sError)) != 0) {
    *error = QString::fromLocal8Bit (sError);
    return false;
  }

  for (int i = 0; i < parts.size (); ++i) {
    const QByteArray valueBytes = parts[i].toLocal8Bit ();

    if (iMbPollSetWriteValueString (ctx, i, valueBytes.constData (),
                                    sError, sizeof (sError)) != 0) {
      *error = QString::fromLocal8Bit (sError);
      vMbPollFreeData (ctx);
      return false;
    }
  }

  countSpin->setValue (parts.size ());
  return true;
}

void
MainWindow::renderReadResult (const xMbPollReadResult & result) {

  resultTable->setRowCount (result.iRowCount);
  for (int i = 0; i < result.iRowCount; ++i) {
    auto * referenceItem =
      new QTableWidgetItem (QString::number (result.pxRows[i].iReference));
    auto * valueItem =
      new QTableWidgetItem (QString::fromLocal8Bit (result.pxRows[i].sValue));
    resultTable->setItem (i, 0, referenceItem);
    resultTable->setItem (i, 1, valueItem);
  }
}

int
MainWindow::currentFunction () const {

  return functionCombo->currentData ().toInt ();
}

int
MainWindow::currentFormat () const {

  return formatCombo->currentData ().toInt ();
}

bool
MainWindow::writeSupported () const {

  const int function = currentFunction ();
  return function == eFuncCoil || function == eFuncHoldingReg;
}
