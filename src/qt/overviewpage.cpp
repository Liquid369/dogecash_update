// Copyright (c) 2011-2014 The Bitcoin developers
// Copyright (c) 2014-2015 The Dash developers
// Copyright (c) 2015-2017 The PIVX developers
// Copyright (c) 2018 The DogeCash developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "overviewpage.h"
#include "ui_overviewpage.h"

#include "bitcoinunits.h"
#include "clientmodel.h"
#include "guiconstants.h"
#include "guiutil.h"
#include "init.h"
#include "obfuscation.h"
#include "obfuscationconfig.h"
#include "optionsmodel.h"
#include "transactionfilterproxy.h"
#include "transactiontablemodel.h"
#include "walletmodel.h"
#include <widget.h>

#include <QAbstractItemDelegate>
#include <QPainter>
#include <QSettings>
#include <QTimer>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>

#define DECORATION_SIZE 65
#define ICON_OFFSET 16
#define NUM_ITEMS 8

extern CWallet* pwalletMain;

class TxViewDelegate : public QAbstractItemDelegate
{
    Q_OBJECT
public:
    TxViewDelegate() : QAbstractItemDelegate(), unit(BitcoinUnits::DOGEC)
    {
    }

    inline void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        painter->save();

        QIcon icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
//        QIcon icon = qvariant_cast<QIcon>(index.data(TransactionTableModel::RawDecorationRole));
        QRect mainRect = option.rect;
//        mainRect.moveLeft(ICON_OFFSET);
//        QRect decorationRect(mainRect.topLeft(), QSize(DECORATION_SIZE, DECORATION_SIZE));
//        int xspace = DECORATION_SIZE + 8;
        painter->setRenderHint(QPainter::Antialiasing);
//        QPainterPath path;
        QRect contentRect = QRect(mainRect.left() + 5, mainRect.top() + 10, mainRect.width() - 20, mainRect.height() - 20);
//        path.addRoundedRect(contentRect, 5, 5);
//        painter->fillPath(path, QBrush(QColor(37, 50, 82)));
        QPoint iconTopleft = QPoint(contentRect.left(), contentRect.top() + (contentRect.height() - DECORATION_SIZE) / 2 - 15);
        QRect decorationRect(iconTopleft, QSize(DECORATION_SIZE, DECORATION_SIZE));
        int xspace = DECORATION_SIZE + 15;

        int ypad = 6;
        int halfheight = (mainRect.height() - 2 * ypad) / 2;
//        QRect amountRect(mainRect.left() + xspace, mainRect.top() + ypad, mainRect.width() - xspace - ICON_OFFSET, halfheight);
        QRect dateRect(mainRect.left() + xspace, mainRect.top()+ypad, mainRect.width() - xspace, halfheight);
        QRect addressRect(mainRect.left() + xspace, mainRect.top() + ypad + halfheight, mainRect.width() - xspace, halfheight);
        icon = QIcon(icon);
        icon.paint(painter, decorationRect);

        QDateTime date = index.data(TransactionTableModel::DateRole).toDateTime();
        QString address = index.data(Qt::DisplayRole).toString();
        qint64 amount = index.data(TransactionTableModel::AmountRole).toLongLong();
        bool confirmed = index.data(TransactionTableModel::ConfirmedRole).toBool();
        QVariant value = index.data(Qt::ForegroundRole);
        QColor foreground = COLOR_BLACK;
        if (value.canConvert<QBrush>()) {
            QBrush brush = qvariant_cast<QBrush>(value);
            foreground = brush.color();
        }

//        painter->setPen(foreground);
        painter->setPen(QColor(185, 185, 185));
        QFont w_orgFont = painter->font();
        QFont w_newFont = w_orgFont;
        w_newFont.setPixelSize(13);
        painter->setFont(w_newFont);

        QRect boundingRect;
        painter->drawText(addressRect, Qt::AlignLeft | Qt::AlignVCenter, address, &boundingRect);

