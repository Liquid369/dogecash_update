// Copyright (c) 2011-2014 The Bitcoin developers
// Copyright (c) 2014-2015 The Dash developers
// Copyright (c) 2015-2017 The PIVX developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "sendcoinsdialog.h"
#include "ui_sendcoinsdialog.h"

#include "addresstablemodel.h"
#include "askpassphrasedialog.h"
#include "bitcoinunits.h"
#include "clientmodel.h"
#include "coincontroldialog.h"
#include "guiutil.h"
#include "optionsmodel.h"
#include "sendcoinsentry.h"
#include "walletmodel.h"

#include "base58.h"
#include "coincontrol.h"
#include "ui_interface.h"
#include "utilmoneystr.h"
#include "wallet.h"

#include <QMessageBox>
#include <QScrollBar>
#include <QSettings>
#include <QTextDocument>
#include <QFont>

#include <QGraphicsBlurEffect>
#include <QScrollArea>
#include <QDebug>
#include <QGridLayout>




ToggleSwitch::ToggleSwitch(QWidget *parent) :
    QAbstractButton(parent),
    m_status(false),
    m_margin(0),
    m_bodyBrush(Qt::lightGray)
{
    this->setBrush(QColor(Qt::darkGreen));
}

ToggleSwitch::ToggleSwitch(const QBrush &brush, QWidget *parent) :
    QAbstractButton(parent),
    m_status(false),
    m_margin(0),
    m_bodyBrush(Qt::lightGray)
{

    this->setBrush(brush);
}

void ToggleSwitch::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setPen(Qt::NoPen);

    if(true == this->isEnabled())
    {
//QColor("#29333d")
        this->brush().setColor(QColor("#b6a27e"));
        painter.setBrush(this->m_status ? QColor("#b6a27e") : QColor("#29333d"));
        painter.setOpacity(1);
        painter.setRenderHint(QPainter::Antialiasing, true);

        painter.drawRoundedRect(QRect(0, 0, this->width(),
            this->height()), 14, 14);

        painter.setBrush(this->m_bodyBrush);
        painter.setOpacity(1);
        QFont *font = new QFont();
        font->setFamily("Chivo");
        font->setPixelSize(13);
        font->setLetterSpacing(QFont::AbsoluteSpacing, 1.2);
        painter.setBrush(Qt::white);
        if(true == this->m_status)
        {
            painter.setPen(QColor("#f2f2f2"));
            painter.drawEllipse(this->width() - this->height()+1 , (this->height() / 2) - (this->height() / 2) + 2,
                this->height()-4, this->height()-4);
            painter.setFont(*font);
            painter.drawText(8,19,"YES");

        }
        else
        {
            painter.setPen(QColor("#526475"));
            painter.drawEllipse(QRectF(2, (this->height() / 2) - (this->height() / 2) + 2, this->height()-4, this->height()-4));
            painter.setFont(*font);
            painter.drawText(37,19, "NO");
        }
    }
    else
    {
        painter.setBrush(Qt::black);
        painter.setOpacity(0.12);

        painter.drawRoundedRect(QRect(0, 0, this->width(),
                                      this->height()), 16, 16);

        painter.setOpacity(1.0);
//        painter.setBrush(QColor("#BDBDBD"));
        painter.setBrush(Qt::blue);


        painter.drawEllipse(QRectF((this->width() / 2), (this->height() / 2) - (this->height() / 2),
            this->height(), this->height()));
    }
}

void ToggleSwitch::mouseReleaseEvent(QMouseEvent *event)
{
    if(event->button() & Qt::LeftButton)
    {
        this->m_status = !this->m_status;
        emit ToggleSwitch::statusChanged(this->m_status);
    }

    QAbstractButton::mouseReleaseEvent(event);
}

QSize ToggleSwitch::sizeHint(void) const
{
    return QSize(this->width() + (2 * this->m_margin), this->height() + (2 * this->m_margin));
}



SendCoinsDialog::SendCoinsDialog(QWidget* parent) : QDialog(parent),
                                                    ui(new Ui::SendCoinsDialog),
                                                    clientModel(0),
                                                    model(0),
                                                    fNewRecipientAllowed(true),
                                                    fFeeMinimized(true)
{
    ui->setupUi(this);

#ifdef Q_OS_MAC // Icons on push buttons are very uncommon on Mac
    ui->addButton->setIcon(QIcon());
    ui->clearButton->setIcon(QIcon());
    ui->sendButton->setIcon(QIcon());
#endif

    GUIUtil::setupAddressWidget(ui->lineEditCoinControlChange, this);



    addEntry();

    connect(ui->addButton, SIGNAL(clicked()), this, SLOT(addEntry()));
    connect(ui->clearButton, SIGNAL(clicked()), this, SLOT(clear()));

    // Coin Control
    connect(ui->pushButtonCoinControl, SIGNAL(clicked()), this, SLOT(coinControlButtonClicked()));
    connect(ui->checkBoxCoinControlChange, SIGNAL(stateChanged(int)), this, SLOT(coinControlChangeChecked(int)));
    connect(ui->lineEditCoinControlChange, SIGNAL(textEdited(const QString&)), this, SLOT(coinControlChangeEdited(const QString&)));

    // UTXO Splitter
    connect(ui->splitBlockCheckBox, SIGNAL(stateChanged(int)), this, SLOT(splitBlockChecked(int)));
    connect(ui->splitBlockLineEdit, SIGNAL(textChanged(const QString&)), this, SLOT(splitBlockLineEditChanged(const QString&)));

    // DogeCash specific
    QSettings settings;
    if (!settings.contains("bUseObfuScation"))
        settings.setValue("bUseObfuScation", false);
    if (!settings.contains("bUseSwiftTX"))
        settings.setValue("bUseSwiftTX", false);

    bool useSwiftTX = settings.value("bUseSwiftTX").toBool();
    if (fLiteMode) {
        ui->checkSwiftTX->setVisible(false);
        CoinControlDialog::coinControl->useObfuScation = false;
        CoinControlDialog::coinControl->useSwiftTX = false;
    } else {
        ui->checkSwiftTX->setChecked(useSwiftTX);
        CoinControlDialog::coinControl->useSwiftTX = useSwiftTX;
    }

    connect(ui->checkSwiftTX, SIGNAL(stateChanged(int)), this, SLOT(updateSwiftTX()));

    // Coin Control: clipboard actions
    QAction* clipboardQuantityAction = new QAction(tr("Copy quantity"), this);
    QAction* clipboardAmountAction = new QAction(tr("Copy amount"), this);
    QAction* clipboardFeeAction = new QAction(tr("Copy fee"), this);
    QAction* clipboardAfterFeeAction = new QAction(tr("Copy after fee"), this);
    QAction* clipboardBytesAction = new QAction(tr("Copy bytes"), this);
    QAction* clipboardPriorityAction = new QAction(tr("Copy priority"), this);
    QAction* clipboardLowOutputAction = new QAction(tr("Copy dust"), this);
    QAction* clipboardChangeAction = new QAction(tr("Copy change"), this);
    connect(clipboardQuantityAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardQuantity()));
    connect(clipboardAmountAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardAmount()));
    connect(clipboardFeeAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardFee()));
    connect(clipboardAfterFeeAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardAfterFee()));
    connect(clipboardBytesAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardBytes()));
    connect(clipboardPriorityAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardPriority()));
    connect(clipboardLowOutputAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardLowOutput()));
    connect(clipboardChangeAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardChange()));
    ui->labelCoinControlQuantity->addAction(clipboardQuantityAction);
    ui->labelCoinControlAmount->addAction(clipboardAmountAction);
    ui->labelCoinControlFee->addAction(clipboardFeeAction);
    ui->labelCoinControlAfterFee->addAction(clipboardAfterFeeAction);
    ui->labelCoinControlBytes->addAction(clipboardBytesAction);
    ui->labelCoinControlPriority->addAction(clipboardPriorityAction);
    ui->labelCoinControlLowOutput->addAction(clipboardLowOutputAction);
    ui->labelCoinControlChange->addAction(clipboardChangeAction);

    // init transaction fee section
    if (!settings.contains("fFeeSectionMinimized"))
        settings.setValue("fFeeSectionMinimized", true);
    if (!settings.contains("nFeeRadio") && settings.contains("nTransactionFee") && settings.value("nTransactionFee").toLongLong() > 0) // compatibility
        settings.setValue("nFeeRadio", 1);                                                                                             // custom
    if (!settings.contains("nFeeRadio"))
        settings.setValue("nFeeRadio", 0);                                                                                                   // recommended
    if (!settings.contains("nCustomFeeRadio") && settings.contains("nTransactionFee") && settings.value("nTransactionFee").toLongLong() > 0) // compatibility
        settings.setValue("nCustomFeeRadio", 1);                                                                                             // total at least
    if (!settings.contains("nCustomFeeRadio"))
        settings.setValue("nCustomFeeRadio", 0); // per kilobyte
    if (!settings.contains("nSmartFeeSliderPosition"))
        settings.setValue("nSmartFeeSliderPosition", 0);
    if (!settings.contains("nTransactionFee"))
        settings.setValue("nTransactionFee", (qint64)DEFAULT_TRANSACTION_FEE);
    if (!settings.contains("fPayOnlyMinFee"))
        settings.setValue("fPayOnlyMinFee", false);
    if (!settings.contains("fSendFreeTransactions"))
        settings.setValue("fSendFreeTransactions", false);

    ui->groupFee->setId(ui->radioSmartFee, 0);
    ui->groupFee->setId(ui->radioCustomFee, 1);
    ui->groupFee->button((int)std::max(0, std::min(1, settings.value("nFeeRadio").toInt())))->setChecked(true);
    ui->groupCustomFee->setId(ui->radioCustomPerKilobyte, 0);
    ui->groupCustomFee->setId(ui->radioCustomAtLeast, 1);
    ui->groupCustomFee->button((int)std::max(0, std::min(1, settings.value("nCustomFeeRadio").toInt())))->setChecked(true);
    ui->sliderSmartFee->setValue(settings.value("nSmartFeeSliderPosition").toInt());
    ui->customFee->setValue(settings.value("nTransactionFee").toLongLong());
    ui->checkBoxMinimumFee->setChecked(settings.value("fPayOnlyMinFee").toBool());
    ui->checkBoxFreeTx->setChecked(settings.value("fSendFreeTransactions").toBool());
    ui->checkzDOGEC->hide();
    minimizeFeeSection(settings.value("fFeeSectionMinimized").toBool());

    ui->frameCoinControl->hide();
    ui->frameFee->hide();
    ui->scrollArea->hide();
    ui->addButton->hide();
    ui->clearButton->hide();
    ui->sendButton->hide();
    ui->checkSwiftTX->hide();
    ui->checkzDOGEC->hide();
    ui->label->hide();
    ui->labelBalance->hide();

    setFont();
    setBackPage();
    setFrontPage();
    setAdvancePage();
    loadDialog();
}

void SendCoinsDialog::setClientModel(ClientModel* clientModel)
{
    this->clientModel = clientModel;

    if (clientModel) {
        connect(clientModel, SIGNAL(numBlocksChanged(int)), this, SLOT(updateSmartFeeLabel()));
    }
}

void SendCoinsDialog::setModel(WalletModel* model)
{
    this->model = model;

    if (model && model->getOptionsModel()) {
        for (int i = 0; i < ui->entries->count(); ++i) {
            SendCoinsEntry* entry = qobject_cast<SendCoinsEntry*>(ui->entries->itemAt(i)->widget());
            if (entry) {
                entry->setModel(model);
            }
        }

        setBalance(model->getBalance(), model->getUnconfirmedBalance(), model->getImmatureBalance(),
                   model->getZerocoinBalance (), model->getUnconfirmedZerocoinBalance (), model->getImmatureZerocoinBalance (),
                   model->getWatchBalance(), model->getWatchUnconfirmedBalance(), model->getWatchImmatureBalance());
        connect(model, SIGNAL(balanceChanged(CAmount, CAmount,  CAmount, CAmount, CAmount, CAmount, CAmount, CAmount, CAmount)), this,
                         SLOT(setBalance(CAmount, CAmount, CAmount, CAmount, CAmount, CAmount, CAmount, CAmount, CAmount)));
        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));
        updateDisplayUnit();

        // Coin Control
        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(coinControlUpdateLabels()));
        connect(model->getOptionsModel(), SIGNAL(coinControlFeaturesChanged(bool)), this, SLOT(coinControlFeatureChanged(bool)));
        ui->frameCoinControl->setVisible(model->getOptionsModel()->getCoinControlFeatures());
        coinControlUpdateLabels();

        // fee section
        connect(ui->sliderSmartFee, SIGNAL(valueChanged(int)), this, SLOT(updateSmartFeeLabel()));
        connect(ui->sliderSmartFee, SIGNAL(valueChanged(int)), this, SLOT(updateGlobalFeeVariables()));
        connect(ui->sliderSmartFee, SIGNAL(valueChanged(int)), this, SLOT(coinControlUpdateLabels()));
        connect(ui->groupFee, SIGNAL(buttonClicked(int)), this, SLOT(updateFeeSectionControls()));
        connect(ui->groupFee, SIGNAL(buttonClicked(int)), this, SLOT(updateGlobalFeeVariables()));
        connect(ui->groupFee, SIGNAL(buttonClicked(int)), this, SLOT(coinControlUpdateLabels()));
        connect(ui->groupCustomFee, SIGNAL(buttonClicked(int)), this, SLOT(updateGlobalFeeVariables()));
        connect(ui->groupCustomFee, SIGNAL(buttonClicked(int)), this, SLOT(coinControlUpdateLabels()));
        connect(ui->customFee, SIGNAL(valueChanged()), this, SLOT(updateGlobalFeeVariables()));
        connect(ui->customFee, SIGNAL(valueChanged()), this, SLOT(coinControlUpdateLabels()));
        connect(ui->checkBoxMinimumFee, SIGNAL(stateChanged(int)), this, SLOT(setMinimumFee()));
        connect(ui->checkBoxMinimumFee, SIGNAL(stateChanged(int)), this, SLOT(updateFeeSectionControls()));
        connect(ui->checkBoxMinimumFee, SIGNAL(stateChanged(int)), this, SLOT(updateGlobalFeeVariables()));
        connect(ui->checkBoxMinimumFee, SIGNAL(stateChanged(int)), this, SLOT(coinControlUpdateLabels()));
        connect(ui->checkBoxFreeTx, SIGNAL(stateChanged(int)), this, SLOT(updateGlobalFeeVariables()));
        connect(ui->checkBoxFreeTx, SIGNAL(stateChanged(int)), this, SLOT(coinControlUpdateLabels()));
        ui->customFee->setSingleStep(CWallet::minTxFee.GetFeePerK());
        updateFeeSectionControls();
        updateMinFeeLabel();
        updateSmartFeeLabel();
        updateGlobalFeeVariables();
    }
}



