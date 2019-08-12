// Copyright (c) 2011-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include "config/navcoin-config.h"
#endif

#include "splashscreen.h"

#include "networkstyle.h"

#include "clientversion.h"
#include "init.h"
#include "util.h"
#include "ui_interface.h"
#include "version.h"

#ifdef ENABLE_WALLET
#include "wallet/wallet.h"
#endif

#include <QApplication>
#include <QCloseEvent>
#include <QDesktopWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>

SplashScreen::SplashScreen(Qt::WindowFlags f, const NetworkStyle *networkStyle) :
    QWidget(0, f)
{
    // define text to place
    QString titleText       = tr(PACKAGE_NAME);
    QString versionText     = QString::fromStdString(FormatFullVersion());
    QString titleAddText    = networkStyle->getTitleAddText();

    // create a bitmap according to device pixelratio
    QSize splashSize(480 * scale(), 320 * scale());
    pixmap = QPixmap(splashSize);

    // Seupt the painter
    QPainter pixPaint(&pixmap);

    QRect rect(QPoint(0,0), splashSize);

    // Fill with white
    pixPaint.fillRect(rect, Qt::white);

    // Load the icon
    QPixmap icon(":icons/splash");

    // Now draw the icon
    pixPaint.drawPixmap(rect, icon);

    // We are done
    pixPaint.end();

    // Build the text
    QString labelText = versionText;

    // Check if we have more text (IE testnet/devnet)
    if(!titleAddText.isEmpty())
        labelText += " <span style='font-weight: bold;'>" + titleAddText + "</span>";

    // Make the version layout
    QLayout* layout = new QHBoxLayout(this);
    layout->setAlignment(Qt::AlignTop | Qt::AlignRight);

    QWidget* spacer = new QWidget();
    layout->addWidget(spacer);

    // Build the new label
    QLabel* label = new QLabel();
    label->setText(labelText);
    label->setStyleSheet("padding: 2pt; font-size: 8pt;");
    layout->addWidget(label);

    // Set window title
    setWindowTitle(titleText + " " + titleAddText);

    // Resize window and move to center of desktop, disallow resizing
    QRect r(QPoint(), splashSize);
    resize(r.size());
    setFixedSize(r.size());
    move(QApplication::desktop()->screenGeometry().center() - r.center());

    subscribeToCoreSignals();
}

SplashScreen::~SplashScreen()
{
    unsubscribeFromCoreSignals();
}

float SplashScreen::scale()
{
    static float scale = 0.0f;

    if (scale == 0.0f)
        scale = (float) QWidget::logicalDpiX() / 96;

    return scale;
}

void SplashScreen::slotFinish(QWidget *mainWin)
{
    Q_UNUSED(mainWin);

    /* If the window is minimized, hide() will be ignored. */
    /* Make sure we de-minimize the splashscreen window before hiding */
    if (isMinimized())
        showNormal();
    hide();
}

static void InitMessage(SplashScreen *splash, const std::string &message)
{
    QMetaObject::invokeMethod(splash, "showMessage",
        Qt::QueuedConnection,
        Q_ARG(QString, QString::fromStdString(message)),
        Q_ARG(QColor, QColor(55,55,55)));
}

static void ShowProgress(SplashScreen *splash, const std::string &title, int nProgress)
{
    InitMessage(splash, title + strprintf("%d", nProgress) + "%");
}

#ifdef ENABLE_WALLET
static void ConnectWallet(SplashScreen *splash, CWallet* wallet)
{
    wallet->ShowProgress.connect(boost::bind(ShowProgress, splash, _1, _2));
}
#endif

void SplashScreen::subscribeToCoreSignals()
{
    // Connect signals to client
    uiInterface.InitMessage.connect(boost::bind(InitMessage, this, _1));
    uiInterface.ShowProgress.connect(boost::bind(ShowProgress, this, _1, _2));
#ifdef ENABLE_WALLET
    uiInterface.LoadWallet.connect(boost::bind(ConnectWallet, this, _1));
#endif
}

void SplashScreen::unsubscribeFromCoreSignals()
{
    // Disconnect signals from client
    uiInterface.InitMessage.disconnect(boost::bind(InitMessage, this, _1));
    uiInterface.ShowProgress.disconnect(boost::bind(ShowProgress, this, _1, _2));
#ifdef ENABLE_WALLET
    if(pwalletMain)
        pwalletMain->ShowProgress.disconnect(boost::bind(ShowProgress, this, _1, _2));
#endif
}

void SplashScreen::showMessage(const QString &message, const QColor &color)
{
    curMessage = message;
    curColor = color;
    update();
}

void SplashScreen::paintEvent(QPaintEvent *event)
{
    // What size to render the font in
    int size = 10 * scale();

    // Build a rect
    QRect r = rect().adjusted(size, size, -size, -size);

    // Make a bold font
    QFont font = QFont(QApplication::font().toString(), size);

    // Paint the window and text
    QPainter painter(this);
    painter.setFont(font);
    painter.setPen(curColor);
    painter.drawPixmap(0, 0, pixmap);
    painter.drawText(r, Qt::AlignBottom | Qt::AlignCenter, curMessage);
}

void SplashScreen::closeEvent(QCloseEvent *event)
{
    StartShutdown(); // allows an "emergency" shutdown during startup
    event->ignore();
}