        if (index.data(TransactionTableModel::WatchonlyRole).toBool()) {
            QIcon iconWatchonly = qvariant_cast<QIcon>(index.data(TransactionTableModel::WatchonlyDecorationRole));
            QRect watchonlyRect(boundingRect.right() + 5, mainRect.top() + ypad + halfheight, 16, halfheight);
            iconWatchonly.paint(painter, watchonlyRect);
        }

        if (amount < 0) {
            foreground = QColor(255, 85, 71);
        } else if (!confirmed) {
            foreground = COLOR_UNCONFIRMED;
        } else {
            foreground = QColor(86, 192, 49);
        }
        painter->setPen(foreground);
        QString amountText = BitcoinUnits::formatWithComma(unit, amount, true, BitcoinUnits::separatorAlways);
        if (!confirmed) {
            amountText = QString("[") + amountText + QString("]");
        }
        QRect amountRect1 = contentRect;
        amountRect1.setWidth(contentRect.width() - 10);

        w_newFont.setPixelSize(15);
        painter->setFont(w_newFont);

        painter->drawText(amountRect1, Qt::AlignRight | Qt::AlignVCenter, amountText);

        painter->setPen(COLOR_BLACK);
        painter->drawText(dateRect, Qt::AlignLeft | Qt::AlignVCenter, GUIUtil::dateTimeStr(date));

        painter->setFont(w_orgFont);

        painter->restore();
    }

    inline QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        return QSize(DECORATION_SIZE, DECORATION_SIZE);
    }

    int unit;
};
#include "overviewpage.moc"

OverviewPage::OverviewPage(QWidget* parent) : QWidget(parent),
                                              ui(new Ui::OverviewPage),
                                              clientModel(0),
                                              walletModel(0),
                                              currentBalance(-1),
                                              currentUnconfirmedBalance(-1),
                                              currentImmatureBalance(-1),
                                              currentZerocoinBalance(-1),
                                              currentUnconfirmedZerocoinBalance(-1),
                                              currentimmatureZerocoinBalance(-1),
                                              currentWatchOnlyBalance(-1),
                                              currentWatchUnconfBalance(-1),
                                              currentWatchImmatureBalance(-1),
                                              txdelegate(new TxViewDelegate()),
                                              filter(0)
{
    nDisplayUnit = 0; // just make sure it's not unitialized

    ui->setupUi(this);

    // Recent transactions
    ui->listTransactions->setItemDelegate(txdelegate);
    ui->listTransactions->setIconSize(QSize(DECORATION_SIZE, DECORATION_SIZE));
    ui->listTransactions->setMinimumHeight(NUM_ITEMS * (DECORATION_SIZE + 2));
    ui->listTransactions->setAttribute(Qt::WA_MacShowFocusRect, false);

    main_widget = new QWidget();
//    ui->horizontalLayout->addWidget(main_widget);
    main_widget->setStyleSheet("background-color: transparent");
    main_widget->setParent(this);
    main_widget->setGeometry(0, 158, this->width(), this->height()-190);
    main_widget->show();
    QWidget *emptyWidget = new QWidget();
    emptyWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->horizontalLayout->addWidget(emptyWidget);

    setBackPage();
    setBodyPage();
    setFontOverViewPage();

    ui->labelAlerts->hide();
    ui->frame->hide();
    ui->frame_2->hide();
    ui->frame_3->hide();
    ui->frame_4->hide();


    connect(ui->listTransactions, SIGNAL(clicked(QModelIndex)), this, SLOT(handleTransactionClicked(QModelIndex)));


    // init "out of sync" warning labels
    ui->labelWalletStatus->setText("(" + tr("out of sync") + ")");
    ui->labelTransactionsStatus->setText("(" + tr("out of sync") + ")");

    // start with displaying the "out of sync" warnings
    showOutOfSyncWarning(true);
}

void OverviewPage::handleTransactionClicked(const QModelIndex& index)
{
    if (filter)
        emit transactionClicked(filter->mapToSource(index));
}

OverviewPage::~OverviewPage()
{
    delete ui;
}