SendCoinsDialog::~SendCoinsDialog()
{
    QSettings settings;
    settings.setValue("fFeeSectionMinimized", fFeeMinimized);
    settings.setValue("nFeeRadio", ui->groupFee->checkedId());
    settings.setValue("nCustomFeeRadio", ui->groupCustomFee->checkedId());
    settings.setValue("nSmartFeeSliderPosition", ui->sliderSmartFee->value());
    settings.setValue("nTransactionFee", (qint64)ui->customFee->value());
    settings.setValue("fPayOnlyMinFee", ui->checkBoxMinimumFee->isChecked());
    settings.setValue("fSendFreeTransactions", ui->checkBoxFreeTx->isChecked());

    delete ui;
}

void SendCoinsDialog::on_sendButton_clicked()
{
    if (!model || !model->getOptionsModel())
        return;

    QList<SendCoinsRecipient> recipients;
    bool valid = true;

    for (int i = 0; i < ui->entries->count(); ++i) {
        SendCoinsEntry* entry = qobject_cast<SendCoinsEntry*>(ui->entries->itemAt(i)->widget());

        //UTXO splitter - address should be our own
        CBitcoinAddress address = entry->getValue().address.toStdString();
        if (!model->isMine(address) && ui->splitBlockCheckBox->checkState() == Qt::Checked) {
            CoinControlDialog::coinControl->fSplitBlock = false;
            ui->splitBlockCheckBox->setCheckState(Qt::Unchecked);
            QMessageBox::warning(this, tr("Send Coins"),
                tr("The split block tool does not work when sending to outside addresses. Try again."),
                QMessageBox::Ok, QMessageBox::Ok);
            return;
        }

        if (entry) {
            if (entry->validate()) {
                recipients.append(entry->getValue());
            } else {
                valid = false;
            }
        }
    }

    if (!valid || recipients.isEmpty()) {
        return;
    }

    //set split block in model
    CoinControlDialog::coinControl->fSplitBlock = ui->splitBlockCheckBox->checkState() == Qt::Checked;

    if (ui->entries->count() > 1 && ui->splitBlockCheckBox->checkState() == Qt::Checked) {
        CoinControlDialog::coinControl->fSplitBlock = false;
        ui->splitBlockCheckBox->setCheckState(Qt::Unchecked);
        QMessageBox::warning(this, tr("Send Coins"),
            tr("The split block tool does not work with multiple addresses. Try again."),
            QMessageBox::Ok, QMessageBox::Ok);
        return;
    }

    if (CoinControlDialog::coinControl->fSplitBlock)
        CoinControlDialog::coinControl->nSplitBlock = int(ui->splitBlockLineEdit->text().toInt());

    QString strFunds = tr("using") + " <b>" + tr("anonymous funds") + "</b>";
    QString strFee = "";
    recipients[0].inputType = ALL_COINS;
    strFunds = tr("using") + " <b>" + tr("any available funds (not recommended)") + "</b>";

    if (ui->checkSwiftTX->isChecked()) {
        recipients[0].useSwiftTX = true;
        strFunds += " ";
        strFunds += tr("and SwiftX");
    } else {
        recipients[0].useSwiftTX = false;
    }


    // Format confirmation message
    QStringList formatted;
    foreach (const SendCoinsRecipient& rcp, recipients) {
        // generate bold amount string
        QString amount = "<b>" + BitcoinUnits::formatHtmlWithUnit(model->getOptionsModel()->getDisplayUnit(), rcp.amount);
        amount.append("</b> ").append(strFunds);

        // generate monospace address string
        QString address = "<span style='font-family: monospace;'>" + rcp.address;
        address.append("</span>");

        QString recipientElement;

        if (!rcp.paymentRequest.IsInitialized()) // normal payment
        {
            if (rcp.label.length() > 0) // label with address
            {
                recipientElement = tr("%1 to %2").arg(amount, GUIUtil::HtmlEscape(rcp.label));
                recipientElement.append(QString(" (%1)").arg(address));
            } else // just address
            {
                recipientElement = tr("%1 to %2").arg(amount, address);
            }
        } else if (!rcp.authenticatedMerchant.isEmpty()) // secure payment request
        {
            recipientElement = tr("%1 to %2").arg(amount, GUIUtil::HtmlEscape(rcp.authenticatedMerchant));
        } else // insecure payment request
        {
            recipientElement = tr("%1 to %2").arg(amount, address);
        }

        if (fSplitBlock) {
            recipientElement.append(tr(" split into %1 outputs using the UTXO splitter.").arg(CoinControlDialog::coinControl->nSplitBlock));
        }

        formatted.append(recipientElement);
    }

    fNewRecipientAllowed = false;

    // request unlock only if was locked or unlocked for mixing:
    // this way we let users unlock by walletpassphrase or by menu
    // and make many transactions while unlocking through this dialog
    // will call relock
    WalletModel::EncryptionStatus encStatus = model->getEncryptionStatus();
    if (encStatus == model->Locked || encStatus == model->UnlockedForAnonymizationOnly) {
        WalletModel::UnlockContext ctx(model->requestUnlock(true));
        if (!ctx.isValid()) {
            // Unlock wallet was cancelled
            fNewRecipientAllowed = true;
            return;
        }
        send(recipients, strFee, formatted);
        return;
    }
    // already unlocked or not encrypted at all
    send(recipients, strFee, formatted);
}

void SendCoinsDialog::send(QList<SendCoinsRecipient> recipients, QString strFee, QStringList formatted)
{
    // prepare transaction for getting txFee earlier
    WalletModelTransaction currentTransaction(recipients);
    WalletModel::SendCoinsReturn prepareStatus;
    if (model->getOptionsModel()->getCoinControlFeatures()) // coin control enabled
        prepareStatus = model->prepareTransaction(currentTransaction, CoinControlDialog::coinControl);
    else
        prepareStatus = model->prepareTransaction(currentTransaction);

    // process prepareStatus and on error generate message shown to user
    processSendCoinsReturn(prepareStatus,
        BitcoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), currentTransaction.getTransactionFee()), true);

    if (prepareStatus.status != WalletModel::OK) {
        fNewRecipientAllowed = true;
        return;
    }

    CAmount txFee = currentTransaction.getTransactionFee();
    QString questionString = tr("Are you sure you want to send?");
    questionString.append("<br /><br />%1");

    if (txFee > 0) {
        // append fee string if a fee is required
        questionString.append("<hr /><span style='color:#aa0000;'>");
        questionString.append(BitcoinUnits::formatHtmlWithUnit(model->getOptionsModel()->getDisplayUnit(), txFee));
        questionString.append("</span> ");
        questionString.append(tr("are added as transaction fee"));
        questionString.append(" ");
        questionString.append(strFee);

        // append transaction size
        questionString.append(" (" + QString::number((double)currentTransaction.getTransactionSize() / 1000) + " kB)");
    }

    // add total amount in all subdivision units
    questionString.append("<hr />");
    CAmount totalAmount = currentTransaction.getTotalTransactionAmount() + txFee;
    QStringList alternativeUnits;
    foreach (BitcoinUnits::Unit u, BitcoinUnits::availableUnits()) {
        if (u != model->getOptionsModel()->getDisplayUnit())
            alternativeUnits.append(BitcoinUnits::formatHtmlWithUnit(u, totalAmount));
    }

    // Show total amount + all alternative units
    questionString.append(tr("Total Amount = <b>%1</b><br />= %2")
                              .arg(BitcoinUnits::formatHtmlWithUnit(model->getOptionsModel()->getDisplayUnit(), totalAmount))
                              .arg(alternativeUnits.join("<br />= ")));

    // Limit number of displayed entries
    int messageEntries = formatted.size();
    int displayedEntries = 0;
    for (int i = 0; i < formatted.size(); i++) {
        if (i >= MAX_SEND_POPUP_ENTRIES) {
            formatted.removeLast();
            i--;
        } else {
            displayedEntries = i + 1;
        }
    }
    questionString.append("<hr />");
    questionString.append(tr("<b>(%1 of %2 entries displayed)</b>").arg(displayedEntries).arg(messageEntries));

    // Display message box
    QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm send coins"),
        questionString.arg(formatted.join("<br />")),
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Cancel);

    if (retval != QMessageBox::Yes) {
        fNewRecipientAllowed = true;
        return;
    }

    // now send the prepared transaction
    WalletModel::SendCoinsReturn sendStatus = model->sendCoins(currentTransaction);
    // process sendStatus and on error generate message shown to user
    processSendCoinsReturn(sendStatus);

    if (sendStatus.status == WalletModel::OK) {
        accept();
        CoinControlDialog::coinControl->UnSelectAll();
        coinControlUpdateLabels();
    }
    fNewRecipientAllowed = true;
}

void SendCoinsDialog::clear()
{
    // Remove entries until only one left
    while (ui->entries->count()) {
        ui->entries->takeAt(0)->widget()->deleteLater();
    }
    addEntry();

    updateTabsAndLabels();
}

void SendCoinsDialog::reject()
{
    clear();
}

void SendCoinsDialog::accept()
{
    clear();
}

SendCoinsEntry* SendCoinsDialog::addEntry()
{
    SendCoinsEntry* entry = new SendCoinsEntry(this);
    entry->setModel(model);
    ui->entries->addWidget(entry);
    connect(entry, SIGNAL(removeEntry(SendCoinsEntry*)), this, SLOT(removeEntry(SendCoinsEntry*)));
    connect(entry, SIGNAL(payAmountChanged()), this, SLOT(coinControlUpdateLabels()));

    updateTabsAndLabels();

    // Focus the field, so that entry can start immediately
    entry->clear();
    entry->setFocus();
    ui->scrollAreaWidgetContents->resize(ui->scrollAreaWidgetContents->sizeHint());
    qApp->processEvents();
    QScrollBar* bar = ui->scrollArea->verticalScrollBar();
    if (bar)
        bar->setSliderPosition(bar->maximum());
    return entry;
}

void SendCoinsDialog::updateTabsAndLabels()
{
    setupTabChain(0);
    coinControlUpdateLabels();
}

void SendCoinsDialog::removeEntry(SendCoinsEntry* entry)
{
    entry->hide();

    // If the last entry is about to be removed add an empty one
    if (ui->entries->count() == 1)
        addEntry();

    entry->deleteLater();

    updateTabsAndLabels();
}

QWidget* SendCoinsDialog::setupTabChain(QWidget* prev)
{
    for (int i = 0; i < ui->entries->count(); ++i) {
        SendCoinsEntry* entry = qobject_cast<SendCoinsEntry*>(ui->entries->itemAt(i)->widget());
        if (entry) {
            prev = entry->setupTabChain(prev);
        }
    }
    QWidget::setTabOrder(prev, ui->sendButton);
    QWidget::setTabOrder(ui->sendButton, ui->clearButton);
    QWidget::setTabOrder(ui->clearButton, ui->addButton);
    return ui->addButton;
}

void SendCoinsDialog::setAddress(const QString& address)
{
    SendCoinsEntry* entry = 0;
    // Replace the first entry if it is still unused
    if (ui->entries->count() == 1) {
        SendCoinsEntry* first = qobject_cast<SendCoinsEntry*>(ui->entries->itemAt(0)->widget());
        if (first->isClear()) {
            entry = first;
        }
    }
    if (!entry) {
        entry = addEntry();
    }

    entry->setAddress(address);
}

void SendCoinsDialog::pasteEntry(const SendCoinsRecipient& rv)
{
    if (!fNewRecipientAllowed)
        return;

    SendCoinsEntry* entry = 0;
    // Replace the first entry if it is still unused
    if (ui->entries->count() == 1) {
        SendCoinsEntry* first = qobject_cast<SendCoinsEntry*>(ui->entries->itemAt(0)->widget());
        if (first->isClear()) {
            entry = first;
        }
    }
    if (!entry) {
        entry = addEntry();
    }

    entry->setValue(rv);
    updateTabsAndLabels();
}

bool SendCoinsDialog::handlePaymentRequest(const SendCoinsRecipient& rv)
{
    // Just paste the entry, all pre-checks
    // are done in paymentserver.cpp.
    pasteEntry(rv);
    return true;
}

void SendCoinsDialog::setBalance(const CAmount& balance, const CAmount& unconfirmedBalance, const CAmount& immatureBalance,
                                 const CAmount& zerocoinBalance, const CAmount& unconfirmedZerocoinBalance, const CAmount& immatureZerocoinBalance,
                                 const CAmount& watchBalance, const CAmount& watchUnconfirmedBalance, const CAmount& watchImmatureBalance)
{
    Q_UNUSED(unconfirmedBalance);
    Q_UNUSED(immatureBalance);
    Q_UNUSED(zerocoinBalance);
    Q_UNUSED(unconfirmedZerocoinBalance);
    Q_UNUSED(immatureZerocoinBalance);
    Q_UNUSED(watchBalance);
    Q_UNUSED(watchUnconfirmedBalance);
    Q_UNUSED(watchImmatureBalance);

    if (model && model->getOptionsModel()) {
        uint64_t bal = 0;
        bal = balance;
        ui->labelBalance->setText(BitcoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), bal));
    }
}

