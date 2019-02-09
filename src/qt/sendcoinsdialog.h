// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_SENDCOINSDIALOG_H
#define BITCOIN_QT_SENDCOINSDIALOG_H

#include "walletmodel.h"

#include <QDialog>
#include <QString>
#include <QWidget>
#include <QLabel>
#include <QSize>
#include <QBrush>
#include <QPushButton>
#include <QFont>
#include <QLineEdit>
#include <QRadioButton>
#include <QCheckBox>
#include <QSlider>

static const int MAX_SEND_POPUP_ENTRIES = 10;

class ClientModel;
class OptionsModel;
class SendCoinsEntry;
class SendCoinsRecipient;



#include <QObject>
#include <QtWidgets>


class ToggleSwitch : public QAbstractButton
{
    Q_OBJECT

    Q_PROPERTY(QBrush brush READ brush WRITE setBrush)
    Q_PROPERTY(bool status READ status WRITE setStatus)

    public:

        ToggleSwitch(QWidget * = Q_NULLPTR);
        ToggleSwitch(const QBrush &, QWidget * = Q_NULLPTR);

        QSize sizeHint(void) const override;

        virtual QBrush brush(void) const;
        virtual void setBrush(const QBrush &);

        virtual bool status(void) const;
        virtual void setStatus(bool);

    signals:

        void statusChanged(bool);

    protected:

        void paintEvent(QPaintEvent*) override;
        void mouseReleaseEvent(QMouseEvent *) override;

        bool m_status;
        int m_margin;

        QBrush m_bodyBrush;
        QBrush m_headBrush;
};

inline QBrush ToggleSwitch::brush(void) const
{
    return this->m_headBrush;
}

inline void ToggleSwitch::setBrush(const QBrush &brush)
{
    this->m_headBrush = brush;
}

inline bool ToggleSwitch::status(void) const
{
    return this->m_status;
}

inline void ToggleSwitch::setStatus(bool status)
{
    this->m_status = status;
}












namespace Ui
{
class SendCoinsDialog;
}

QT_BEGIN_NAMESPACE
class QUrl;
QT_END_NAMESPACE

/** Dialog for sending bitcoins */
class SendCoinsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SendCoinsDialog(QWidget* parent = 0);
    ~SendCoinsDialog();

    void setClientModel(ClientModel* clientModel);
    void setModel(WalletModel* model);
    void resizeEvent(QResizeEvent *event);

    void setFont();
    void setBackPage();
    void setFrontPage();
    void setAdvancePage();
    void loadDialog();
    void loadDialogTable();

    /** Set up the tab chain manually, as Qt messes up the tab chain by default in some cases (issue https://bugreports.qt-project.org/browse/QTBUG-10907).
     */
    QWidget* setupTabChain(QWidget* prev);

    void setAddress(const QString& address);
    void pasteEntry(const SendCoinsRecipient& rv);
    bool handlePaymentRequest(const SendCoinsRecipient& recipient);
    bool fSplitBlock;

public slots:
    void clear();
    void reject();
    void accept();
    SendCoinsEntry* addEntry();
    void updateTabsAndLabels();
    void setPayTo(QString address);
    void setBalance(const CAmount& balance, const CAmount& unconfirmedBalance, const CAmount& immatureBalance, 
                    const CAmount& zerocoinBalance, const CAmount& unconfirmedZerocoinBalance, const CAmount& immatureZerocoinBalance,
                    const CAmount& watchOnlyBalance, const CAmount& watchUnconfBalance, const CAmount& watchImmatureBalance);

    void showDialog();
    void showAdvanced();


private:
    Ui::SendCoinsDialog* ui;
    ClientModel* clientModel;
    WalletModel* model;

//    QWidget *main_Widget, *widget_background, *front_Widget, *front_top_widget, *front_body_widget;
//    QLabel *label_background;

//    QLabel *front_label_balanece, *front_value_balance, *front_label_payTo, *front_label_label, *front_label_amount;
    QWidget *front_Widget, *front_top_widget, *front_body_widget, *front_top_widget_1 ;
    QWidget *line_widget, *body_widget, *body_pay_widget, *body_label_widget, *body_amount_widget, *body_toggle_widget;
    QLabel *line_label;
    QLineEdit *edit_pay, *edit_label, *edit_amount;
    QLabel *front_label_balance, *front_value_balance, *front_label_pay, *front_label_label, *front_label_amount;
    QFont *font_main, *font_value, *font_small;

    QLabel *front_label_swiftx;