void OverviewPage::getPercentage(CAmount nUnlockedBalance, CAmount nZerocoinBalance, QString& sDOGECPercentage, QString& szDOGECPercentage)
{
    int nPrecision = 2;
    double dzPercentage = 0.0;

    if (nZerocoinBalance <= 0){
        dzPercentage = 0.0;
    }
    else{
        if (nUnlockedBalance <= 0){
            dzPercentage = 100.0;
        }
        else{
            dzPercentage = 100.0 * (double)(nZerocoinBalance / (double)(nZerocoinBalance + nUnlockedBalance));
        }
    }

    double dPercentage = 100.0 - dzPercentage;
    
    szDOGECPercentage = "(" + QLocale(QLocale::system()).toString(dzPercentage, 'f', nPrecision) + " %)";
    sDOGECPercentage = "(" + QLocale(QLocale::system()).toString(dPercentage, 'f', nPrecision) + " %)";
    
}

void OverviewPage::setBalance(const CAmount& balance, const CAmount& unconfirmedBalance, const CAmount& immatureBalance, 
                              const CAmount& zerocoinBalance, const CAmount& unconfirmedZerocoinBalance, const CAmount& immatureZerocoinBalance,
                              const CAmount& watchOnlyBalance, const CAmount& watchUnconfBalance, const CAmount& watchImmatureBalance)
{
    currentBalance = balance;
    currentUnconfirmedBalance = unconfirmedBalance;
    currentImmatureBalance = immatureBalance;
    currentZerocoinBalance = zerocoinBalance;
    currentUnconfirmedZerocoinBalance = unconfirmedZerocoinBalance;
    currentimmatureZerocoinBalance = immatureZerocoinBalance;
    currentWatchOnlyBalance = watchOnlyBalance;
    currentWatchUnconfBalance = watchUnconfBalance;
    currentWatchImmatureBalance = watchImmatureBalance;

    // DOGEC labels
    ui->labelBalance->setText(BitcoinUnits::floorHtmlWithoutUnit(nDisplayUnit, balance - immatureBalance, false, BitcoinUnits::separatorAlways));
    ui->labelzBalance->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, zerocoinBalance, false, BitcoinUnits::separatorAlways));
    ui->labelUnconfirmed->setText(BitcoinUnits::floorHtmlWithoutUnit(nDisplayUnit, unconfirmedBalance, false, BitcoinUnits::separatorAlways));
    ui->labelImmature->setText(BitcoinUnits::floorHtmlWithoutUnit(nDisplayUnit, immatureBalance, false, BitcoinUnits::separatorAlways));
    ui->labelTotal->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, balance + unconfirmedBalance, false, BitcoinUnits::separatorAlways));

    // Watchonly labels
    ui->labelWatchAvailable->setText(BitcoinUnits::floorHtmlWithoutUnit(nDisplayUnit, watchOnlyBalance, false, BitcoinUnits::separatorAlways));

    body_value_available->setText(QString::number(watchOnlyBalance) + " DOGEC");

    ui->labelWatchPending->setText(BitcoinUnits::floorHtmlWithoutUnit(nDisplayUnit, watchUnconfBalance, false, BitcoinUnits::separatorAlways));
    ui->labelWatchImmature->setText(BitcoinUnits::floorHtmlWithoutUnit(nDisplayUnit, watchImmatureBalance, false, BitcoinUnits::separatorAlways));

    body_value_immature->setText(QString::number(watchImmatureBalance) + " DOGEC");

    ui->labelWatchTotal->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, watchOnlyBalance + watchUnconfBalance + watchImmatureBalance, false, BitcoinUnits::separatorAlways));
//    QString value_total = BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, watchOnlyBalance + watchUnconfBalance + watchImmatureBalance, false, BitcoinUnits::separatorAlways);