void SendCoinsDialog::showDialog()
{
    ControlDialog->show();
    front_Widget->hide();
}

void SendCoinsDialog::showAdvanced()
{
    advance_Widget->show();
    front_Widget->hide();
}


void SendCoinsDialog::updateDisplayUnit()
{
    TRY_LOCK(cs_main, lockMain);
    if (!lockMain) return;

    setBalance(model->getBalance(), model->getUnconfirmedBalance(), model->getImmatureBalance(),
               model->getZerocoinBalance (), model->getUnconfirmedZerocoinBalance (), model->getImmatureZerocoinBalance (),
               model->getWatchBalance(), model->getWatchUnconfirmedBalance(), model->getWatchImmatureBalance());
    coinControlUpdateLabels();
    ui->customFee->setDisplayUnit(model->getOptionsModel()->getDisplayUnit());
    updateMinFeeLabel();
    updateSmartFeeLabel();
}

void SendCoinsDialog::updateSwiftTX()
{
    QSettings settings;
    settings.setValue("bUseSwiftTX", ui->checkSwiftTX->isChecked());
    CoinControlDialog::coinControl->useSwiftTX = ui->checkSwiftTX->isChecked();
    coinControlUpdateLabels();
}

void SendCoinsDialog::processSendCoinsReturn(const WalletModel::SendCoinsReturn& sendCoinsReturn, const QString& msgArg, bool fPrepare)
{
    bool fAskForUnlock = false;
    
    QPair<QString, CClientUIInterface::MessageBoxFlags> msgParams;
    // Default to a warning message, override if error message is needed
    msgParams.second = CClientUIInterface::MSG_WARNING;

    // This comment is specific to SendCoinsDialog usage of WalletModel::SendCoinsReturn.
    // WalletModel::TransactionCommitFailed is used only in WalletModel::sendCoins()
    // all others are used only in WalletModel::prepareTransaction()
    switch (sendCoinsReturn.status) {
    case WalletModel::InvalidAddress:
        msgParams.first = tr("The recipient address is not valid, please recheck.");
        break;
    case WalletModel::InvalidAmount:
        msgParams.first = tr("The amount to pay must be larger than 0.");
        break;
    case WalletModel::AmountExceedsBalance:
        msgParams.first = tr("The amount exceeds your balance.");
        break;
    case WalletModel::AmountWithFeeExceedsBalance:
        msgParams.first = tr("The total exceeds your balance when the %1 transaction fee is included.").arg(msgArg);
        break;
    case WalletModel::DuplicateAddress:
        msgParams.first = tr("Duplicate address found, can only send to each address once per send operation.");
        break;
    case WalletModel::TransactionCreationFailed:
        msgParams.first = tr("Transaction creation failed!");
        msgParams.second = CClientUIInterface::MSG_ERROR;
        break;
    case WalletModel::TransactionCommitFailed:
        msgParams.first = tr("The transaction was rejected! This might happen if some of the coins in your wallet were already spent, such as if you used a copy of wallet.dat and coins were spent in the copy but not marked as spent here.");
        msgParams.second = CClientUIInterface::MSG_ERROR;
        break;
    case WalletModel::AnonymizeOnlyUnlocked:
        // Unlock is only need when the coins are send
        if(!fPrepare)
            fAskForUnlock = true;
        else
            msgParams.first = tr("Error: The wallet was unlocked only to anonymize coins.");
        break;

    case WalletModel::InsaneFee:
        msgParams.first = tr("A fee %1 times higher than %2 per kB is considered an insanely high fee.").arg(10000).arg(BitcoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), ::minRelayTxFee.GetFeePerK()));
        break;
    // included to prevent a compiler warning.
    case WalletModel::OK:
    default:
        return;
    }

    // Unlock wallet if it wasn't fully unlocked already
    if(fAskForUnlock) {
        model->requestUnlock(false);
        if(model->getEncryptionStatus () != WalletModel::Unlocked) {
            msgParams.first = tr("Error: The wallet was unlocked only to anonymize coins. Unlock canceled.");
        }
        else {
            // Wallet unlocked
            return;
        }
    }

    emit message(tr("Send Coins"), msgParams.first, msgParams.second);
}

void SendCoinsDialog::minimizeFeeSection(bool fMinimize)
{
    ui->labelFeeMinimized->setVisible(fMinimize);
    ui->buttonChooseFee->setVisible(fMinimize);
    ui->buttonMinimizeFee->setVisible(!fMinimize);
    ui->frameFeeSelection->setVisible(!fMinimize);
    ui->horizontalLayoutSmartFee->setContentsMargins(0, (fMinimize ? 0 : 6), 0, 0);
    fFeeMinimized = fMinimize;
}

void SendCoinsDialog::on_buttonChooseFee_clicked()
{
    minimizeFeeSection(false);
}

void SendCoinsDialog::on_buttonMinimizeFee_clicked()
{
    updateFeeMinimizedLabel();
    minimizeFeeSection(true);
}

void SendCoinsDialog::setMinimumFee()
{
    ui->radioCustomPerKilobyte->setChecked(true);
    ui->customFee->setValue(CWallet::minTxFee.GetFeePerK());
}

void SendCoinsDialog::updateFeeSectionControls()
{
    ui->sliderSmartFee->setEnabled(ui->radioSmartFee->isChecked());
    ui->labelSmartFee->setEnabled(ui->radioSmartFee->isChecked());
    ui->labelSmartFee2->setEnabled(ui->radioSmartFee->isChecked());
    ui->labelSmartFee3->setEnabled(ui->radioSmartFee->isChecked());
    ui->labelFeeEstimation->setEnabled(ui->radioSmartFee->isChecked());
    ui->labelSmartFeeNormal->setEnabled(ui->radioSmartFee->isChecked());
    ui->labelSmartFeeFast->setEnabled(ui->radioSmartFee->isChecked());
    ui->checkBoxMinimumFee->setEnabled(ui->radioCustomFee->isChecked());
    ui->labelMinFeeWarning->setEnabled(ui->radioCustomFee->isChecked());
    ui->radioCustomPerKilobyte->setEnabled(ui->radioCustomFee->isChecked() && !ui->checkBoxMinimumFee->isChecked());
    ui->radioCustomAtLeast->setEnabled(ui->radioCustomFee->isChecked() && !ui->checkBoxMinimumFee->isChecked());
    ui->customFee->setEnabled(ui->radioCustomFee->isChecked() && !ui->checkBoxMinimumFee->isChecked());
}

void SendCoinsDialog::updateGlobalFeeVariables()
{
    if (ui->radioSmartFee->isChecked()) {
        nTxConfirmTarget = (int)25 - (int)std::max(0, std::min(24, ui->sliderSmartFee->value()));
        payTxFee = CFeeRate(0);
    } else {
        nTxConfirmTarget = 25;
        payTxFee = CFeeRate(ui->customFee->value());
        fPayAtLeastCustomFee = ui->radioCustomAtLeast->isChecked();
    }

    fSendFreeTransactions = ui->checkBoxFreeTx->isChecked();
}

void SendCoinsDialog::updateFeeMinimizedLabel()
{
    if (!model || !model->getOptionsModel())
        return;

    if (ui->radioSmartFee->isChecked())
        ui->labelFeeMinimized->setText(ui->labelSmartFee->text());
    else {
        ui->labelFeeMinimized->setText(BitcoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), ui->customFee->value()) +
                                       ((ui->radioCustomPerKilobyte->isChecked()) ? "/kB" : ""));
    }
}

void SendCoinsDialog::updateMinFeeLabel()
{
    if (model && model->getOptionsModel())
        ui->checkBoxMinimumFee->setText(tr("Pay only the minimum fee of %1").arg(BitcoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), CWallet::minTxFee.GetFeePerK()) + "/kB"));
}

void SendCoinsDialog::updateSmartFeeLabel()
{
    if (!model || !model->getOptionsModel())
        return;

    int nBlocksToConfirm = (int)25 - (int)std::max(0, std::min(24, ui->sliderSmartFee->value()));
    CFeeRate feeRate = mempool.estimateFee(nBlocksToConfirm);
    if (feeRate <= CFeeRate(0)) // not enough data => minfee
    {
        ui->labelSmartFee->setText(BitcoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), CWallet::minTxFee.GetFeePerK()) + "/kB");
        ui->labelSmartFee2->show(); // (Smart fee not initialized yet. This usually takes a few blocks...)
        ui->labelFeeEstimation->setText("");
    } else {
        ui->labelSmartFee->setText(BitcoinUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), feeRate.GetFeePerK()) + "/kB");
        ui->labelSmartFee2->hide();
        ui->labelFeeEstimation->setText(tr("Estimated to begin confirmation within %n block(s).", "", nBlocksToConfirm));
    }

    updateFeeMinimizedLabel();
}

// UTXO splitter
void SendCoinsDialog::splitBlockChecked(int state)
{
    if (model) {
        CoinControlDialog::coinControl->fSplitBlock = (state == Qt::Checked);
        fSplitBlock = (state == Qt::Checked);
        ui->splitBlockLineEdit->setEnabled((state == Qt::Checked));
        ui->labelBlockSizeText->setEnabled((state == Qt::Checked));
        ui->labelBlockSize->setEnabled((state == Qt::Checked));
        coinControlUpdateLabels();
    }
}

//UTXO splitter
void SendCoinsDialog::splitBlockLineEditChanged(const QString& text)
{
    //grab the amount in Coin Control AFter Fee field
    QString qAfterFee = ui->labelCoinControlAfterFee->text().left(ui->labelCoinControlAfterFee->text().indexOf(" ")).replace("~", "").simplified().replace(" ", "");

    //convert to CAmount
    CAmount nAfterFee;
    ParseMoney(qAfterFee.toStdString().c_str(), nAfterFee);

    //if greater than 0 then divide after fee by the amount of blocks
    CAmount nSize = nAfterFee;
    int nBlocks = text.toInt();
    if (nAfterFee && nBlocks)
        nSize = nAfterFee / nBlocks;

    //assign to split block dummy, which is used to recalculate the fee amount more outputs
    CoinControlDialog::nSplitBlockDummy = nBlocks;

    //update labels
    ui->labelBlockSize->setText(QString::fromStdString(FormatMoney(nSize)));
    coinControlUpdateLabels();
}

// Coin Control: copy label "Quantity" to clipboard
void SendCoinsDialog::coinControlClipboardQuantity()
{
    GUIUtil::setClipboard(ui->labelCoinControlQuantity->text());
}

// Coin Control: copy label "Amount" to clipboard
void SendCoinsDialog::coinControlClipboardAmount()
{
    GUIUtil::setClipboard(ui->labelCoinControlAmount->text().left(ui->labelCoinControlAmount->text().indexOf(" ")));
}

// Coin Control: copy label "Fee" to clipboard
void SendCoinsDialog::coinControlClipboardFee()
{
    GUIUtil::setClipboard(ui->labelCoinControlFee->text().left(ui->labelCoinControlFee->text().indexOf(" ")).replace("~", ""));
}

// Coin Control: copy label "After fee" to clipboard
void SendCoinsDialog::coinControlClipboardAfterFee()
{
    GUIUtil::setClipboard(ui->labelCoinControlAfterFee->text().left(ui->labelCoinControlAfterFee->text().indexOf(" ")).replace("~", ""));
}

// Coin Control: copy label "Bytes" to clipboard
void SendCoinsDialog::coinControlClipboardBytes()
{
    GUIUtil::setClipboard(ui->labelCoinControlBytes->text().replace("~", ""));
}

// Coin Control: copy label "Priority" to clipboard
void SendCoinsDialog::coinControlClipboardPriority()
{
    GUIUtil::setClipboard(ui->labelCoinControlPriority->text());
}

// Coin Control: copy label "Dust" to clipboard
void SendCoinsDialog::coinControlClipboardLowOutput()
{
    GUIUtil::setClipboard(ui->labelCoinControlLowOutput->text());
}

// Coin Control: copy label "Change" to clipboard
void SendCoinsDialog::coinControlClipboardChange()
{
    GUIUtil::setClipboard(ui->labelCoinControlChange->text().left(ui->labelCoinControlChange->text().indexOf(" ")).replace("~", ""));
}

// Coin Control: settings menu - coin control enabled/disabled by user
void SendCoinsDialog::coinControlFeatureChanged(bool checked)
{
    ui->frameCoinControl->setVisible(checked);

    if (!checked && model) // coin control features disabled
        CoinControlDialog::coinControl->SetNull();

    if (checked)
        coinControlUpdateLabels();
}

// Coin Control: button inputs -> show actual coin control dialog
void SendCoinsDialog::coinControlButtonClicked()
{
    CoinControlDialog dlg;
    dlg.setModel(model);
    dlg.exec();
    coinControlUpdateLabels();
}

// Coin Control: checkbox custom change address
void SendCoinsDialog::coinControlChangeChecked(int state)
{
    if (state == Qt::Unchecked) {
        CoinControlDialog::coinControl->destChange = CNoDestination();
        ui->labelCoinControlChangeLabel->clear();
    } else
        // use this to re-validate an already entered address
        coinControlChangeEdited(ui->lineEditCoinControlChange->text());

    ui->lineEditCoinControlChange->setEnabled((state == Qt::Checked));
}

// Coin Control: custom change address changed
void SendCoinsDialog::coinControlChangeEdited(const QString& text)
{
    if (model && model->getAddressTableModel()) {
        // Default to no change address until verified
        CoinControlDialog::coinControl->destChange = CNoDestination();
        ui->labelCoinControlChangeLabel->setStyleSheet("QLabel{color:red;}");

        CBitcoinAddress addr = CBitcoinAddress(text.toStdString());

        if (text.isEmpty()) // Nothing entered
        {
            ui->labelCoinControlChangeLabel->setText("");
        } else if (!addr.IsValid()) // Invalid address
        {
            ui->labelCoinControlChangeLabel->setText(tr("Warning: Invalid DogeCash address"));
        } else // Valid address
        {
            CPubKey pubkey;
            CKeyID keyid;
            addr.GetKeyID(keyid);
            if (!model->getPubKey(keyid, pubkey)) // Unknown change address
            {
                ui->labelCoinControlChangeLabel->setText(tr("Warning: Unknown change address"));
            } else // Known change address
            {
                ui->labelCoinControlChangeLabel->setStyleSheet("QLabel{color:black;}");

                // Query label
                QString associatedLabel = model->getAddressTableModel()->labelForAddress(text);
                if (!associatedLabel.isEmpty())
                    ui->labelCoinControlChangeLabel->setText(associatedLabel);
                else
                    ui->labelCoinControlChangeLabel->setText(tr("(no label)"));

                CoinControlDialog::coinControl->destChange = addr.Get();
            }
        }
    }
}

