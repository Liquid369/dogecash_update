// Copyright (c) 2011-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_RECEIVECOINSDIALOG_H
#define BITCOIN_QT_RECEIVECOINSDIALOG_H

#include "guiutil.h"

#include <QDialog>
#include <QHeaderView>
#include <QItemSelection>
#include <QKeyEvent>
#include <QMenu>
#include <QPoint>
#include <QVariant>

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QSpacerItem>
#include <QGraphicsOpacityEffect>
#include <QScrollArea>
#include <QGridLayout>

class OptionsModel;
class WalletModel;



#include <QMouseEvent>
#include <QEnterEvent>
class LabelButton : public QLabel
{
    Q_OBJECT

public:
    LabelButton(QWidget *parent);
    LabelButton(QWidget *parent, QRect geo, QString imageString);
    LabelButton(QWidget *parent, int i);

    int getIndex();
public:
    int index;
private:
    void mouseReleaseEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    virtual void enterEvent(QEvent * event);
    virtual void leaveEvent(QEvent * event);
//    void mouseMoveEvent(QMouseEvent *event);

signals:
    void Clicked(int i);
    void Entered(int i);
    void Leaved(int i);
    void Pressed(LabelButton *label);
    void Selected();
};



namespace Ui
{
class ReceiveCoinsDialog;
}

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

/** Dialog for requesting payment of bitcoins */
class ReceiveCoinsDialog : public QDialog
{
    Q_OBJECT

public:
    enum ColumnWidths {
        DATE_COLUMN_WIDTH = 130,
        LABEL_COLUMN_WIDTH = 120,
        AMOUNT_MINIMUM_COLUMN_WIDTH = 160,
        MINIMUM_COLUMN_WIDTH = 130
    };

    explicit ReceiveCoinsDialog(QWidget* parent = 0);
    ~ReceiveCoinsDialog();

    void setModel(WalletModel* model);

    void loadPage();
    void load_Label_input();
    void set_main_font();
    void load_table_receive();

    void load_table_header();
    void load_table_body();

    void loadDialog();

public slots:
    void clear();
    void reject();
    void accept();

protected:
    virtual void keyPressEvent(QKeyEvent* event);

private:
    Ui::ReceiveCoinsDialog* ui;
    GUIUtil::TableViewLastColumnResizingFixer* columnResizingFixer;
    WalletModel* model;
    QMenu* contextMenu;

    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer;
    QGridLayout *gridLayout;
    QLabel *label_amount;
    QLabel *label_message;
    QLabel *label_label;
    QPushButton *button_request;
    QHBoxLayout *horizontalLayout_2;
    QSpacerItem *horizontalSpacer_3;
    QCheckBox *check_resuse_address;
    QLabel *label_reuse_address;
    QWidget *widget_label, *main_Widget;
    QVBoxLayout *verticalLayout_3;
    QVBoxLayout *verticalLayout_4;
    QLineEdit *edit_amount_value;
    QVBoxLayout *verticalLayout_5;
    QLineEdit *edit_message_value;
    QSpacerItem *horizontalSpacer_2;
    QVBoxLayout *verticalLayout_2;
    QLabel *label_table_title;
    QHBoxLayout *horizontalLayout_3;
    QWidget *widget_table;
    QSpacerItem *verticalSpacer;


    QVBoxLayout *verticalLayout_dialog;
    QSpacerItem *verticalSpacer_dialog;
    QHBoxLayout *horizontalLayout_dialog_5;
    QSpacerItem *horizontalSpacer_dialog_6;
    QVBoxLayout *verticalLayout_dialog_2;
    QHBoxLayout *horizontalLayout_dialog_2;
    QLabel *label_information_dialog;
    QSpacerItem *horizontalSpacer_dialog;
    QHBoxLayout *horizontalLayout_dialog_4;
    QLabel *label_uri_dialog;
    QLabel *label_uri_value_dialog;
    QSpacerItem *horizontalSpacer_dialog_2;
    QHBoxLayout *horizontalLayout_dialog_3;
    QLabel *label_address_dialog;
    QLabel *label_address_value_dialog;
    QSpacerItem *horizontalSpacer_dialog_3;
    QHBoxLayout *horizontalLayout_dialog;
    QPushButton *button_uri_dialog;
    QPushButton *button_address_dialog;
    QPushButton *button_remove_dialog;
    QSpacerItem *horizontalSpacer_dialog_4;
    QSpacerItem *horizontalSpacer_dialog_7;
    QSpacerItem *horizontalSpacer_dialog_5;
    QSpacerItem *verticalSpacer_dialog_2;
    QWidget *dialog_Widget;




    QLineEdit *edit_label_receive;
    QPushButton *button_search_label_receive;
    QPixmap *pixmap_button_search_receive;
    QIcon *icon_buttonsearch_receive;
    QFont *font_main_receive, *font_table_receive;
    QWidget *widget_table_data;
    QScrollArea *scrollArea;
    QLabel *label_table_date_header, *label_table_label_header, *label_table_message_header, *label_table_amount_header;
    QGridLayout *loGrid;
    QLabel **label_table_date, **label_table_label, **label_table_message, **label_table_amount;
    QGraphicsOpacityEffect *opacity_widget_table_receive, *opacity_label_label_receive, *opacity_label_amount_receive, *opacity_label_message_receive, *opacity_widget_label_receive, *opacity_edit_amount_receive, *opacity_edit_message_receive, *opacity_button_request_receive, *opacity_check_reuse_address, *opacity_label_reuse_address, *opacity_label_table_title_receive;

    int count;




    void copyColumnToClipboard(int column);
    virtual void resizeEvent(QResizeEvent* event);

private slots:
    void on_receiveButton_clicked();
    void on_showRequestButton_clicked();
    void on_removeRequestButton_clicked();
    void on_recentRequestsView_doubleClicked(const QModelIndex& index);
    void recentRequestsView_selectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
    void updateDisplayUnit();
    void showMenu(const QPoint& point);
    void copyLabel();
    void copyMessage();
    void copyAmount();
    void select_item(int i);
};

#endif // BITCOIN_QT_RECEIVECOINSDIALOG_H