//    QLineEdit *editLine = new QLineEdit();
//    editLine->setText(BitcoinUnits::floorHtmlWithoutUnit(nDisplayUnit, watchOnlyBalance, false, BitcoinUnits::separatorAlways));
//    editLine->show();
//    value_total = value_total.mid(35,value_total.length() - 48);
//    int total_value = value_total.toDouble();

    body_value_total->setText(QString::number(watchOnlyBalance + watchUnconfBalance + watchImmatureBalance));

    // zDOGEC labels
    QString szPercentage = "";
    QString sPercentage = "";
    CAmount nLockedBalance = 0;
    if (pwalletMain) {
        nLockedBalance = pwalletMain->GetLockedCoins();
    }
    ui->labelLockedBalance->setText(BitcoinUnits::floorHtmlWithoutUnit(nDisplayUnit, nLockedBalance, false, BitcoinUnits::separatorAlways));

    CAmount nTotalBalance = balance + unconfirmedBalance;
    CAmount nUnlockedBalance = nTotalBalance - nLockedBalance;
    CAmount matureZerocoinBalance = zerocoinBalance - immatureZerocoinBalance;
    getPercentage(nUnlockedBalance, zerocoinBalance, sPercentage, szPercentage);

    ui->labelBalancez->setText(BitcoinUnits::floorHtmlWithoutUnit(nDisplayUnit, nTotalBalance, false, BitcoinUnits::separatorAlways));
    ui->labelzBalancez->setText(BitcoinUnits::floorHtmlWithoutUnit(nDisplayUnit, zerocoinBalance, false, BitcoinUnits::separatorAlways));
    ui->labelzBalanceImmature->setText(BitcoinUnits::floorHtmlWithoutUnit(nDisplayUnit, immatureZerocoinBalance, false, BitcoinUnits::separatorAlways));
    ui->labelzBalanceUnconfirmed->setText(BitcoinUnits::floorHtmlWithoutUnit(nDisplayUnit, unconfirmedZerocoinBalance, false, BitcoinUnits::separatorAlways));
    ui->labelzBalanceMature->setText(BitcoinUnits::floorHtmlWithoutUnit(nDisplayUnit, matureZerocoinBalance, false, BitcoinUnits::separatorAlways));
    ui->labelTotalz->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, nTotalBalance + zerocoinBalance, false, BitcoinUnits::separatorAlways));
    ui->labelUnLockedBalance->setText(BitcoinUnits::floorHtmlWithoutUnit(nDisplayUnit, nUnlockedBalance, false, BitcoinUnits::separatorAlways));
//    ui->labelDOGECPercent->setText(sPercentage);
//    ui->labelzDOGECPercent->setText(szPercentage);

    // Adjust bubble-help according to AutoMint settings
    QString automintHelp = tr("Current percentage of zDOGEC.\nIf AutoMint is enabled this percentage will settle around the configured AutoMint percentage (default = 10%).\n");
    bool fEnableZeromint = GetBoolArg("-enablezeromint", false);
    int nZeromintPercentage = GetArg("-zeromintpercentage", 10);
    if (fEnableZeromint) {
        automintHelp += tr("AutoMint is currently enabled and set to ") + QString::number(nZeromintPercentage) + "%.\n";
        automintHelp += tr("To disable AutoMint delete set 'enablezeromint=1' to 'enablezeromint=0' in dogecash2.conf.");
    }
    else {
        automintHelp += tr("AutoMint is currently disabled.\nTo enable AutoMint add 'enablezeromint=1' in dogecash2.conf");
    }
//    ui->labelzDOGECPercent->setToolTip(automintHelp);

    // only show immature (newly mined) balance if it's non-zero, so as not to complicate things
    // for the non-mining users
    bool showImmature = immatureBalance != 0;
    bool showWatchOnlyImmature = watchImmatureBalance != 0;

    // for symmetry reasons also show immature label when the watch-only one is shown
    ui->labelImmature->setVisible(showImmature || showWatchOnlyImmature);
    ui->labelImmatureText->setVisible(showImmature || showWatchOnlyImmature);
    ui->labelWatchImmature->setVisible(showWatchOnlyImmature); // show watch-only immature balance

    static int cachedTxLocks = 0;

    if (cachedTxLocks != nCompleteTXLocks) {
        cachedTxLocks = nCompleteTXLocks;
        ui->listTransactions->update();
    }
}