// Coin Control: update labels
void SendCoinsDialog::coinControlUpdateLabels()
{
    if (!model || !model->getOptionsModel() || !model->getOptionsModel()->getCoinControlFeatures())
        return;

    // set pay amounts
    CoinControlDialog::payAmounts.clear();
    for (int i = 0; i < ui->entries->count(); ++i) {
        SendCoinsEntry* entry = qobject_cast<SendCoinsEntry*>(ui->entries->itemAt(i)->widget());
        if (entry)
            CoinControlDialog::payAmounts.append(entry->getValue().amount);
    }

    if (CoinControlDialog::coinControl->HasSelected()) {
        // actual coin control calculation
        CoinControlDialog::updateLabels(model, this);

        // show coin control stats
        ui->labelCoinControlAutomaticallySelected->hide();
        ui->widgetCoinControl->show();
    } else {
        // hide coin control stats
        ui->labelCoinControlAutomaticallySelected->show();
        ui->widgetCoinControl->hide();
        ui->labelCoinControlInsuffFunds->hide();
    }
}
void SendCoinsDialog::resizeEvent(QResizeEvent *event)
{

    line_widget->setGeometry(0, 158, this->width(), this->height()-190);
    line_label->setMinimumWidth(line_widget->height());
    line_label->setMaximumWidth(line_widget->height());
    front_Widget->setGeometry(0, 0, this->width(), this->height()-32);
    advance_Widget->setGeometry(0, 158, this->width(), this->height()-158);
    ControlDialog->setGeometry(0, 158, this->width(), this->height()-190);
}
void SendCoinsDialog::setBackPage()
{

    line_widget = new QWidget();
    line_widget->setLayout(new QHBoxLayout);
    line_label = new QLabel();
    line_label->setPixmap(QPixmap(":/images/res/images/main-back.png"));
    line_label->setScaledContents(true);
    line_label->setMinimumWidth(line_widget->height());
    line_label->setMaximumWidth(line_widget->height());
    line_widget->layout()->addWidget(line_label);
    line_widget->setParent(this);
    line_widget->setGeometry(0, 158, this->width(), this->height()-190);
    line_widget->show();

}


void SendCoinsDialog::setFont()
{
    font_main = new QFont();
    font_main->setFamily("Chivo");
    font_main->setPixelSize(15);
    font_main->setLetterSpacing(QFont::AbsoluteSpacing, 1.4);
    font_main->setBold(true);

    font_small = new QFont();
    font_small->setFamily("Chivo");
    font_small->setPixelSize(13);
    font_small->setLetterSpacing(QFont::AbsoluteSpacing, 1.2);

    font_value = new QFont();
    font_value->setFamily("Chapaza");
    font_value->setLetterSpacing(QFont::AbsoluteSpacing, 1.5);
    font_value->setPixelSize(44);
}


void SendCoinsDialog::setFrontPage()
{
    front_Widget = new QWidget();
    front_Widget->setGeometry(0, 0, this->width(), this->height());
//    front_Widget->show();
    front_Widget->setParent(this);
//    front_Widget->setLayout(new QVBoxLayout);
    front_Widget->setStyleSheet("background-color: transparent");
    front_Widget->setLayout(new QVBoxLayout());
    front_Widget->layout()->setMargin(0);
    front_Widget->layout()->setSpacing(0);
//    front_Widget->layout()->setContentsMargins(0,0,0,0);
    front_Widget->show();
    front_Widget->raise();
    line_widget->lower();

    front_top_widget = new QWidget();
//    front_top_widget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
//    front_top_widget->setMinimumHeight(160);
//    front_top_widget->setMaximumHeight(160);
    front_top_widget->setParent(this);
    front_top_widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    front_top_widget->setMinimumHeight(158);
    front_top_widget->setMaximumHeight(158);
    front_top_widget->setStyleSheet("background-color: transparent");
    front_Widget->layout()->addWidget(front_top_widget);

    front_top_widget->setLayout(new QHBoxLayout());
    front_top_widget->layout()->setMargin(0);
    front_top_widget->layout()->setContentsMargins(52, 0, 52, 0);
//    front_top_widget->setStyleSheet("background-color: white");
//    front_Widget->show();
//    main_Widget->layout()->addWidget(front_top_widget);
    front_top_widget->layout()->setSpacing(0);
//    front_top_widget->layout()->setContentsMargins(0, 0, 0, 0);

    front_top_widget_1 = new QWidget();
    front_top_widget_1->setParent(this);
    front_top_widget_1->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    front_top_widget_1->setMinimumHeight(72);
    front_top_widget_1->setMaximumHeight(72);
    front_top_widget_1->setLayout(new QVBoxLayout());

    front_top_widget_1->setLayout(new QVBoxLayout());
    front_top_widget_1->layout()->setMargin(0);
    front_top_widget_1->layout()->setSpacing(9);

    front_label_balance = new QLabel();
    front_label_balance->setText("DOGEC BALANCE");
    front_label_balance->setStyleSheet("background-color: transparent; color: white");
    front_label_balance->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    front_label_balance->setMinimumSize(140, 18);
    front_label_balance->setMaximumSize(140, 18);
    front_label_balance->setFont(*font_main);

    front_value_balance = new QLabel();
    front_value_balance->setStyleSheet("background-color: transparent; color: #ffdfb5");
    front_value_balance->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    front_value_balance->setText("0");
    front_value_balance->setFont(*font_value);

    front_top_widget_1->layout()->addWidget(front_label_balance);
    front_top_widget_1->layout()->addWidget(front_value_balance);


    QWidget *top_empty_widget = new QWidget();
    top_empty_widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    front_button_control = new QPushButton();
    front_button_control->setText("COIN CONTROL");
    front_button_control->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    front_button_control->setMinimumSize(168, 49);
    front_button_control->setMaximumSize(168, 49);
    front_button_control->setStyleSheet("background-color: rgb(182, 162, 126); color: rgb(242, 242, 242);");
    front_button_control->setFont(*font_main);

    connect(front_button_control, SIGNAL(clicked ()), SLOT(showDialog()));

    front_top_widget->layout()->addWidget(front_top_widget_1);
    front_top_widget->layout()->addWidget(top_empty_widget);
    front_top_widget->layout()->addWidget(front_button_control);

    front_body_widget = new QWidget();
    front_body_widget->setParent(this);
    front_body_widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    front_body_widget->setStyleSheet("background-color: transparent");
    front_Widget->layout()->addWidget(front_body_widget);


    body_widget = new QWidget();
    body_widget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    body_widget->setMinimumSize(761, 232);
    body_widget->setMaximumSize(761, 232);
    body_widget->setStyleSheet("background-color: transparent");
    front_body_widget->setLayout(new QHBoxLayout());
    QGridLayout *body_grid_layout = new QGridLayout();
    body_widget->setLayout(body_grid_layout);
//    body_widget->setLayout(new QGridLayout());
    body_grid_layout->setMargin(0);
    body_grid_layout->setContentsMargins(0,0,0,0);
    body_grid_layout->setHorizontalSpacing(17);
    body_grid_layout->setVerticalSpacing(19);
    body_grid_layout->setColumnMinimumWidth(0, 130);
    body_grid_layout->setColumnMinimumWidth(1, 467);
    body_grid_layout->setColumnMinimumWidth(2, 130);
    body_grid_layout->setRowMinimumHeight(0, 49);
    body_grid_layout->setRowMinimumHeight(1, 49);
    body_grid_layout->setRowMinimumHeight(2, 49);
    body_grid_layout->setRowMinimumHeight(3, 28);

    body_pay_widget = new QWidget();
    body_pay_widget->setStyleSheet("background-color: #29333d");
    body_pay_widget->setLayout(new QHBoxLayout());
    body_pay_widget->layout()->setMargin(0);
    body_pay_widget->layout()->setSpacing(0);
    body_pay_widget->layout()->setContentsMargins(0,0,0,0);

    edit_pay = new QLineEdit();
    edit_pay->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    edit_pay->setMinimumHeight(49);
    edit_pay->setMaximumHeight(49);
    edit_pay->setStyleSheet("background-color: transparent; color: #ffdfb5; border-color: rgb(transparent); qproperty-frame: false");
    edit_pay->setFont(*font_main);
    edit_pay->setFrame(false);

    front_button_search = new QPushButton();
    front_button_search->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    front_button_search->setMinimumSize(49, 49);
    front_button_search->setMaximumSize(49, 49);
    front_button_search->setStyleSheet("background-color: transparent");
    front_button_search->setFlat(true);
    front_button_search->setIcon(QPixmap(":/icons/res/icons/search.png"));
    front_button_search->setIconSize(QSize(24,24));
    body_pay_widget->layout()->addWidget(edit_pay);
    body_pay_widget->layout()->addWidget(front_button_search);



    body_label_widget = new QWidget();
    body_label_widget->setStyleSheet("background-color: #29333d");
    body_label_widget->setLayout(new QHBoxLayout());
    body_label_widget->layout()->setMargin(0);
    body_label_widget->layout()->setSpacing(0);
    body_label_widget->layout()->setContentsMargins(0,0,0,0);

    edit_label = new QLineEdit();
    edit_label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    edit_label->setMinimumHeight(49);
    edit_label->setMaximumHeight(49);
    edit_label->setStyleSheet("background-color: transparent; color: #ffdfb5");
    edit_label->setFrame(false);
    edit_label->setFont(*font_main);
    body_label_widget->layout()->addWidget(edit_label);


    body_amount_widget = new QWidget();
    body_amount_widget->setStyleSheet("background-color: #29333d");
    body_amount_widget->setLayout(new QHBoxLayout());
    body_amount_widget->layout()->setMargin(0);
    body_amount_widget->layout()->setSpacing(0);
    body_amount_widget->layout()->setContentsMargins(0,0,0,0);

    edit_amount = new QLineEdit();
    edit_amount->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    edit_amount->setMinimumHeight(49);
    edit_amount->setMaximumHeight(49);
    edit_amount->setStyleSheet("background-color: transparent; color: #ffdfb5");
    edit_amount->setFrame(false);
    edit_amount->setFont(*font_main);
    body_amount_widget->layout()->addWidget(edit_amount);



    body_toggle_widget = new QWidget();
    body_toggle_widget->setStyleSheet("background-color: transparent");

    body_grid_layout->addWidget(body_pay_widget, 0, 1);
    body_grid_layout->addWidget(body_label_widget, 1, 1);
    body_grid_layout->addWidget(body_amount_widget, 2, 1);
    body_grid_layout->addWidget(body_toggle_widget, 3, 1);

    front_label_balance = new QLabel();
    front_label_balance->setStyleSheet("background-color: transparent; color: white; margin-top: 17px");
    front_label_balance->setText("Pay To:");
    front_label_balance->setAlignment(Qt::AlignRight);
    front_label_balance->setFont(*font_main);
    body_grid_layout->addWidget(front_label_balance, 0, 0);

    front_label_label = new QLabel();
    front_label_label->setStyleSheet("background-color: transparent; color: white; margin-top: 17px");
    front_label_label->setText("LABEL:");
    front_label_label->setAlignment(Qt::AlignRight);
    front_label_label->setFont(*font_main);
    body_grid_layout->addWidget(front_label_label, 1, 0);

    front_label_amount = new QLabel();
    front_label_amount->setStyleSheet("background-color: transparent; color: white; margin-top: 17px");
    front_label_amount->setText("AMOUNT:");
    front_label_amount->setAlignment(Qt::AlignRight);
    front_label_amount->setFont(*font_main);
    body_grid_layout->addWidget(front_label_amount, 2, 0);

    front_button_send = new QPushButton();
    front_button_send->setText("SEND");
    front_button_send->setStyleSheet("background-color: rgb(182, 162, 126); color: rgb(242, 242, 242);");
    front_button_send->setFont(*font_main);
    front_button_send->setMinimumHeight(49);
    front_button_send->setMaximumHeight(49);
    body_grid_layout->addWidget(front_button_send, 2, 2);

    body_toggle_widget->setLayout(new QHBoxLayout());
    body_toggle_widget->layout()->setMargin(0);
    body_toggle_widget->layout()->setSpacing(11);
    body_toggle_widget->layout()->setContentsMargins(0,0,0,0);

    front_button_advanced = new QPushButton();
    front_button_advanced->setText("ADVANCE");
    front_button_advanced->setFont(*font_small);
    front_button_advanced->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    front_button_advanced->setMinimumSize(82, 15);
    front_button_advanced->setMaximumSize(82, 15);
    front_button_advanced->setStyleSheet("background-color: transparent; color: white");

    connect(front_button_advanced, SIGNAL(clicked()), SLOT(showAdvanced()));

    QWidget *emptyWidget1 = new QWidget();
    emptyWidget1->setStyleSheet("background-color: transparent");
    emptyWidget1->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    front_label_swiftx = new QLabel();
    front_label_swiftx->setStyleSheet("background-color: transparent; color: white");
    front_label_swiftx->setText("SWIFTX");
    front_label_swiftx->setFont(*font_small);

    front_toggle = new ToggleSwitch();
    front_toggle->setParent(this);
    front_toggle->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    front_toggle->setMinimumSize(68, 28);
    front_toggle->setMaximumSize(68, 28);

    body_toggle_widget->layout()->addWidget(front_button_advanced);
    body_toggle_widget->layout()->addWidget(emptyWidget1);
    body_toggle_widget->layout()->addWidget(front_label_swiftx);
    body_toggle_widget->layout()->addWidget(front_toggle);

    front_body_widget->layout()->addWidget(body_widget);

}