//    QPushButton *front_button_control, *front_button_advanced,*front_button_search,  *front_button_send, *front_toggle_buttion;
//    QPushButton *front_button_yes;
    QPushButton *front_button_control, *front_button_send, *front_button_advanced, *front_button_search;

    ToggleSwitch *front_toggle;

    QWidget *advance_Widget, *advance_body_widget;

    QWidget *ControlDialog;
    QVBoxLayout *verticalLayout_dialog;
    QHBoxLayout *horizontalLayout_dialog_3;
    QGridLayout *gridLayout_dialog;
    QHBoxLayout *horizontalLayout_dialog_6;
    QLabel *label_quantity_dialog;
    QLabel *label_quantity_value_dialog;
    QHBoxLayout *horizontalLayout_dialog_7;
    QLabel *label_bytes_dialog;
    QLabel *label_bytes_value_dialog;
    QHBoxLayout *horizontalLayout_dialog_8;
    QLabel *label_amount_dialog;
    QLabel *label_amount_value_dialog;
    QHBoxLayout *horizontalLayout_dialog_10;
    QLabel *label_priority_dialog;
    QLabel *label_priority_value_dialog;
    QHBoxLayout *horizontalLayout_dialog_11;
    QLabel *label_change_dialog;
    QLabel *label_change_value_dialog;
    QHBoxLayout *horizontalLayout_dialog_12;
    QLabel *label_dust_dialog;
    QLabel *label_dust_value_dialog;
    QHBoxLayout *horizontalLayout_dialog_13;
    QLabel *label_fee_dialog;
    QLabel *label_fee_value_dialog;
    QHBoxLayout *horizontalLayout_dialog_14;
    QLabel *label_after_fee_dialog;
    QLabel *label_after_fee_value_dialog;
    QSpacerItem *horizontalSpacer_dialog_3;
    QHBoxLayout *horizontalLayout_dialog;
    QPushButton *button_select_all_dialog;
    QSpacerItem *horizontalSpacer_dialog_4;
    QPushButton *button_toggle_lock_state_dialog;
    QSpacerItem *horizontalSpacer_dialog_5;
    QLabel *label_tree_mode_dialog;
    QSpacerItem *horizontalSpacer_dialog_6;
    QHBoxLayout *layout_tree_mode_toggle_dialog;
    QSpacerItem *horizontalSpacer_dialog_7;
    QLabel *label_list_mode_dialog;
    QSpacerItem *horizontalSpacer_dialog_8;
    QLabel *label_list_mode_value_dialog;
    QSpacerItem *horizontalSpacer_dialog_2;
    QVBoxLayout *verticalLayout_dialog_2;
    QHBoxLayout *horizontalLayout_dialog_2;
    QLabel *label_request_payments_history_dialog;
    QSpacerItem *horizontalSpacer_dialog_9;
    QHBoxLayout *horizontalLayout_dialog_5;
    QWidget *widget_table_dialog;
    QSpacerItem *verticalSpacer_dialog;
    QHBoxLayout *horizontalLayout_4_dialog;
    QPushButton *button_ok_control_dialog;
    QSpacerItem *horizontalSpacer_dialog;




    ToggleSwitch *switching_advance;
    QVBoxLayout *verticalLayout_advance;
    QLabel *label_transaction_fee_advance;
    QGridLayout *gridLayout_advance;
    QRadioButton *radio_custom_advance;
    QHBoxLayout *horizontalLayout_2_advance;
    QRadioButton *radio_recommened_advance;
    QLabel *label_recommened_advance;
    QSpacerItem *horizontalSpacer_advance;
    QVBoxLayout *verticalLayout_2_advance;
    QHBoxLayout *horizontalLayout_3_advance;
    QLabel *label_6_advance;
    QLabel *label_5_advance;
    QSlider *speed_slider_advance;
    QCheckBox *check_minimum_fee_advance;
    QLabel *label_2_advance;
    QHBoxLayout *horizontalLayout_5_advance;
    QLineEdit *lineEdit_advance;
    QLabel *label_8_advance;
    QLabel *label_3_advance;
    QHBoxLayout *horizontalLayout_4_advance;
    QLabel *label_9_advance;
    QWidget *widget_toggle_advance;
    QLabel *label_7_advance;
    QPushButton *button_ok_advance;







    QLabel **label_locks_table,**label_table_tree, **label_table_amount, **label_table_label, **label_table_address, **label_table_personal, **label_table_date, **label_table_confirmation;
    QCheckBox **check_table_control_dailog;
    QPixmap *pixmap_lock_control_dialog, *pixmap_tree_control_dailog;
    QIcon *lockIcon_control_dialog;
    QWidget *widget_table_data;
    QScrollArea *scrollArea;
    QGridLayout *loGrid;

    QLabel *label_amount_header, *label_space_header3, *label_label_header, *label_address_header, *label_personal_header, *label_date_header, *label_confirmation_header, *label_space_header1, *label_space_header2;


    int count;

    bool fNewRecipientAllowed;
    void send(QList<SendCoinsRecipient> recipients, QString strFee, QStringList formatted);
    bool fFeeMinimized;

    // Process WalletModel::SendCoinsReturn and generate a pair consisting
    // of a message and message flags for use in emit message().
    // Additional parameter msgArg can be used via .arg(msgArg).
    void processSendCoinsReturn(const WalletModel::SendCoinsReturn& sendCoinsReturn, const QString& msgArg = QString(), bool fPrepare = false);
    void minimizeFeeSection(bool fMinimize);
    void updateFeeMinimizedLabel();

private slots:
    void on_sendButton_clicked();
    void on_buttonChooseFee_clicked();
    void on_buttonMinimizeFee_clicked();
    void removeEntry(SendCoinsEntry* entry);
    void updateDisplayUnit();
    void updateSwiftTX();
    void coinControlFeatureChanged(bool);
    void coinControlButtonClicked();
    void coinControlChangeChecked(int);
    void coinControlChangeEdited(const QString&);
    void coinControlUpdateLabels();
    void coinControlClipboardQuantity();
    void coinControlClipboardAmount();
    void coinControlClipboardFee();
    void coinControlClipboardAfterFee();
    void coinControlClipboardBytes();
    void coinControlClipboardPriority();
    void coinControlClipboardLowOutput();
    void coinControlClipboardChange();
    void splitBlockChecked(int);
    void splitBlockLineEditChanged(const QString& text);
    void setMinimumFee();
    void updateFeeSectionControls();
    void updateMinFeeLabel();
    void updateSmartFeeLabel();
    void updateGlobalFeeVariables();

signals:
    // Fired when a message should be reported to the user
    void message(const QString& title, const QString& message, unsigned int style);
};



#endif // BITCOIN_QT_SENDCOINSDIALOG_H