// show/hide watch-only labels
void OverviewPage::updateWatchOnlyLabels(bool showWatchOnly)
{
    ui->labelSpendable->setVisible(showWatchOnly);      // show spendable label (only when watch-only is active)
    ui->labelWatchonly->setVisible(showWatchOnly);      // show watch-only label
//    ui->lineWatchBalance->setVisible(showWatchOnly);    // show watch-only balance separator line
    ui->labelWatchAvailable->setVisible(showWatchOnly); // show watch-only available balance
    ui->labelWatchPending->setVisible(showWatchOnly);   // show watch-only pending balance
    ui->labelWatchTotal->setVisible(showWatchOnly);     // show watch-only total balance

    if (!showWatchOnly) {
        ui->labelWatchImmature->hide();
    } else {
        ui->labelBalance->setIndent(20);
        ui->labelUnconfirmed->setIndent(20);
        ui->labelImmature->setIndent(20);
        ui->labelTotal->setIndent(20);
    }
}

void OverviewPage::setClientModel(ClientModel* model)
{
    this->clientModel = model;
    if (model) {
        // Show warning if this is a prerelease version
        connect(model, SIGNAL(alertsChanged(QString)), this, SLOT(updateAlerts(QString)));
        updateAlerts(model->getStatusBarWarnings());
    }
}

void OverviewPage::setWalletModel(WalletModel* model)
{
    this->walletModel = model;
    if (model && model->getOptionsModel()) {
        // Set up transaction list
        filter = new TransactionFilterProxy();
        filter->setSourceModel(model->getTransactionTableModel());
        filter->setLimit(NUM_ITEMS);
        filter->setDynamicSortFilter(true);
        filter->setSortRole(Qt::EditRole);
        filter->setShowInactive(false);
        filter->sort(TransactionTableModel::Date, Qt::DescendingOrder);

        ui->listTransactions->setModel(filter);
        ui->listTransactions->setModelColumn(TransactionTableModel::ToAddress);

        // Keep up to date with wallet
        setBalance(model->getBalance(), model->getUnconfirmedBalance(), model->getImmatureBalance(),
                   model->getZerocoinBalance(), model->getUnconfirmedZerocoinBalance(), model->getImmatureZerocoinBalance(), 
                   model->getWatchBalance(), model->getWatchUnconfirmedBalance(), model->getWatchImmatureBalance());
        connect(model, SIGNAL(balanceChanged(CAmount, CAmount, CAmount, CAmount, CAmount, CAmount, CAmount, CAmount, CAmount)), this, 
                         SLOT(setBalance(CAmount, CAmount, CAmount, CAmount, CAmount, CAmount, CAmount, CAmount, CAmount)));

        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));

        updateWatchOnlyLabels(model->haveWatchOnly());
        connect(model, SIGNAL(notifyWatchonlyChanged(bool)), this, SLOT(updateWatchOnlyLabels(bool)));
    }

    // update the display unit, to not use the default ("DOGEC")
    updateDisplayUnit();
}

void OverviewPage::updateDisplayUnit()
{
    if (walletModel && walletModel->getOptionsModel()) {
        nDisplayUnit = walletModel->getOptionsModel()->getDisplayUnit();
        if (currentBalance != -1)
            setBalance(currentBalance, currentUnconfirmedBalance, currentImmatureBalance, currentZerocoinBalance, currentUnconfirmedZerocoinBalance, currentimmatureZerocoinBalance,
                currentWatchOnlyBalance, currentWatchUnconfBalance, currentWatchImmatureBalance);

        // Update txdelegate->unit with the current unit
        txdelegate->unit = nDisplayUnit;

        ui->listTransactions->update();
    }
}

void OverviewPage::updateAlerts(const QString& warnings)
{
    this->ui->labelAlerts->setVisible(!warnings.isEmpty());
    this->ui->labelAlerts->setText(warnings);
}

void OverviewPage::showOutOfSyncWarning(bool fShow)
{
    ui->labelWalletStatus->setVisible(fShow);
    ui->labelTransactionsStatus->setVisible(fShow);
}

void OverviewPage::setBackPage()
{
    line_widget = new QWidget();
    line_widget->setLayout(new QHBoxLayout);
    line_label = new QLabel();
    line_label->setPixmap(QPixmap(":/images/res/images/main-back.png"));
    line_label->setScaledContents(true);
    line_label->setMinimumWidth(line_widget->height());
    line_label->setMaximumWidth(line_widget->height());
    line_widget->layout()->addWidget(line_label);
    line_widget->setParent(main_widget);
    line_widget->setGeometry(0,0,main_widget->width(), main_widget->height());
    line_widget->show();

}