void SendCoinsDialog::setPayTo(QString address)
{
    edit_pay->setText(address);
}

void SendCoinsDialog::setAdvancePage()
{
//    advance_Widget = new QWidget();
//    advance_Widget->setParent(this);
//    advance_Widget->setGeometry(0, 158, this->width(), this->height()-158);
//    advance_Widget->setStyleSheet("background-color: red");
//    advance_Widget->show();
//    advance_Widget->hide();

//    advance_Widget->setLayout(new QVBoxLayout());
//    advance_Widget->layout()->setMargin(0);
//    advance_Widget->layout()->setSpacing(0);
//    advance_Widget->layout()->setContentsMargins(52, 0, 52, 0);
//    advance_Widget->raise();

//    advance_body_widget = new QWidget();
//    advance_body_widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
//    advance_body_widget->setMinimumHeight(447);
//    advance_body_widget->setMaximumHeight(447);
//    advance_body_widget->setStyleSheet("background-color: blue");
//    advance_Widget->layout()->addWidget(advance_body_widget);


    advance_Widget = new QWidget();
    advance_Widget->setParent(this);
    advance_Widget->setGeometry(0, (this->height()-530)/2 , this->width(), 372);
    verticalLayout_advance = new QVBoxLayout(advance_Widget);
    verticalLayout_advance->setSpacing(44);
    verticalLayout_advance->setObjectName(QStringLiteral("verticalLayout"));
    verticalLayout_advance->setContentsMargins(52, 32, 52, 17);
    label_transaction_fee_advance = new QLabel(advance_Widget);
    label_transaction_fee_advance->setObjectName(QStringLiteral("label_transaction_fee_advance"));
    QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Fixed);

    label_transaction_fee_advance->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    label_transaction_fee_advance->setStyleSheet("background-color: transparent; color: white");
    label_transaction_fee_advance->setFont(*font_main);

    verticalLayout_advance->addWidget(label_transaction_fee_advance);

    gridLayout_advance = new QGridLayout();
    gridLayout_advance->setObjectName(QStringLiteral("gridLayout_advance"));
    gridLayout_advance->setVerticalSpacing(16);
    gridLayout_advance->setContentsMargins(45, -1, 45, -1);
    radio_custom_advance = new QRadioButton(advance_Widget);
    radio_custom_advance->setStyleSheet("background-color: transparent; color: white");
    radio_custom_advance->setFont(*font_main);
    radio_custom_advance->setObjectName(QStringLiteral("radio_custom_advance"));

    gridLayout_advance->addWidget(radio_custom_advance, 0, 1, 1, 1);

    horizontalLayout_2_advance = new QHBoxLayout();
    horizontalLayout_2_advance->setObjectName(QStringLiteral("horizontalLayout_2"));
    radio_recommened_advance = new QRadioButton(advance_Widget);
    radio_recommened_advance->setStyleSheet("background-color: transparent; color: white");
    radio_recommened_advance->setFont(*font_main);
    radio_recommened_advance->setObjectName(QStringLiteral("radio_recommened_advance"));

    horizontalLayout_2_advance->addWidget(radio_recommened_advance);

    label_recommened_advance = new QLabel(advance_Widget);
    label_recommened_advance->setStyleSheet("background-color: transparent; color: #b6a27e");
    label_recommened_advance->setFont(*font_main);
    label_recommened_advance->setObjectName(QStringLiteral("label_recommened_advance"));

    horizontalLayout_2_advance->addWidget(label_recommened_advance);

    horizontalSpacer_advance = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    horizontalLayout_2_advance->addItem(horizontalSpacer_advance);


    gridLayout_advance->addLayout(horizontalLayout_2_advance, 0, 0, 1, 1);

    verticalLayout_2_advance = new QVBoxLayout();
    verticalLayout_2_advance->setObjectName(QStringLiteral("verticalLayout_2_advance"));
    horizontalLayout_3_advance = new QHBoxLayout();
    horizontalLayout_3_advance->setObjectName(QStringLiteral("horizontalLayout_3"));
    label_6_advance = new QLabel(advance_Widget);
    label_6_advance->setObjectName(QStringLiteral("label_6_advance"));
    label_6_advance->setStyleSheet("background-color: transparent; color: #959595");
    label_6_advance->setFont(*font_small);
    label_6_advance->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    label_6_advance->setMinimumSize(QSize(0, 18));
    label_6_advance->setMaximumSize(QSize(16777215, 18));

    horizontalLayout_3_advance->addWidget(label_6_advance);

    label_5_advance = new QLabel(advance_Widget);
    label_5_advance->setObjectName(QStringLiteral("label_5_advance"));
    label_5_advance->setStyleSheet("background-color: transparent; color: #959595");
    label_5_advance->setFont(*font_small);
    label_5_advance->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    label_5_advance->setMinimumSize(QSize(0, 18));
    label_5_advance->setMaximumSize(QSize(16777215, 18));
    label_5_advance->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

    horizontalLayout_3_advance->addWidget(label_5_advance);


    verticalLayout_2_advance->addLayout(horizontalLayout_3_advance);

    speed_slider_advance = new QSlider(advance_Widget);
    speed_slider_advance->setObjectName(QStringLiteral("speed_slider_advance"));
    speed_slider_advance->setOrientation(Qt::Horizontal);
    speed_slider_advance->setStyleSheet("QSlider::groove:horizontal {border: 1px solid #bbb;background: white;height: 10px;border-radius: 4px;}QSlider::sub-page:horizontal {background: qlineargradient(x1: 0, y1: 0,    x2: 0, y2: 1,    stop: 0 #66e, stop: 1 #bbf);background: qlineargradient(x1: 0, y1: 0.2, x2: 1, y2: 1,    stop: 0 #bbf, stop: 1 #55f);border: 1px solid #777;height: 10px;border-radius: 4px;}QSlider::add-page:horizontal {background: #fff;border: 1px solid #777;height: 10px;border-radius: 4px;}QSlider::handle:horizontal {background: qlineargradient(x1:0, y1:0, x2:1, y2:1,    stop:0 #eee, stop:1 #ccc);border: 1px solid #777;width: 13px;margin-top: -2px;margin-bottom: -2px;border-radius: 4px;}QSlider::handle:horizontal:hover {background: qlineargradient(x1:0, y1:0, x2:1, y2:1,    stop:0 #fff, stop:1 #ddd);border: 1px solid #444;border-radius: 4px;}QSlider::sub-page:horizontal:disabled {background: #bbb;border-color: #999;}QSlider::add-page:horizontal:disabled {background: #eee;border-color: #999;}QSlider::handle:horizontal:disabled {background: #eee;border: 1px solid #aaa;border-radius: 4px;}");

    verticalLayout_2_advance->addWidget(speed_slider_advance);

    gridLayout_advance->addLayout(verticalLayout_2_advance, 2, 0, 1, 1);

    check_minimum_fee_advance = new QCheckBox(advance_Widget);
    check_minimum_fee_advance->setObjectName(QStringLiteral("check_minimum_fee_advance"));
    check_minimum_fee_advance->setStyleSheet("background-color: transparent; color: white");
    check_minimum_fee_advance->setFont(*font_main);

    gridLayout_advance->addWidget(check_minimum_fee_advance, 3, 1, 1, 1);

    label_2_advance = new QLabel(advance_Widget);
    label_2_advance->setObjectName(QStringLiteral("label_2_advance"));

    gridLayout_advance->addWidget(label_2_advance, 3, 0, 1, 1);

    horizontalLayout_5_advance = new QHBoxLayout();
    horizontalLayout_5_advance->setObjectName(QStringLiteral("horizontalLayout_5_advance"));
    lineEdit_advance = new QLineEdit(advance_Widget);
    lineEdit_advance->setObjectName(QStringLiteral("lineEdit_advance"));
    lineEdit_advance->setMinimumSize(QSize(0, 49));
    lineEdit_advance->setMaximumSize(QSize(16777215, 49));
    lineEdit_advance->setStyleSheet("background-color: #29333d; color: #ffdfb5");
    lineEdit_advance->setFrame(false);
    lineEdit_advance->setFont(*font_main);


    horizontalLayout_5_advance->addWidget(lineEdit_advance);

    label_8_advance = new QLabel(advance_Widget);
    label_8_advance->setObjectName(QStringLiteral("label_8_advance"));
    label_8_advance->setStyleSheet("background-color: transparent; color: #ffdfb5");
    label_8_advance->setFont(*font_main);

    horizontalLayout_5_advance->addWidget(label_8_advance);


    gridLayout_advance->addLayout(horizontalLayout_5_advance, 2, 1, 1, 1);

    label_3_advance = new QLabel(advance_Widget);
    label_3_advance->setObjectName(QStringLiteral("label_3_advance"));
    sizePolicy1.setHeightForWidth(label_3_advance->sizePolicy().hasHeightForWidth());
    label_3_advance->setSizePolicy(sizePolicy1);
    label_3_advance->setMinimumSize(QSize(0, 18));
    label_3_advance->setMaximumSize(QSize(16777215, 18));
    label_3_advance->setStyleSheet("background-color: transparent; color: white");
    label_3_advance->setFont(*font_main);


    gridLayout_advance->addWidget(label_3_advance, 1, 0, 1, 1);

    horizontalLayout_4_advance = new QHBoxLayout();
    horizontalLayout_4_advance->setObjectName(QStringLiteral("horizontalLayout_4_advance"));
    label_9_advance = new QLabel(advance_Widget);
    label_9_advance->setObjectName(QStringLiteral("label_9_advance"));
    sizePolicy1.setHeightForWidth(label_9_advance->sizePolicy().hasHeightForWidth());
    label_9_advance->setSizePolicy(sizePolicy1);
    label_9_advance->setMinimumSize(QSize(0, 18));
    label_9_advance->setMaximumSize(QSize(16777215, 18));
    label_9_advance->setStyleSheet("background-color: transparent; color: white");
    label_9_advance->setFont(*font_main);

    horizontalLayout_4_advance->addWidget(label_9_advance);

    widget_toggle_advance = new QWidget(advance_Widget);
    widget_toggle_advance->setObjectName(QStringLiteral("widget_toggle_advance"));
    widget_toggle_advance->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    widget_toggle_advance->setMinimumSize(QSize(68, 28));
    widget_toggle_advance->setMaximumSize(QSize(68, 28));
    widget_toggle_advance->setLayout(new QVBoxLayout());
    widget_toggle_advance->layout()->setMargin(0);
    widget_toggle_advance->layout()->setContentsMargins(0,0,0,0);
    widget_toggle_advance->layout()->setSpacing(0);
    switching_advance = new ToggleSwitch();
    switching_advance->setGeometry(0,0,68,28);
    widget_toggle_advance->layout()->addWidget(switching_advance);

    horizontalLayout_4_advance->addWidget(widget_toggle_advance);

    label_7_advance = new QLabel(advance_Widget);
    label_7_advance->setObjectName(QStringLiteral("label_7_advance"));
    label_7_advance->setStyleSheet("background-color: transparent; color: white");
    label_7_advance->setFont(*font_main);

    label_2_advance->setStyleSheet("background-color: transparent; color: #959595");
    label_2_advance->setFont(*font_small);

    horizontalLayout_4_advance->addWidget(label_7_advance);


    gridLayout_advance->addLayout(horizontalLayout_4_advance, 1, 1, 1, 1);


    verticalLayout_advance->addLayout(gridLayout_advance);

    button_ok_advance = new QPushButton(advance_Widget);
    button_ok_advance->setObjectName(QStringLiteral("button_ok_advance"));
    QSizePolicy sizePolicy2(QSizePolicy::Fixed, QSizePolicy::Fixed);
    sizePolicy2.setHorizontalStretch(0);
    sizePolicy2.setVerticalStretch(0);
    sizePolicy2.setHeightForWidth(button_ok_advance->sizePolicy().hasHeightForWidth());
    button_ok_advance->setSizePolicy(sizePolicy2);
    button_ok_advance->setMinimumSize(QSize(150, 49));
    button_ok_advance->setMaximumSize(QSize(150, 49));
    button_ok_advance->setStyleSheet("background-color: #b6a27e; color: #f2f2f2");
    button_ok_advance->setFont(*font_main);
    verticalLayout_advance->addWidget(button_ok_advance);

    label_transaction_fee_advance->setText(QApplication::translate("advance_Widget", "TRANSADTION FEE", Q_NULLPTR));
    radio_custom_advance->setText(QApplication::translate("advance_Widget", "CUSTOM", Q_NULLPTR));
    radio_recommened_advance->setText(QApplication::translate("advance_Widget", "RECOMMENDED", Q_NULLPTR));
    label_recommened_advance->setText(QApplication::translate("advance_Widget", "0.00113841 DOGEC/KB", Q_NULLPTR));
    label_6_advance->setText(QApplication::translate("advance_Widget", "NORMAL", Q_NULLPTR));
    label_5_advance->setText(QApplication::translate("advance_Widget", "FAST", Q_NULLPTR));
    check_minimum_fee_advance->setText(QApplication::translate("advance_Widget", "PAY ONLY THE MINIMUM FEE (0.0001 DOGEC)", Q_NULLPTR));
    label_2_advance->setText(QApplication::translate("advance_Widget", "Estimated to begin confirmation within 1 block", Q_NULLPTR));
    label_8_advance->setText(QApplication::translate("advance_Widget", "DOGEC", Q_NULLPTR));
    label_3_advance->setText(QApplication::translate("advance_Widget", "CONFIRMATION TIME", Q_NULLPTR));
    label_9_advance->setText(QApplication::translate("advance_Widget", "PER KILOBYTE", Q_NULLPTR));
    label_7_advance->setText(QApplication::translate("advance_Widget", "TOTAL AT LEAST", Q_NULLPTR));
    button_ok_advance->setText(QApplication::translate("advance_Widget", "OK", Q_NULLPTR));

    advance_Widget->hide();

}

void SendCoinsDialog::loadDialog()
{
    ControlDialog = new QWidget(this);
    ControlDialog->setGeometry(0, 158, this->width(), this->height() - 158);
    ControlDialog->setStyleSheet(QLatin1String("QCheckBox::indicator:checked  {image: url(:/icon/check.png);}  QCheckBox::indicator:unchecked  { image: url(:/icon/uncheck.png);  }\n"
    "QCheckBox::indicator{width: 13px;height:13px;}"));
    verticalLayout_dialog = new QVBoxLayout(ControlDialog);
    verticalLayout_dialog->setSpacing(0);
    verticalLayout_dialog->setObjectName(QStringLiteral("verticalLayout_dialog"));
    verticalLayout_dialog->setContentsMargins(0, 40, 0, 0);
    horizontalLayout_dialog_3 = new QHBoxLayout();
    horizontalLayout_dialog_3->setSpacing(0);
    horizontalLayout_dialog_3->setObjectName(QStringLiteral("horizontalLayout_dialog_3"));
    gridLayout_dialog = new QGridLayout();
    gridLayout_dialog->setObjectName(QStringLiteral("gridLayout_dialog"));
    gridLayout_dialog->setHorizontalSpacing(65);
    gridLayout_dialog->setVerticalSpacing(14);
    horizontalLayout_dialog_6 = new QHBoxLayout();
    horizontalLayout_dialog_6->setSpacing(16);
    horizontalLayout_dialog_6->setObjectName(QStringLiteral("horizontalLayout_dialog_6"));
    horizontalLayout_dialog_6->setContentsMargins(54, -1, -1, -1);
    label_quantity_dialog = new QLabel(ControlDialog);
    label_quantity_dialog->setObjectName(QStringLiteral("label_quantity_dialog"));
    QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(label_quantity_dialog->sizePolicy().hasHeightForWidth());
    label_quantity_dialog->setSizePolicy(sizePolicy);
    label_quantity_dialog->setMinimumSize(QSize(86, 18));
    label_quantity_dialog->setMaximumSize(QSize(16777215, 18));
    label_quantity_dialog->setStyleSheet(QLatin1String("background-color: transparent;\n"
"color: #ffffff"));

    horizontalLayout_dialog_6->addWidget(label_quantity_dialog);

    label_quantity_value_dialog = new QLabel(ControlDialog);
    label_quantity_value_dialog->setObjectName(QStringLiteral("label_quantity_value_dialog"));
    QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Fixed);
    sizePolicy1.setHorizontalStretch(0);
    sizePolicy1.setVerticalStretch(0);
    sizePolicy1.setHeightForWidth(label_quantity_value_dialog->sizePolicy().hasHeightForWidth());
    label_quantity_value_dialog->setSizePolicy(sizePolicy1);
    label_quantity_value_dialog->setMinimumSize(QSize(0, 18));
    label_quantity_value_dialog->setMaximumSize(QSize(16777215, 18));
    label_quantity_value_dialog->setStyleSheet(QLatin1String("background-color: transparent;\n"
"color: #ffdfb5;"));

    horizontalLayout_dialog_6->addWidget(label_quantity_value_dialog);


    gridLayout_dialog->addLayout(horizontalLayout_dialog_6, 0, 0, 1, 1);

    horizontalLayout_dialog_7 = new QHBoxLayout();
    horizontalLayout_dialog_7->setSpacing(13);
    horizontalLayout_dialog_7->setObjectName(QStringLiteral("horizontalLayout_dialog_7"));
    horizontalLayout_dialog_7->setContentsMargins(54, -1, -1, -1);
    label_bytes_dialog = new QLabel(ControlDialog);
    label_bytes_dialog->setObjectName(QStringLiteral("label_bytes_dialog"));
    sizePolicy.setHeightForWidth(label_bytes_dialog->sizePolicy().hasHeightForWidth());
    label_bytes_dialog->setSizePolicy(sizePolicy);
    label_bytes_dialog->setMinimumSize(QSize(53, 18));
    label_bytes_dialog->setMaximumSize(QSize(53, 18));
    label_bytes_dialog->setStyleSheet(QLatin1String("background-color: transparent;\n"
"color: #ffffff"));

    horizontalLayout_dialog_7->addWidget(label_bytes_dialog);

    label_bytes_value_dialog = new QLabel(ControlDialog);
    label_bytes_value_dialog->setObjectName(QStringLiteral("label_bytes_value_dialog"));
    sizePolicy1.setHeightForWidth(label_bytes_value_dialog->sizePolicy().hasHeightForWidth());
    label_bytes_value_dialog->setSizePolicy(sizePolicy1);
    label_bytes_value_dialog->setMinimumSize(QSize(0, 18));
    label_bytes_value_dialog->setMaximumSize(QSize(16777215, 18));
    QFont font;
    font.setFamily(QStringLiteral("Chivo"));
    label_bytes_value_dialog->setFont(font);
    label_bytes_value_dialog->setStyleSheet(QLatin1String("background-color: transparent;\n"
"color: #ffdfb5;"));

    horizontalLayout_dialog_7->addWidget(label_bytes_value_dialog);


    gridLayout_dialog->addLayout(horizontalLayout_dialog_7, 1, 0, 1, 1);

    horizontalLayout_dialog_8 = new QHBoxLayout();
    horizontalLayout_dialog_8->setSpacing(14);
    horizontalLayout_dialog_8->setObjectName(QStringLiteral("horizontalLayout_dialog_8"));
    label_amount_dialog = new QLabel(ControlDialog);
    label_amount_dialog->setObjectName(QStringLiteral("label_amount_dialog"));
    QSizePolicy sizePolicy2(QSizePolicy::Fixed, QSizePolicy::Fixed);
    sizePolicy2.setHorizontalStretch(0);
    sizePolicy2.setVerticalStretch(18);
    sizePolicy2.setHeightForWidth(label_amount_dialog->sizePolicy().hasHeightForWidth());
    label_amount_dialog->setSizePolicy(sizePolicy2);
    label_amount_dialog->setMinimumSize(QSize(73, 18));
    label_amount_dialog->setMaximumSize(QSize(37, 18));
    label_amount_dialog->setStyleSheet(QLatin1String("background-color: transparent;\n"
"color: #ffffff"));

    horizontalLayout_dialog_8->addWidget(label_amount_dialog);

    label_amount_value_dialog = new QLabel(ControlDialog);
    label_amount_value_dialog->setObjectName(QStringLiteral("label_amount_value_dialog"));
    sizePolicy1.setHeightForWidth(label_amount_value_dialog->sizePolicy().hasHeightForWidth());
    label_amount_value_dialog->setSizePolicy(sizePolicy1);
    label_amount_value_dialog->setMinimumSize(QSize(0, 18));
    label_amount_value_dialog->setMaximumSize(QSize(16777215, 18));
    label_amount_value_dialog->setStyleSheet(QLatin1String("background-color: transparent;\n"
"color: #ffdfb5;"));

    horizontalLayout_dialog_8->addWidget(label_amount_value_dialog);


    gridLayout_dialog->addLayout(horizontalLayout_dialog_8, 0, 1, 1, 1);

    horizontalLayout_dialog_10 = new QHBoxLayout();
    horizontalLayout_dialog_10->setSpacing(15);
    horizontalLayout_dialog_10->setObjectName(QStringLiteral("horizontalLayout_dialog_10"));
    label_priority_dialog = new QLabel(ControlDialog);
    label_priority_dialog->setObjectName(QStringLiteral("label_priority_dialog"));
    sizePolicy.setHeightForWidth(label_priority_dialog->sizePolicy().hasHeightForWidth());
    label_priority_dialog->setSizePolicy(sizePolicy);
    label_priority_dialog->setMinimumSize(QSize(83, 18));
    label_priority_dialog->setMaximumSize(QSize(83, 18));
    label_priority_dialog->setStyleSheet(QLatin1String("background-color: transparent;\n"
"color: #ffffff"));

    horizontalLayout_dialog_10->addWidget(label_priority_dialog);

    label_priority_value_dialog = new QLabel(ControlDialog);
    label_priority_value_dialog->setObjectName(QStringLiteral("label_priority_value_dialog"));
    sizePolicy1.setHeightForWidth(label_priority_value_dialog->sizePolicy().hasHeightForWidth());
    label_priority_value_dialog->setSizePolicy(sizePolicy1);
    label_priority_value_dialog->setMinimumSize(QSize(0, 18));
    label_priority_value_dialog->setMaximumSize(QSize(16777215, 18));
    label_priority_value_dialog->setStyleSheet(QLatin1String("background-color: transparent;\n"
"color: #ffdfb5;"));

    horizontalLayout_dialog_10->addWidget(label_priority_value_dialog);


    gridLayout_dialog->addLayout(horizontalLayout_dialog_10, 1, 1, 1, 1);

    horizontalLayout_dialog_11 = new QHBoxLayout();
    horizontalLayout_dialog_11->setSpacing(16);
    horizontalLayout_dialog_11->setObjectName(QStringLiteral("horizontalLayout_dialog_11"));
    label_change_dialog = new QLabel(ControlDialog);
    label_change_dialog->setObjectName(QStringLiteral("label_change_dialog"));
    sizePolicy.setHeightForWidth(label_change_dialog->sizePolicy().hasHeightForWidth());
    label_change_dialog->setSizePolicy(sizePolicy);
    label_change_dialog->setMinimumSize(QSize(69, 18));
    label_change_dialog->setMaximumSize(QSize(69, 18));
    label_change_dialog->setStyleSheet(QLatin1String("background-color: transparent;\n"
"color: #ffffff"));

    horizontalLayout_dialog_11->addWidget(label_change_dialog);

    label_change_value_dialog = new QLabel(ControlDialog);
    label_change_value_dialog->setObjectName(QStringLiteral("label_change_value_dialog"));
    QSizePolicy sizePolicy3(QSizePolicy::Preferred, QSizePolicy::Fixed);

    label_change_value_dialog->setSizePolicy(sizePolicy3);
    label_change_value_dialog->setMinimumSize(QSize(0, 18));
    label_change_value_dialog->setMaximumSize(QSize(16777215, 18));
    label_change_value_dialog->setStyleSheet(QLatin1String("background-color: transparent;\n"
"color: #ffdfb5;"));

    horizontalLayout_dialog_11->addWidget(label_change_value_dialog);


    gridLayout_dialog->addLayout(horizontalLayout_dialog_11, 0, 2, 1, 1);

    horizontalLayout_dialog_12 = new QHBoxLayout();
    horizontalLayout_dialog_12->setSpacing(16);
    horizontalLayout_dialog_12->setObjectName(QStringLiteral("horizontalLayout_dialog_12"));
    label_dust_dialog = new QLabel(ControlDialog);
    label_dust_dialog->setObjectName(QStringLiteral("label_dust_dialog"));
    sizePolicy.setHeightForWidth(label_dust_dialog->sizePolicy().hasHeightForWidth());
    label_dust_dialog->setSizePolicy(sizePolicy);
    label_dust_dialog->setMinimumSize(QSize(44, 18));
    label_dust_dialog->setMaximumSize(QSize(44, 18));
    label_dust_dialog->setStyleSheet(QLatin1String("background-color: transparent;\n"
"color: #ffffff"));

    horizontalLayout_dialog_12->addWidget(label_dust_dialog);

    label_dust_value_dialog = new QLabel(ControlDialog);
    label_dust_value_dialog->setObjectName(QStringLiteral("label_dust_value_dialog"));
    sizePolicy1.setHeightForWidth(label_dust_value_dialog->sizePolicy().hasHeightForWidth());
    label_dust_value_dialog->setSizePolicy(sizePolicy1);
    label_dust_value_dialog->setMinimumSize(QSize(0, 18));
    label_dust_value_dialog->setMaximumSize(QSize(16777215, 18));
    label_dust_value_dialog->setStyleSheet(QLatin1String("background-color: transparent;\n"
"color: #ffdfb5;"));

    horizontalLayout_dialog_12->addWidget(label_dust_value_dialog);


    gridLayout_dialog->addLayout(horizontalLayout_dialog_12, 1, 2, 1, 1);

    horizontalLayout_dialog_13 = new QHBoxLayout();
    horizontalLayout_dialog_13->setSpacing(12);
    horizontalLayout_dialog_13->setObjectName(QStringLiteral("horizontalLayout_dialog_13"));
    label_fee_dialog = new QLabel(ControlDialog);
    label_fee_dialog->setObjectName(QStringLiteral("label_fee_dialog"));
    sizePolicy.setHeightForWidth(label_fee_dialog->sizePolicy().hasHeightForWidth());
    label_fee_dialog->setSizePolicy(sizePolicy);
    label_fee_dialog->setMinimumSize(QSize(31, 18));
    label_fee_dialog->setMaximumSize(QSize(31, 18));
    label_fee_dialog->setStyleSheet(QLatin1String("background-color: transparent;\n"
"color: #ffffff"));

    horizontalLayout_dialog_13->addWidget(label_fee_dialog);

    label_fee_value_dialog = new QLabel(ControlDialog);
    label_fee_value_dialog->setObjectName(QStringLiteral("label_fee_value_dialog"));
    sizePolicy1.setHeightForWidth(label_fee_value_dialog->sizePolicy().hasHeightForWidth());
    label_fee_value_dialog->setSizePolicy(sizePolicy1);
    label_fee_value_dialog->setMinimumSize(QSize(0, 18));
    label_fee_value_dialog->setMaximumSize(QSize(16777215, 18));
    label_fee_value_dialog->setStyleSheet(QLatin1String("background-color: transparent;\n"
"color: #ffdfb5;"));

    horizontalLayout_dialog_13->addWidget(label_fee_value_dialog);


    gridLayout_dialog->addLayout(horizontalLayout_dialog_13, 0, 3, 1, 1);

    horizontalLayout_dialog_14 = new QHBoxLayout();
    horizontalLayout_dialog_14->setSpacing(16);
    horizontalLayout_dialog_14->setObjectName(QStringLiteral("horizontalLayout_dialog_14"));
    label_after_fee_dialog = new QLabel(ControlDialog);
    label_after_fee_dialog->setObjectName(QStringLiteral("label_after_fee_dialog"));
    sizePolicy.setHeightForWidth(label_after_fee_dialog->sizePolicy().hasHeightForWidth());
    label_after_fee_dialog->setSizePolicy(sizePolicy);
    label_after_fee_dialog->setMinimumSize(QSize(88, 18));
    label_after_fee_dialog->setMaximumSize(QSize(88, 18));
    label_after_fee_dialog->setStyleSheet(QLatin1String("background-color: transparent;\n"
"color: #ffffff"));

    horizontalLayout_dialog_14->addWidget(label_after_fee_dialog);

    label_after_fee_value_dialog = new QLabel(ControlDialog);
    label_after_fee_value_dialog->setObjectName(QStringLiteral("label_after_fee_value_dialog"));
    sizePolicy1.setHeightForWidth(label_after_fee_value_dialog->sizePolicy().hasHeightForWidth());
    label_after_fee_value_dialog->setSizePolicy(sizePolicy1);
    label_after_fee_value_dialog->setMinimumSize(QSize(0, 18));
    label_after_fee_value_dialog->setMaximumSize(QSize(16777215, 18));
    label_after_fee_value_dialog->setStyleSheet(QLatin1String("background-color: transparent;\n"
"color: #ffdfb5;"));

    horizontalLayout_dialog_14->addWidget(label_after_fee_value_dialog);


    gridLayout_dialog->addLayout(horizontalLayout_dialog_14, 1, 3, 1, 1);


    horizontalLayout_dialog_3->addLayout(gridLayout_dialog);

    horizontalSpacer_dialog_3 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    horizontalLayout_dialog_3->addItem(horizontalSpacer_dialog_3);


    verticalLayout_dialog->addLayout(horizontalLayout_dialog_3);

    horizontalLayout_dialog = new QHBoxLayout();
    horizontalLayout_dialog->setSpacing(0);
    horizontalLayout_dialog->setObjectName(QStringLiteral("horizontalLayout_dialog"));
    horizontalLayout_dialog->setContentsMargins(54, 30, -1, 28);
    button_select_all_dialog = new QPushButton(ControlDialog);
    button_select_all_dialog->setObjectName(QStringLiteral("button_select_all_dialog"));
    sizePolicy.setHeightForWidth(button_select_all_dialog->sizePolicy().hasHeightForWidth());
    button_select_all_dialog->setSizePolicy(sizePolicy);
    button_select_all_dialog->setMinimumSize(QSize(150, 49));
    button_select_all_dialog->setStyleSheet(QLatin1String("background-color: #b6a27e;\n"
"color: #f2f2f2;"));

    horizontalLayout_dialog->addWidget(button_select_all_dialog);

    horizontalSpacer_dialog_4 = new QSpacerItem(36, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);

    horizontalLayout_dialog->addItem(horizontalSpacer_dialog_4);

    button_toggle_lock_state_dialog = new QPushButton(ControlDialog);
    button_toggle_lock_state_dialog->setObjectName(QStringLiteral("button_toggle_lock_state_dialog"));
    sizePolicy.setHeightForWidth(button_toggle_lock_state_dialog->sizePolicy().hasHeightForWidth());
    button_toggle_lock_state_dialog->setSizePolicy(sizePolicy);
    button_toggle_lock_state_dialog->setMinimumSize(QSize(218, 49));
    button_toggle_lock_state_dialog->setMaximumSize(QSize(218, 49));
    button_toggle_lock_state_dialog->setStyleSheet(QLatin1String("background-color: #b6a27e;\n"
"color: #f2f2f2;"));

    horizontalLayout_dialog->addWidget(button_toggle_lock_state_dialog);

    horizontalSpacer_dialog_5 = new QSpacerItem(83, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);

    horizontalLayout_dialog->addItem(horizontalSpacer_dialog_5);

    label_tree_mode_dialog = new QLabel(ControlDialog);
    label_tree_mode_dialog->setObjectName(QStringLiteral("label_tree_mode_dialog"));
    sizePolicy.setHeightForWidth(label_tree_mode_dialog->sizePolicy().hasHeightForWidth());
    label_tree_mode_dialog->setSizePolicy(sizePolicy);
    label_tree_mode_dialog->setMinimumSize(QSize(97, 18));
    label_tree_mode_dialog->setMaximumSize(QSize(97, 18));
    label_tree_mode_dialog->setStyleSheet(QLatin1String("background-color: transparent;\n"
"color: #f2f2f2;"));

    horizontalLayout_dialog->addWidget(label_tree_mode_dialog);

    horizontalSpacer_dialog_6 = new QSpacerItem(14, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);

    horizontalLayout_dialog->addItem(horizontalSpacer_dialog_6);

    layout_tree_mode_toggle_dialog = new QHBoxLayout();
    layout_tree_mode_toggle_dialog->setObjectName(QStringLiteral("layout_tree_mode_toggle_dialog"));

    horizontalLayout_dialog->addLayout(layout_tree_mode_toggle_dialog);

    horizontalSpacer_dialog_7 = new QSpacerItem(14, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);

    horizontalLayout_dialog->addItem(horizontalSpacer_dialog_7);

    label_list_mode_dialog = new QLabel(ControlDialog);
    label_list_mode_dialog->setObjectName(QStringLiteral("label_list_mode_dialog"));
    sizePolicy.setHeightForWidth(label_list_mode_dialog->sizePolicy().hasHeightForWidth());
    label_list_mode_dialog->setSizePolicy(sizePolicy);
    label_list_mode_dialog->setMinimumSize(QSize(93, 18));
    label_list_mode_dialog->setMaximumSize(QSize(93, 18));
    label_list_mode_dialog->setStyleSheet(QLatin1String("background-color: transparent;\n"
"color: #ffffff;"));

    horizontalLayout_dialog->addWidget(label_list_mode_dialog);

    horizontalSpacer_dialog_8 = new QSpacerItem(14, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);

    horizontalLayout_dialog->addItem(horizontalSpacer_dialog_8);

    label_list_mode_value_dialog = new QLabel(ControlDialog);
    label_list_mode_value_dialog->setObjectName(QStringLiteral("label_list_mode_value_dialog"));
    sizePolicy3.setHeightForWidth(label_list_mode_value_dialog->sizePolicy().hasHeightForWidth());
    label_list_mode_value_dialog->setSizePolicy(sizePolicy3);
    label_list_mode_value_dialog->setMinimumSize(QSize(0, 18));
    label_list_mode_value_dialog->setStyleSheet(QLatin1String("background-color: transparent;\n"
"color: #ffdfb5;"));

    horizontalLayout_dialog->addWidget(label_list_mode_value_dialog);

    horizontalSpacer_dialog_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    horizontalLayout_dialog->addItem(horizontalSpacer_dialog_2);


    verticalLayout_dialog->addLayout(horizontalLayout_dialog);

    verticalLayout_dialog_2 = new QVBoxLayout();
    verticalLayout_dialog_2->setSpacing(0);
    verticalLayout_dialog_2->setObjectName(QStringLiteral("verticalLayout_dialog_2"));
    verticalLayout_dialog_2->setContentsMargins(-1, 0, -1, 20);
    horizontalLayout_dialog_2 = new QHBoxLayout();
    horizontalLayout_dialog_2->setSpacing(0);
    horizontalLayout_dialog_2->setObjectName(QStringLiteral("horizontalLayout_dialog_2"));
    horizontalLayout_dialog_2->setContentsMargins(54, -1, -1, -1);
    label_request_payments_history_dialog = new QLabel(ControlDialog);
    label_request_payments_history_dialog->setObjectName(QStringLiteral("label_request_payments_history_dialog"));
    sizePolicy1.setHeightForWidth(label_request_payments_history_dialog->sizePolicy().hasHeightForWidth());
    label_request_payments_history_dialog->setSizePolicy(sizePolicy1);
    label_request_payments_history_dialog->setMinimumSize(QSize(0, 18));
    label_request_payments_history_dialog->setMaximumSize(QSize(16777215, 18));
    label_request_payments_history_dialog->setStyleSheet(QLatin1String("background-color: transparent;\n"
"color: #ffffff;"));

    horizontalLayout_dialog_2->addWidget(label_request_payments_history_dialog);

    horizontalSpacer_dialog_9 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    horizontalLayout_dialog_2->addItem(horizontalSpacer_dialog_9);


    verticalLayout_dialog_2->addLayout(horizontalLayout_dialog_2);

    horizontalLayout_dialog_5 = new QHBoxLayout();
    horizontalLayout_dialog_5->setSpacing(0);
    horizontalLayout_dialog_5->setObjectName(QStringLiteral("horizontalLayout_dialog_5"));
    horizontalLayout_dialog_5->setContentsMargins(-1, 11, -1, -1);
    widget_table_dialog = new QWidget(ControlDialog);
    widget_table_dialog->setObjectName(QStringLiteral("widget_table_dialog"));
    widget_table_dialog->setEnabled(true);
    QSizePolicy sizePolicy4(QSizePolicy::Expanding, QSizePolicy::Preferred);
    sizePolicy4.setHorizontalStretch(0);
    sizePolicy4.setVerticalStretch(0);
    sizePolicy4.setHeightForWidth(widget_table_dialog->sizePolicy().hasHeightForWidth());
    widget_table_dialog->setSizePolicy(sizePolicy4);
    widget_table_dialog->setMinimumSize(QSize(1178, 50));
    widget_table_dialog->setMaximumSize(QSize(16777215, 16777215));
    widget_table_dialog->setStyleSheet(QStringLiteral("background-color: transparent;"));

    horizontalLayout_dialog_5->addWidget(widget_table_dialog);


    verticalLayout_dialog_2->addLayout(horizontalLayout_dialog_5);

    verticalSpacer_dialog = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

    verticalLayout_dialog_2->addItem(verticalSpacer_dialog);


    verticalLayout_dialog->addLayout(verticalLayout_dialog_2);

    horizontalLayout_4_dialog = new QHBoxLayout();
    horizontalLayout_4_dialog->setSpacing(0);
    horizontalLayout_4_dialog->setObjectName(QStringLiteral("horizontalLayout_4_dialog"));
    horizontalLayout_4_dialog->setContentsMargins(54, -1, -1, -1);
    button_ok_control_dialog = new QPushButton(ControlDialog);
    button_ok_control_dialog->setObjectName(QStringLiteral("button_ok_control_dialog"));
    sizePolicy.setHeightForWidth(button_ok_control_dialog->sizePolicy().hasHeightForWidth());
    button_ok_control_dialog->setSizePolicy(sizePolicy);
    button_ok_control_dialog->setMinimumSize(QSize(150, 49));
    button_ok_control_dialog->setStyleSheet(QLatin1String("background-color: #b6a27e;\n"
"color: #f2f2f2;"));

    horizontalLayout_4_dialog->addWidget(button_ok_control_dialog);

    horizontalSpacer_dialog = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    horizontalLayout_4_dialog->addItem(horizontalSpacer_dialog);


    verticalLayout_dialog->addLayout(horizontalLayout_4_dialog);

    label_quantity_dialog->setText(QApplication::translate("ControlDialog", "QUANTITY", Q_NULLPTR));
    label_quantity_value_dialog->setText(QApplication::translate("ControlDialog", "35", Q_NULLPTR));
    label_bytes_dialog->setText(QApplication::translate("ControlDialog", "BYTES", Q_NULLPTR));
    label_bytes_value_dialog->setText(QApplication::translate("ControlDialog", "~5258", Q_NULLPTR));
    label_amount_dialog->setText(QApplication::translate("ControlDialog", "AMOUNT", Q_NULLPTR));
    label_amount_value_dialog->setText(QApplication::translate("ControlDialog", "1,234.321 DOGEC", Q_NULLPTR));
    label_priority_dialog->setText(QApplication::translate("ControlDialog", "PRIORITY", Q_NULLPTR));
    label_priority_value_dialog->setText(QApplication::translate("ControlDialog", "MEDIUM", Q_NULLPTR));
    label_change_dialog->setText(QApplication::translate("ControlDialog", "CHANGE", Q_NULLPTR));
    label_change_value_dialog->setText(QApplication::translate("ControlDialog", "NO", Q_NULLPTR));
    label_dust_dialog->setText(QApplication::translate("ControlDialog", "DUST", Q_NULLPTR));
    label_dust_value_dialog->setText(QApplication::translate("ControlDialog", "NO", Q_NULLPTR));
    label_fee_dialog->setText(QApplication::translate("ControlDialog", "FEE", Q_NULLPTR));
    label_fee_value_dialog->setText(QApplication::translate("ControlDialog", "~0.100000000 DOGEC", Q_NULLPTR));
    label_after_fee_dialog->setText(QApplication::translate("ControlDialog", "AFTER FEE", Q_NULLPTR));
    label_after_fee_value_dialog->setText(QApplication::translate("ControlDialog", "~0.100000000 DOGEC", Q_NULLPTR));
    button_select_all_dialog->setText(QApplication::translate("ControlDialog", "SELECT ALL", Q_NULLPTR));
    button_toggle_lock_state_dialog->setText(QApplication::translate("ControlDialog", "TOGGLE LOCK STATE", Q_NULLPTR));
    label_tree_mode_dialog->setText(QApplication::translate("ControlDialog", "TREE MODE", Q_NULLPTR));
    label_list_mode_dialog->setText(QApplication::translate("ControlDialog", "LIST MODE", Q_NULLPTR));
    label_list_mode_value_dialog->setText(QApplication::translate("ControlDialog", "(10 LOCKED)", Q_NULLPTR));
    label_request_payments_history_dialog->setText(QApplication::translate("ControlDialog", "REQUESTED PAYMENTS HISTORY", Q_NULLPTR));
    button_ok_control_dialog->setText(QApplication::translate("ControlDialog", "OK", Q_NULLPTR));


    label_fee_dialog->setFont(*font_main);
    label_dust_dialog->setFont(*font_main);
    label_bytes_dialog->setFont(*font_main);
    label_amount_dialog->setFont(*font_main);
    label_change_dialog->setFont(*font_main);
    label_priority_dialog->setFont(*font_main);
    label_quantity_dialog->setFont(*font_main);
    label_after_fee_dialog->setFont(*font_main);
    label_fee_value_dialog->setFont(*font_main);
    label_dust_value_dialog->setFont(*font_main);
    label_bytes_value_dialog->setFont(*font_main);
    label_amount_value_dialog->setFont(*font_main);
    label_change_value_dialog->setFont(*font_main);
    label_priority_value_dialog->setFont(*font_main);
    label_quantity_value_dialog->setFont(*font_main);
    label_after_fee_value_dialog->setFont(*font_main);
    label_request_payments_history_dialog->setFont(*font_main);

    button_select_all_dialog->setFont(*font_main);
    button_toggle_lock_state_dialog->setFont(*font_main);
    label_tree_mode_dialog->setFont(*font_main);
    label_list_mode_dialog->setFont(*font_main);
    label_list_mode_value_dialog->setFont(*font_main);
    button_ok_control_dialog->setFont(*font_main);


    loadDialogTable();

    ControlDialog->hide();
}