void OverviewPage::setBodyPage()
{
    body_widget = new QWidget();
    body_widget->setLayout(new QHBoxLayout());

    body_widget->setParent(main_widget);
    body_widget->setGeometry(0, 0, main_widget->width(), main_widget->height());
    body_widget->setStyleSheet("background-color: transparent");
    QWidget *h_emptyWidget1 = new QWidget();
    h_emptyWidget1->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    h_emptyWidget1->setStyleSheet("background-color: transparent");
    QWidget *h_emptyWidget2 = new QWidget();
    h_emptyWidget2->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    h_emptyWidget2->setStyleSheet("background-color: transparent");

    QVBoxLayout *v_bodylayout1 = new QVBoxLayout();
    v_bodylayout1->setSpacing(10);
    v_bodylayout1->setContentsMargins(0,0,0,0);
    v_bodylayout1->setMargin(0);

    body_widget->layout()->addWidget(h_emptyWidget1);
    QWidget *new_horizontal = new QWidget();
    new_horizontal->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    new_horizontal->setLayout(v_bodylayout1);
    body_widget->layout()->addWidget(new_horizontal); //main data area with top, bottom space.
    body_widget->layout()->addWidget(h_emptyWidget2);



    QWidget *v_emptyWidget1 = new QWidget();
    v_emptyWidget1->setStyleSheet("background-color: transparent");

    QWidget *v_emptyWidget2 = new QWidget();
    v_emptyWidget2->setStyleSheet("background-color: transparent");
    QWidget *dataWidget = new QWidget();

    QHBoxLayout *h_topBodyLayout = new QHBoxLayout();
    h_topBodyLayout->setSpacing(7);
    dataWidget->setLayout(h_topBodyLayout);

    QHBoxLayout *h_bottomBodyLayout = new QHBoxLayout();
    h_bottomBodyLayout->setSpacing(10);

    QWidget *total_bodyWidget = new QWidget();
    total_bodyWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    total_bodyWidget->setMaximumHeight(174);
    total_bodyWidget->setMinimumHeight(174);
    total_bodyWidget->setMinimumWidth(385);

    v_bodylayout1->addWidget(v_emptyWidget1);

    QWidget *h_topBodyWidget = new QWidget();
    h_topBodyWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    h_topBodyWidget->setMinimumHeight(18);
    h_topBodyWidget->setMaximumHeight(18);
    h_topBodyWidget->setLayout(h_topBodyLayout);
    h_topBodyLayout->setContentsMargins(0,0,0,0);
    h_topBodyLayout->setMargin(0);

    QWidget *h_bottomBodyWidget = new QWidget();
    h_bottomBodyWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    h_bottomBodyWidget->setMinimumHeight(18);
    h_bottomBodyWidget->setMaximumHeight(18);
    h_bottomBodyWidget->setLayout(h_bottomBodyLayout);
    h_bottomBodyLayout->setContentsMargins(0,0,0,0);
    h_bottomBodyLayout->setMargin(0);

    v_bodylayout1->addWidget(v_emptyWidget1);

    v_bodylayout1->addWidget(h_topBodyWidget);
        body_label_available = new QLabel();
        body_label_available->setText("AVAILABLE: ");
        body_label_available->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        body_label_available->setMinimumSize(110, 18);
        body_label_available->setMaximumSize(110, 18);
        body_label_available->setStyleSheet("background-color: transparent; color: white;");
        h_topBodyLayout->addWidget(body_label_available);
        
        body_value_available = new QLabel();
        body_value_available->setText("0 DOGEC");
        body_value_available->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        body_value_available->setMinimumHeight(18);
        body_value_available->setMaximumHeight(18);
        body_value_available->setStyleSheet("background-color: transparent; color: #ffdfb5");
        h_topBodyLayout->addWidget(body_value_available);

        QWidget *body_empty_available = new QWidget();
        body_empty_available->setStyleSheet("background-color: transparent");
        body_empty_available->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        body_empty_available->setMinimumHeight(18);
        body_empty_available->setMaximumHeight(18);

        h_topBodyLayout->addWidget(body_empty_available);


    v_bodylayout1->addWidget(total_bodyWidget);
        total_bodyWidget->setLayout(new QVBoxLayout());
        total_bodyWidget->setStyleSheet("background-color: #343e48");
        total_bodyWidget->layout()->setContentsMargins(65, 0, 65, 0);
        total_bodyWidget->layout()->setSpacing(7);
        total_bodyWidget->layout()->setMargin(0);

        body_label_total = new QLabel();
        body_label_total->setText("TOTAL DOGEC");
        body_label_total->setStyleSheet("background-color: transparent; color: white; margin-top: 10px");
        body_label_total->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        body_label_total->setMinimumHeight(35);
        body_label_total->setMaximumHeight(35);
        body_label_total->setAlignment(Qt::AlignCenter);

        total_bodyWidget->layout()->addWidget(body_label_total);

        body_value_total = new QLabel();
        body_value_total->setStyleSheet("background-color: transparent; color: #ffdfb5;");
        body_value_total->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        body_value_total->setMinimumHeight(100);
        body_value_total->setMinimumHeight(100);
        body_value_total->setText(QString::number(0));
        body_value_total->setAlignment(Qt::AlignCenter);
        
        total_bodyWidget->layout()->addWidget(body_value_total);

        QWidget *body_empty_total = new QWidget();
        body_empty_total->setStyleSheet("background-color: transparent");
        body_empty_total->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        
        total_bodyWidget->layout()->addWidget(body_empty_total);


    v_bodylayout1->addWidget(h_bottomBodyWidget);
        QWidget *body_empty_immature = new QWidget();
        
        body_empty_immature->setStyleSheet("background-color: transparent");
        body_empty_immature->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        body_empty_immature->setMinimumHeight(18);
        body_empty_immature->setMaximumHeight(18);
        h_bottomBodyLayout->addWidget(body_empty_immature);

        body_label_immature = new QLabel();
        body_label_immature->setText("IMMATURE: ");
        body_label_immature->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        body_label_immature->setMinimumSize(100, 18);
        body_label_immature->setMaximumSize(100, 18);
        body_label_immature->setStyleSheet("background-color: transparent; color: white;");
        h_bottomBodyLayout->addWidget(body_label_immature);

        body_value_immature = new QLabel();
        body_value_immature->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        body_value_immature->setStyleSheet("background-color: transparent; color: #ffdfb5");
        body_value_immature->setMinimumHeight(18);
        body_value_immature->setMaximumHeight(18);
        body_value_immature->setText(QString::number(0) + " DOGEC" );

        h_bottomBodyLayout->addWidget(body_value_immature);

    v_bodylayout1->addWidget(v_emptyWidget2);


    body_widget->show();
    body_widget->raise();
    line_widget->lower();
}

void OverviewPage::resizeEvent(QResizeEvent *event)
{
    main_widget->setGeometry(0, 160, this->width(), this->height()-190);
    line_widget->setGeometry(0,0,main_widget->width(), main_widget->height());    
    line_label->setMinimumWidth(line_widget->height());
    line_label->setMaximumWidth(line_widget->height());

    body_widget->setGeometry(0, 0, main_widget->width(), main_widget->height());
    body_widget->raise();
    line_widget->lower();
}

void OverviewPage::showWidget()
{
    main_widget->show();
    line_widget->show();
    body_widget->show();
    
}

void OverviewPage::setFontOverViewPage()
{
    font_label = new QFont();
    font_label->setFamily("Chivo");
    font_label->setLetterSpacing(QFont::AbsoluteSpacing, 1.4);
    font_label->setBold(true);
    font_label->setPixelSize(15);

    font_value = new QFont();
    font_value->setFamily("Chapaza");
    font_value->setLetterSpacing(QFont::AbsoluteSpacing, 3.8);
    font_value->setPixelSize(114);


    body_label_total->setFont(*font_label);
    body_label_immature->setFont(*font_label);
    body_label_available->setFont(*font_label);
    body_value_immature->setFont(*font_label);
    body_value_available->setFont(*font_label);
    body_value_total->setFont(*font_value);
}