void SendCoinsDialog::loadDialogTable()
{
    widget_table_data = new QWidget();
    widget_table_data->setStyleSheet("background-color: transparent");
    scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setWidget(widget_table_data);
    scrollArea->setStyleSheet("background-color:transparent;");
    loGrid = new QGridLayout();
    widget_table_data->setLayout(loGrid);

    widget_table_dialog->setLayout(new QVBoxLayout());
    widget_table_dialog->layout()->addWidget(scrollArea);
    widget_table_dialog->layout()->setMargin(0);
    widget_table_dialog->layout()->setSpacing(0);

    loGrid->setMargin(0);
    loGrid->setHorizontalSpacing(0);
    loGrid->setVerticalSpacing(0);


    label_space_header1 = new QLabel();
    label_space_header1->setStyleSheet("background-color: #1f272e; color: #b99d7c");
    label_space_header1->setMinimumSize(41,49);
    label_space_header1->setMaximumSize(41, 49);
    loGrid->addWidget(label_space_header1, 0, 0);
    label_space_header1->show();

    label_space_header2 = new QLabel();
    label_space_header2->setStyleSheet("background-color: #1f272e");
    label_space_header2->setMinimumSize(34,49);
    label_space_header2->setMaximumSize(34, 49);
    loGrid->addWidget(label_space_header2, 0, 1);
    label_space_header2->show();

    label_space_header3 = new QLabel();
    label_space_header3->setStyleSheet("background-color: #1f272e");
    label_space_header3->setMinimumSize(34,49);
    label_space_header3->setMaximumSize(34, 49);
    loGrid->addWidget(label_space_header3, 0, 2);

    label_amount_header = new QLabel("AMOUNT");
    label_amount_header->setStyleSheet("background-color: #1f272e; color: #b99d7c; padding-top: 17px; padding-bottom: 10px; padding-left: 11px; padding-right: 10px;");
    label_amount_header->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    label_amount_header->setMinimumHeight(49);
    label_amount_header->setMaximumHeight(49);
    label_amount_header->setFont(*font_small);
    loGrid->addWidget(label_amount_header, 0, 3);
    label_amount_header->show();

    label_label_header = new QLabel("LABEL");
    label_label_header->setStyleSheet("background-color: #1f272e; color: #b99d7c; padding-top: 17px; padding-bottom: 10px; padding-left: 11px; padding-right: 10px;");
    label_label_header->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    label_label_header->setMinimumHeight(49);
    label_label_header->setMaximumHeight(49);
    label_label_header->setFont(*font_small);
    loGrid->addWidget(label_label_header, 0, 4);
    label_label_header->show();

    label_address_header = new QLabel("ADDRESS");
    label_address_header->setStyleSheet("background-color: #1f272e; color: #b99d7c; padding-top: 17px; padding-bottom: 10px; padding-left: 11px; padding-right: 10px;");
    label_address_header->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    label_address_header->setMinimumHeight(49);
    label_address_header->setMaximumHeight(49);
    label_address_header->setFont(*font_small);
    loGrid->addWidget(label_address_header, 0, 5);
    label_address_header->show();

    label_personal_header = new QLabel("PERSONAL");
    label_personal_header->setStyleSheet("background-color: #1f272e; color: #b99d7c; padding-top: 17px; padding-bottom: 10px; padding-left: 11px; padding-right: 10px;");
    label_personal_header->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    label_personal_header->setMinimumHeight(49);
    label_personal_header->setMaximumHeight(49);
    label_personal_header->setFont(*font_small);
    loGrid->addWidget(label_personal_header, 0, 6);
    label_personal_header->show();

    label_date_header = new QLabel("DATE");
    label_date_header->setStyleSheet("background-color: #1f272e; color: #b99d7c; padding-top: 17px; padding-bottom: 10px; padding-left: 11px; padding-right: 10px;");
    label_date_header->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    label_date_header->setMinimumHeight(49);
    label_date_header->setMaximumHeight(49);
    label_date_header->setFont(*font_small);
    loGrid->addWidget(label_date_header, 0, 7);
    label_date_header->show();

    label_confirmation_header = new QLabel("CONFIRMATION");
    label_confirmation_header->setStyleSheet("background-color: #1f272e; color: #b99d7c; padding-top: 17px; padding-bottom: 10px; padding-left: 11px; padding-right: 10px;");
    label_confirmation_header->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    label_confirmation_header->setMinimumHeight(49);
    label_confirmation_header->setMaximumHeight(49);
    label_confirmation_header->setFont(*font_small);
    loGrid->addWidget(label_confirmation_header, 0,8);
    label_confirmation_header->show();


//    scrollArea->setWidget(ui->widget_table);
//    ui->widget_table->setLayout(ui->layout_table);



    pixmap_lock_control_dialog = new QPixmap(":/icon/lock_table.svg");
    pixmap_tree_control_dailog = new QPixmap(":/icon/tree_table.png");
    count = 15;
    label_locks_table = new QLabel*[count];
    label_table_amount = new QLabel*[count];
    label_table_label = new QLabel*[count];
    label_table_address = new QLabel*[count];
    label_table_personal = new QLabel*[count];
    label_table_date = new QLabel*[count];
    label_table_confirmation = new QLabel*[count];
    check_table_control_dailog = new QCheckBox*[count];
    label_table_tree = new QLabel*[count];


    for (int i = 0; i< count; i++){
        label_locks_table[i] = new QLabel();
        if (i % 2 == 0) {
            label_locks_table[i]->setStyleSheet("background-color: #2a3036; padding-left: 21px; padding-right: 10px;  padding-top: 8px;  padding-bottom: 8px;  color: #f2f2f2;");
        } else {
            label_locks_table[i]->setStyleSheet("background-color: #1f272e; padding-left: 21px; padding-right: 10px;  padding-top: 8px;  padding-bottom: 8px;  color: #f2f2f2;");
        }

        label_locks_table[i]->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        label_locks_table[i]->setMinimumSize(41, 29);
        label_locks_table[i]->setMaximumSize(41, 29);
        label_locks_table[i]->setPixmap(*pixmap_lock_control_dialog);
        label_locks_table[i]->setMargin(-2);

        label_table_tree[i] = new QLabel();
        if (i % 2 == 0) {
            label_table_tree[i]->setStyleSheet("background-color: #2a3036; padding-left: 11px; padding-right: 10px;  padding-top: 8px;  padding-bottom: 8px;  color: #f2f2f2;");
        } else {
            label_table_tree[i]->setStyleSheet("background-color: #1f272e; padding-left: 11px; padding-right: 10px;  padding-top: 8px;  padding-bottom: 8px;  color: #f2f2f2;");
        }

        label_table_tree[i]->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        label_table_tree[i]->setMinimumSize(34, 29);
        label_table_tree[i]->setMaximumSize(34, 29);
        label_table_tree[i]->setPixmap(*pixmap_tree_control_dailog);
        label_table_tree[i]->setMargin(-2);


        check_table_control_dailog[i] = new QCheckBox();
        if (i % 2 == 0) {
            check_table_control_dailog[i]->setStyleSheet("background-color: #2a3036; padding-left: 11px; padding-right: 10px;  padding-top: 8px;  padding-bottom: 8px;  color: #f2f2f2;");
        } else {
            check_table_control_dailog[i]->setStyleSheet("background-color: #1f272e; padding-left: 11px; padding-right: 10px;  padding-top: 8px;  padding-bottom: 8px;  color: #f2f2f2;");
        }
        check_table_control_dailog[i]->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        check_table_control_dailog[i]->setMinimumSize(34, 29);
        check_table_control_dailog[i]->setMaximumSize(34, 29);

        label_table_amount[i] = new QLabel("1,324.55231");
        if (i % 2 == 0) {
            label_table_amount[i]->setStyleSheet("background-color: #2a3036; padding-left: 11px; padding-right: 10px;  padding-top: 8px;  padding-bottom: 8px;  color: #f2f2f2;");
        } else {
            label_table_amount[i]->setStyleSheet("background-color: #1f272e; padding-left: 11px; padding-right: 10px;  padding-top: 8px;  padding-bottom: 8px;  color: #f2f2f2;");
        }
        label_table_amount[i]->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        label_table_amount[i]->setMinimumHeight(29);
        label_table_amount[i]->setMaximumHeight(29);
        label_table_amount[i]->setFont(*font_small);

        label_table_label[i] = new QLabel("(No label)");
        if (i % 2 == 0) {
            label_table_label[i]->setStyleSheet("background-color: #2a3036; padding-left: 11px; padding-right: 10px;  padding-top: 8px;  padding-bottom: 8px;  color: #f2f2f2;");
        } else {
            label_table_label[i]->setStyleSheet("background-color: #1f272e; padding-left: 11px; padding-right: 10px;  padding-top: 8px;  padding-bottom: 8px;  color: #f2f2f2;");
        }
        label_table_label[i]->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        label_table_label[i]->setMinimumHeight(29);
        label_table_label[i]->setMaximumHeight(29);
        label_table_label[i]->setFont(*font_small);
//        label_table_label[i]->text().toUpper();

        label_table_address[i] = new QLabel("DQ68xWwN8mkU8cJDLvP6HK434qUptCH68v");
        if (i % 2 == 0) {
            label_table_address[i]->setStyleSheet("background-color: #2a3036; padding-left: 11px; padding-right: 10px;  padding-top: 8px;  padding-bottom: 8px;  color: #f2f2f2;");
        } else {
            label_table_address[i]->setStyleSheet("background-color: #1f272e; padding-left: 11px; padding-right: 10px;  padding-top: 8px;  padding-bottom: 8px;  color: #f2f2f2;");
        }
        label_table_address[i]->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        label_table_address[i]->setMinimumHeight(29);
        label_table_address[i]->setMaximumHeight(29);
        label_table_address[i]->setFont(*font_small);
//        label_table_address[i]->text().toUpper();

        label_table_personal[i] = new QLabel("Personal");
        if (i % 2 == 0) {
            label_table_personal[i]->setStyleSheet("background-color: #2a3036; padding-left: 11px; padding-right: 10px;  padding-top: 8px;  padding-bottom: 8px;  color: #f2f2f2;");
        } else {
            label_table_personal[i]->setStyleSheet("background-color: #1f272e; padding-left: 11px; padding-right: 10px;  padding-top: 8px;  padding-bottom: 8px;  color: #f2f2f2;");
        }
        label_table_personal[i]->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        label_table_personal[i]->setMinimumHeight(29);
        label_table_personal[i]->setMaximumHeight(29);
        label_table_personal[i]->setFont(*font_small);
//        label_table_personal[i]->text().toUpper();

        label_table_date[i] = new QLabel("02/29/2018");
        if (i % 2 == 0) {
            label_table_date[i]->setStyleSheet("background-color: #2a3036; padding-left: 11px; padding-right: 10px;  padding-top: 8px;  padding-bottom: 8px;  color: #f2f2f2;");
        } else {
            label_table_date[i]->setStyleSheet("background-color: #1f272e; padding-left: 11px; padding-right: 10px;  padding-top: 8px;  padding-bottom: 8px;  color: #f2f2f2;");
        }
        label_table_date[i]->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        label_table_date[i]->setMinimumHeight(29);
        label_table_date[i]->setMaximumHeight(29);
        label_table_date[i]->setFont(*font_small);

        label_table_confirmation[i] = new QLabel("77,423");
        if (i % 2 == 0) {
            label_table_confirmation[i]->setStyleSheet("background-color: #2a3036; padding-left: 11px; padding-right: 10px;  padding-top: 8px;  padding-bottom: 8px;  color: #f2f2f2;");
        } else {
            label_table_confirmation[i]->setStyleSheet("background-color: #1f272e; padding-left: 11px; padding-right: 10px;  padding-top: 8px;  padding-bottom: 8px;  color: #f2f2f2;");
        }
        label_table_confirmation[i]->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        label_table_confirmation[i]->setMinimumHeight(29);
        label_table_confirmation[i]->setMaximumHeight(29);
        label_table_confirmation[i]->setFont(*font_small);


        loGrid->addWidget(label_locks_table[i], i+1,0);
        loGrid->addWidget(label_table_tree[i], i+1, 1);
        loGrid->addWidget(check_table_control_dailog[i], i+1, 2);
        loGrid->addWidget(label_table_amount[i], i+1, 3);
        loGrid->addWidget(label_table_label[i], i+1, 4);
        loGrid->addWidget(label_table_address[i], i+1, 5);
        loGrid->addWidget(label_table_personal[i], i+1, 6);
        loGrid->addWidget(label_table_date[i], i+1, 7);
        loGrid->addWidget(label_table_confirmation[i], i+1, 8);
    }
}
