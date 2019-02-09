// Copyright (c) 2011-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "receivecoinsdialog.h"
#include "ui_receivecoinsdialog.h"

#include "addressbookpage.h"
#include "addresstablemodel.h"
#include "bitcoinunits.h"
#include "guiutil.h"
#include "optionsmodel.h"
#include "receiverequestdialog.h"
#include "recentrequeststablemodel.h"
#include "walletmodel.h"

#include <QAction>
#include <QCursor>
#include <QItemSelection>
#include <QMessageBox>
#include <QScrollBar>
#include <QTextDocument>




#include <QString>
#include <QRect>
#include <QCursor>
#include <QPixmap>


LabelButton::LabelButton(QWidget *parent) : QLabel(parent)
{
        this->setCursor(QCursor(Qt::PointingHandCursor));
        this->setStyleSheet(QString::fromUtf8("background:\"transparent\";"));
        this->setScaledContents(true);
        this->index = -1;
}
LabelButton::LabelButton(QWidget *parent, QRect geo, QString imageString) : QLabel(parent)
{
        this->setGeometry(geo);
        this->setCursor(QCursor(Qt::PointingHandCursor));
        this->setStyleSheet(QString::fromUtf8("background:\"transparent\";"));
        this->setScaledContents(true);
        this->setPixmap(QPixmap(imageString));
        this->index = -1;
}
LabelButton::LabelButton(QWidget *parent, int i): QLabel(parent){
    this->setCursor(QCursor(Qt::PointingHandCursor));
    this->setStyleSheet(QString::fromUtf8("background:\"transparent\";"));
    this->setScaledContents(true);
    this->index = i;
}

int LabelButton::getIndex()
{
    return this->index;
}

void LabelButton::mouseReleaseEvent(QMouseEvent *event){
    if (this->index == -1){
        emit Selected();
    } else {
        emit Clicked(this->index);
    }
    event->accept();
}

void LabelButton::mousePressEvent(QMouseEvent *event){
//    emit Clicked(this->index);
    emit Pressed(this);
    event->accept();
}

void LabelButton::enterEvent(QEvent *event)
{
    emit Entered(this->index);
    event->accept();
}

void LabelButton::leaveEvent(QEvent *event)
{
    emit Leaved(this->index);
    event->accept();
}





ReceiveCoinsDialog::ReceiveCoinsDialog(QWidget* parent) : QDialog(parent),
                                                          ui(new Ui::ReceiveCoinsDialog),
                                                          model(0)
{
    ui->setupUi(this);

#ifdef Q_OS_MAC // Icons on push buttons are very uncommon on Mac
    ui->clearButton->setIcon(QIcon());
    ui->receiveButton->setIcon(QIcon());
    ui->showRequestButton->setIcon(QIcon());
    ui->removeRequestButton->setIcon(QIcon());
#endif

    // context menu actions
    QAction* copyLabelAction = new QAction(tr("Copy label"), this);
    QAction* copyMessageAction = new QAction(tr("Copy message"), this);
    QAction* copyAmountAction = new QAction(tr("Copy amount"), this);

    // context menu
    contextMenu = new QMenu();
    contextMenu->addAction(copyLabelAction);
    contextMenu->addAction(copyMessageAction);
    contextMenu->addAction(copyAmountAction);

    // context menu signals
    connect(ui->recentRequestsView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showMenu(QPoint)));
    connect(copyLabelAction, SIGNAL(triggered()), this, SLOT(copyLabel()));
    connect(copyMessageAction, SIGNAL(triggered()), this, SLOT(copyMessage()));
    connect(copyAmountAction, SIGNAL(triggered()), this, SLOT(copyAmount()));

    connect(ui->clearButton, SIGNAL(clicked()), this, SLOT(clear()));

    ui->frame->hide();
    ui->frame2->hide();

    loadPage();
    loadDialog();
    load_Label_input();
    set_main_font();
    load_table_receive();
}

void ReceiveCoinsDialog::setModel(WalletModel* model)
{
    this->model = model;

    if (model && model->getOptionsModel()) {
        model->getRecentRequestsTableModel()->sort(RecentRequestsTableModel::Date, Qt::DescendingOrder);
        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));
        updateDisplayUnit();

        QTableView* tableView = ui->recentRequestsView;

        tableView->verticalHeader()->hide();
        tableView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        tableView->setModel(model->getRecentRequestsTableModel());
        tableView->setAlternatingRowColors(true);
        tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
        tableView->setSelectionMode(QAbstractItemView::ContiguousSelection);
        tableView->setColumnWidth(RecentRequestsTableModel::Date, DATE_COLUMN_WIDTH);
        tableView->setColumnWidth(RecentRequestsTableModel::Label, LABEL_COLUMN_WIDTH);

        connect(tableView->selectionModel(),
            SIGNAL(selectionChanged(QItemSelection, QItemSelection)), this,
            SLOT(recentRequestsView_selectionChanged(QItemSelection, QItemSelection)));
        // Last 2 columns are set by the columnResizingFixer, when the table geometry is ready.
        columnResizingFixer = new GUIUtil::TableViewLastColumnResizingFixer(tableView, AMOUNT_MINIMUM_COLUMN_WIDTH, DATE_COLUMN_WIDTH);
    }
}

void ReceiveCoinsDialog::loadPage()
{
    main_Widget = new QWidget(this);
//    main_Widget->resize(1178, 640);
    main_Widget->setGeometry(0, 158, this->width(), this->height()-190);
//    main_Widget->setStyleSheet(QLatin1String("QCheckBox::indicator:checked  {image: url(:/icon/check_white.png);}  QCheckBox::indicator:unchecked  { image: url(:/icon/uncheck.png);  }\n"
//"QCheckBox::indicator{width: 23px;height:23px;}"));
    verticalLayout = new QVBoxLayout(main_Widget);
    verticalLayout->setSpacing(0);
//    verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
    verticalLayout->setContentsMargins(0, 29, 0, 0);
    horizontalLayout = new QHBoxLayout();
//    horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
    horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    horizontalLayout->addItem(horizontalSpacer);

    gridLayout = new QGridLayout();
//    gridLayout->setObjectName(QStringLiteral("gridLayout"));
    gridLayout->setHorizontalSpacing(17);
    gridLayout->setVerticalSpacing(14);
    label_amount = new QLabel(main_Widget);
    label_amount->setObjectName(QStringLiteral("label_amount"));
    QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
//    sizePolicy.setHorizontalStretch(0);
//    sizePolicy.setVerticalStretch(0);
//    sizePolicy.setHeightForWidth(label_amount->sizePolicy().hasHeightForWidth());
    label_amount->setSizePolicy(sizePolicy);
    label_amount->setMinimumSize(QSize(130, 54));
    label_amount->setMaximumSize(QSize(130, 54));
    label_amount->setStyleSheet(QLatin1String("background-color: transparent;\n"
"color: #ffffff;"));
    label_amount->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

    gridLayout->addWidget(label_amount, 1, 0, 1, 1);

    label_message = new QLabel(main_Widget);
//    label_message->setObjectName(QStringLiteral("label_message"));
//    sizePolicy.setHeightForWidth(label_message->sizePolicy().hasHeightForWidth());
    label_message->setSizePolicy(sizePolicy);
    label_message->setMinimumSize(QSize(130, 54));
    label_message->setMaximumSize(QSize(130, 54));
    label_message->setStyleSheet(QLatin1String("background-color: transparent;\n"
"color: #ffffff;"));
    label_message->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

    gridLayout->addWidget(label_message, 2, 0, 1, 1);

    label_label = new QLabel(main_Widget);
//    label_label->setObjectName(QStringLiteral("label_label"));
//    sizePolicy.setHeightForWidth(label_label->sizePolicy().hasHeightForWidth());
    label_label->setSizePolicy(sizePolicy);
    label_label->setMinimumSize(QSize(130, 49));
    label_label->setMaximumSize(QSize(130, 49));
    label_label->setStyleSheet(QLatin1String("background-color: transparent;\n"
"color: #ffffff;"));
    label_label->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

    gridLayout->addWidget(label_label, 0, 0, 1, 1);

    button_request = new QPushButton(main_Widget);
//    button_request->setObjectName(QStringLiteral("button_request"));
    sizePolicy.setHeightForWidth(button_request->sizePolicy().hasHeightForWidth());
    button_request->setSizePolicy(sizePolicy);
    button_request->setMinimumSize(QSize(130, 49));
    button_request->setMaximumSize(QSize(130, 49));
    button_request->setStyleSheet(QLatin1String("background-color: #b6a27e;\n"
"color: #f2f2f2; margin-top: 5px"));

    gridLayout->addWidget(button_request, 2, 2, 1, 1);

    horizontalLayout_2 = new QHBoxLayout();
    horizontalLayout_2->setSpacing(7);
//    horizontalLayout_2->setObjectName(QStringLiteral("horizontalLayout_2"));
    horizontalSpacer_3 = new QSpacerItem(40, 13, QSizePolicy::Expanding, QSizePolicy::Minimum);

    horizontalLayout_2->addItem(horizontalSpacer_3);

    check_resuse_address = new QCheckBox(main_Widget);
//    check_resuse_address->setObjectName(QStringLiteral("check_resuse_address"));
//    sizePolicy.setHeightForWidth(check_resuse_address->sizePolicy().hasHeightForWidth());
    check_resuse_address->setSizePolicy(sizePolicy);
    check_resuse_address->setMinimumSize(QSize(23, 23));
    check_resuse_address->setMaximumSize(QSize(23, 23));
    check_resuse_address->setStyleSheet(QStringLiteral("background-color: transparent"));
    check_resuse_address->setIconSize(QSize(23, 23));

    horizontalLayout_2->addWidget(check_resuse_address);

    label_reuse_address = new QLabel(main_Widget);
//    label_reuse_address->setObjectName(QStringLiteral("label_reuse_address"));
    QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Fixed);
//    sizePolicy1.setHorizontalStretch(0);
//    sizePolicy1.setVerticalStretch(18);
//    sizePolicy1.setHeightForWidth(label_reuse_address->sizePolicy().hasHeightForWidth());
    label_reuse_address->setSizePolicy(sizePolicy1);
    label_reuse_address->setMinimumSize(QSize(0, 18));
    label_reuse_address->setMaximumSize(QSize(16777215, 18));
    label_reuse_address->setStyleSheet(QLatin1String("color: #f2f2f2;\n"
"background-color: transparent;"));

    horizontalLayout_2->addWidget(label_reuse_address);


    gridLayout->addLayout(horizontalLayout_2, 3, 1, 1, 1);

    widget_label = new QWidget(main_Widget);
    widget_label->setObjectName(QStringLiteral("widget_label"));
//    sizePolicy.setHeightForWidth(widget_label->sizePolicy().hasHeightForWidth());
    widget_label->setSizePolicy(sizePolicy);
    widget_label->setMinimumSize(QSize(467, 49));
    widget_label->setMaximumSize(QSize(467, 49));
    widget_label->setStyleSheet(QStringLiteral("background-color: #29333d;"));

    gridLayout->addWidget(widget_label, 0, 1, 1, 1);

    verticalLayout_3 = new QVBoxLayout();
//    verticalLayout_3->setObjectName(QStringLiteral("verticalLayout_3"));

    gridLayout->addLayout(verticalLayout_3, 1, 2, 1, 1);

    verticalLayout_4 = new QVBoxLayout();
    verticalLayout_4->setSpacing(0);
//    verticalLayout_4->setObjectName(QStringLiteral("verticalLayout_4"));
    verticalLayout_4->setContentsMargins(-1, 5, -1, 0);
    edit_amount_value = new QLineEdit(main_Widget);
//    edit_amount_value->setObjectName(QStringLiteral("edit_amount_value"));
//    sizePolicy.setHeightForWidth(edit_amount_value->sizePolicy().hasHeightForWidth());
    edit_amount_value->setSizePolicy(sizePolicy);
    edit_amount_value->setMinimumSize(QSize(467, 54));
    edit_amount_value->setMaximumSize(QSize(467, 54));
    edit_amount_value->setStyleSheet(QLatin1String("background-color:#29333d;\n"
"color: #ffdfb5;"));
    edit_amount_value->setFrame(false);

    verticalLayout_4->addWidget(edit_amount_value);


    gridLayout->addLayout(verticalLayout_4, 1, 1, 1, 1);

    verticalLayout_5 = new QVBoxLayout();
    verticalLayout_5->setSpacing(0);
//    verticalLayout_5->setObjectName(QStringLiteral("verticalLayout_5"));
    verticalLayout_5->setContentsMargins(-1, 5, -1, -1);
    edit_message_value = new QLineEdit(main_Widget);
//    edit_message_value->setObjectName(QStringLiteral("edit_message_value"));
    QSizePolicy sizePolicy2(QSizePolicy::Fixed, QSizePolicy::Fixed);
//    sizePolicy2.setHorizontalStretch(46);
//    sizePolicy2.setVerticalStretch(49);
//    sizePolicy2.setHeightForWidth(edit_message_value->sizePolicy().hasHeight/ForWidth());
    edit_message_value->setSizePolicy(sizePolicy2);
    edit_message_value->setMinimumSize(QSize(467, 54));
    edit_message_value->setMaximumSize(QSize(467, 54));
    edit_message_value->setStyleSheet(QLatin1String("background-color: #29333d;\n"
"color: #ffdfb5;"));
    edit_message_value->setFrame(false);

    verticalLayout_5->addWidget(edit_message_value);


    gridLayout->addLayout(verticalLayout_5, 2, 1, 1, 1);


    horizontalLayout->addLayout(gridLayout);

    horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    horizontalLayout->addItem(horizontalSpacer_2);


    verticalLayout->addLayout(horizontalLayout);

    verticalLayout_2 = new QVBoxLayout();
    verticalLayout_2->setSpacing(0);
//    verticalLayout_2->setObjectName(QStringLiteral("verticalLayout_2"));
    verticalLayout_2->setContentsMargins(-1, 9, -1, 20);
    label_table_title = new QLabel(main_Widget);
//    label_table_title->setObjectName(QStringLiteral("label_table_title"));
    QSizePolicy sizePolicy3(QSizePolicy::Preferred, QSizePolicy::Fixed);
//    sizePolicy3.setHorizontalStretch(0);
//    sizePolicy3.setVerticalStretch(0);
//    sizePolicy3.setHeightForWidth(label_table_title->sizePolicy().hasHeightForWidth());
    label_table_title->setSizePolicy(sizePolicy3);
    label_table_title->setStyleSheet(QLatin1String("background-color: transparent;\n"
"padding-left: 97px;\n"
"color: #ffffff;\n"
"padding-bottom: 11px;"));

    verticalLayout_2->addWidget(label_table_title);

    horizontalLayout_3 = new QHBoxLayout();
//    horizontalLayout_3->setObjectName(QStringLiteral("horizontalLayout_3"));
    widget_table = new QWidget(main_Widget);
//    widget_table->setObjectName(QStringLiteral("widget_table"));
    QSizePolicy sizePolicy4(QSizePolicy::Preferred, QSizePolicy::Preferred);
//    sizePolicy4.setHorizontalStretch(0);
//    sizePolicy4.setVerticalStretch(0);
//    sizePolicy4.setHeightForWidth(widget_table->sizePolicy().hasHeightForWidth());
    widget_table->setSizePolicy(sizePolicy4);
    widget_table->setMinimumSize(QSize(0, 0));
    widget_table->setStyleSheet(QStringLiteral("background-color: blue;"));

    horizontalLayout_3->addWidget(widget_table);


    verticalLayout_2->addLayout(horizontalLayout_3);

    verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

    verticalLayout_2->addItem(verticalSpacer);


    verticalLayout->addLayout(verticalLayout_2);
    label_amount->setText(QApplication::translate("receive", "AMOUNT", Q_NULLPTR));
    label_message->setText(QApplication::translate("receive", "MESSAGE", Q_NULLPTR));
    label_label->setText(QApplication::translate("receive", "LABEL", Q_NULLPTR));
    button_request->setText(QApplication::translate("receive", "REQUEST", Q_NULLPTR));
    check_resuse_address->setText(QString());
    label_reuse_address->setText(QApplication::translate("receive", "REUSE EXISTING ADDRESS ", Q_NULLPTR));
    label_table_title->setText(QApplication::translate("receive", "REQUESTED PAYMENTS HISTORY", Q_NULLPTR));

    main_Widget->show();
}

void ReceiveCoinsDialog::load_Label_input()
{
    widget_label->setLayout(new QHBoxLayout());
    edit_label_receive = new QLineEdit();
    button_search_label_receive = new QPushButton();
    widget_label->layout()->setMargin(0);
    widget_label->layout()->setSpacing(0);
    widget_label->layout()->setContentsMargins(0,0,0,0);
    widget_label->layout()->addWidget(edit_label_receive);
    widget_label->layout()->addWidget(button_search_label_receive);
    edit_label_receive->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    edit_label_receive->setMinimumHeight(49);
    edit_label_receive->setMaximumHeight(49);
    edit_label_receive->setFrame(false);
    edit_label_receive->setStyleSheet("background-color: transparent; color: #ffdfb5");

    button_search_label_receive->setMinimumSize(49, 49);
    button_search_label_receive->setMaximumSize(49, 49);
    button_search_label_receive->setStyleSheet("background-color: transparent; color: #ffdfb5;");


    pixmap_button_search_receive = new QPixmap(":/icons/res/icons/search.png");
    icon_buttonsearch_receive = new QIcon(*pixmap_button_search_receive);
    button_search_label_receive->setIcon(*icon_buttonsearch_receive);
    button_search_label_receive->setIconSize(QSize(24,24));
}

void ReceiveCoinsDialog::set_main_font()
{
    font_main_receive = new QFont();
    font_main_receive->setFamily("Chivo");
    font_main_receive->setPixelSize(15);
    font_main_receive->setLetterSpacing(QFont::AbsoluteSpacing, 1.4);
    font_main_receive->setBold(true);

    edit_label_receive->setFont(*font_main_receive);
    label_label->setFont(*font_main_receive);
    label_amount->setFont(*font_main_receive);
    label_message->setFont(*font_main_receive);
    edit_amount_value->setFont(*font_main_receive);
    edit_message_value->setFont(*font_main_receive);
    button_request->setFont(*font_main_receive);
    label_table_title->setFont(*font_main_receive);

    font_table_receive = new QFont();
    font_table_receive->setFamily("Chivo");
    font_table_receive->setLetterSpacing(QFont::AbsoluteSpacing, 1.2);
    font_table_receive->setPixelSize(13);

    label_reuse_address->setFont(*font_table_receive);
}

void ReceiveCoinsDialog::load_table_receive()
{
    widget_table_data = new QWidget();
    widget_table_data->setStyleSheet("background-color: transparent");
    scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setWidget(widget_table_data);
    scrollArea->setStyleSheet("background-color:transparent;");
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setGeometry(0, 0, 10, 10);
    loGrid = new QGridLayout();
    widget_table_data->setLayout(loGrid);
    loGrid->setMargin(0);
    loGrid->setHorizontalSpacing(0);
    loGrid->setVerticalSpacing(0);

    widget_table->setLayout(new QVBoxLayout());
    widget_table->setStyleSheet("background-color: transparent");
    widget_table->layout()->addWidget(scrollArea);
//    ui->widget_table->layout()->setSpacerItem();
    widget_table->layout()->setMargin(0);
    widget_table->layout()->setSpacing(0);
    widget_table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    widget_table->setMinimumHeight(0);

    this->load_table_header();
    this->load_table_body();

}

void ReceiveCoinsDialog::load_table_header()
{
    label_table_date_header = new QLabel("DATE");
    label_table_date_header->setStyleSheet("background-color: #1f272e; color: #b99d7c; padding-top: 17px; padding-bottom: 10px; padding-left: 97px; padding-right: 10px;");
    label_table_date_header->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    label_table_date_header->setMinimumHeight(41);
    label_table_date_header->setMaximumHeight(41);
    label_table_date_header->setFont(*font_table_receive);
    loGrid->addWidget(label_table_date_header, 0, 0);

    label_table_date_header->show();

    label_table_label_header = new QLabel("LABEL");
    label_table_label_header->setStyleSheet("background-color: #1f272e; color: #b99d7c; padding-top: 17px; padding-bottom: 10px; padding-left: 57px; padding-right: 10px;");
    label_table_label_header->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    label_table_label_header->setMinimumHeight(41);
    label_table_label_header->setMaximumHeight(41);
    label_table_label_header->setFont(*font_table_receive);
    loGrid->addWidget(label_table_label_header, 0, 1);

    label_table_message_header = new QLabel("MESSAGE");
    label_table_message_header->setStyleSheet("background-color: #1f272e; color: #b99d7c; padding-top: 17px; padding-bottom: 10px; padding-left: 57px; padding-right: 10px;");
    label_table_message_header->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    label_table_message_header->setMinimumHeight(41);
    label_table_message_header->setMaximumHeight(41);
    label_table_message_header->setFont(*font_table_receive);
    loGrid->addWidget(label_table_message_header, 0, 2);

    label_table_amount_header = new QLabel("AMOUNT (DOGEC)");
    label_table_amount_header->setStyleSheet("background-color: #1f272e; color: #b99d7c; padding-top: 17px; padding-bottom: 10px; padding-left: 57px; padding-right: 10px;");
    label_table_amount_header->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    label_table_amount_header->setMinimumHeight(41);
    label_table_amount_header->setMaximumHeight(41);
    label_table_amount_header->setFont(*font_table_receive);
    loGrid->addWidget(label_table_amount_header, 0, 3);
}

void ReceiveCoinsDialog::load_table_body()
{
    count = 15;
    label_table_label = new QLabel*[count];
    label_table_amount = new QLabel*[count];
    label_table_message = new QLabel*[count];
    label_table_date = new QLabel*[count];

    for (int i = 0; i<count; i++){

        label_table_date[i] = new LabelButton(this, i);
        label_table_date[i]->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        label_table_date[i]->setMinimumSize(290, 29);
        label_table_date[i]->setMaximumSize(290, 29);
        label_table_date[i]->setText(QString::number(i) + ": 02/29/2018 03:55AM");
        loGrid->addWidget(label_table_date[i], i+1, 0);
        label_table_date[i]->show();
        label_table_date[i]->setFont(*font_table_receive);
        if (i % 2 == 0) {
            label_table_date[i]->setStyleSheet("background-color: #2a3036; color: #f2f2f2; padding-top: 8px; padding-bottom: 8px; padding-left: 97px; padding-right: 10px");
        } else {
            label_table_date[i]->setStyleSheet("background-color: #1f272e; color: #f2f2f2; padding-top: 8px; padding-bottom: 8px; padding-left: 97px; padding-right: 10px");
        }
        connect(label_table_date[i], SIGNAL(Clicked(int)), SLOT(select_item(int )));
        connect(label_table_date[i], SIGNAL(Entered(int)), SLOT(hoverd_row(int)));
        connect(label_table_date[i], SIGNAL(Leaved(int)), SLOT(leaved_row(int)));


        label_table_label[i] = new LabelButton(this, i);
        label_table_label[i]->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        label_table_label[i]->setMinimumSize(160, 29);
        label_table_label[i]->setMaximumSize(160, 29);
        label_table_label[i]->setText("(No label) " + QString::number(i));
        loGrid->addWidget(label_table_label[i], i+1, 1);
        label_table_label[i]->show();
        label_table_label[i]->setFont(*font_table_receive);

        if (i % 2 == 0) {
            label_table_label[i]->setStyleSheet("background-color: #2a3036; color: #f2f2f2; padding-top: 8px; padding-bottom: 8px; padding-left: 57px; padding-right: 10px");
        } else {
            label_table_label[i]->setStyleSheet("background-color: #1f272e; color: #f2f2f2; padding-top: 8px; padding-bottom: 8px; padding-left: 57px; padding-right: 10px");
        }

        connect(label_table_label[i], SIGNAL(Clicked(int)), SLOT(select_item(int )));
        connect(label_table_label[i], SIGNAL(Entered(int)), SLOT(hoverd_row(int)));
        connect(label_table_label[i], SIGNAL(Leaved(int)), SLOT(leaved_row(int)));

        label_table_message[i] = new LabelButton(this, i);
        label_table_message[i]->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        label_table_message[i]->setMinimumHeight(29);
        label_table_message[i]->setMaximumHeight(29);
        label_table_message[i]->setText("SHARED MASTERNODE WITH PEDRO" + QString::number(i));
        loGrid->addWidget(label_table_message[i], i+1, 2);
        label_table_message[i]->show();
        label_table_message[i]->setFont(*font_table_receive);

        if (i % 2 == 0) {
            label_table_message[i]->setStyleSheet("background-color: #2a3036; color: #f2f2f2; padding-top: 8px; padding-bottom: 8px; padding-left: 57px; padding-right: 10px");
        } else {
            label_table_message[i]->setStyleSheet("background-color: #1f272e; color: #f2f2f2; padding-top: 8px; padding-bottom: 8px; padding-left: 57px; padding-right: 10px");
        }
        connect(label_table_message[i], SIGNAL(Clicked(int)), SLOT(select_item(int )));
        connect(label_table_message[i], SIGNAL(Entered(int)), SLOT(hoverd_row(int)));
        connect(label_table_message[i], SIGNAL(Leaved(int)), SLOT(leaved_row(int)));

        label_table_amount[i] = new LabelButton(this, i);
        label_table_amount[i]->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        label_table_amount[i]->setMinimumHeight(29);
        label_table_amount[i]->setMaximumHeight(29);
        label_table_amount[i]->setText(QString::number(i) + ": 1,324.55231");
        loGrid->addWidget(label_table_amount[i], i+1, 3);
        label_table_amount[i]->show();
        label_table_amount[i]->setFont(*font_table_receive);
        if (i % 2 == 0) {
            label_table_amount[i]->setStyleSheet("background-color: #2a3036; color: #f2f2f2; padding-top: 8px; padding-bottom: 8px; padding-left: 57px; padding-right: 10px");
        } else {
            label_table_amount[i]->setStyleSheet("background-color: #1f272e; color: #f2f2f2; padding-top: 8px; padding-bottom: 8px; padding-left: 57px; padding-right: 10px");
        }
        connect(label_table_amount[i], SIGNAL(Clicked(int)), SLOT(select_item(int)));
        connect(label_table_amount[i], SIGNAL(Entered(int)), SLOT(hoverd_row(int)));
        connect(label_table_amount[i], SIGNAL(Leaved(int)), SLOT(leaved_row(int)));
    }
}

void ReceiveCoinsDialog::loadDialog()
{
    dialog_Widget = new QWidget(this);
    dialog_Widget->setGeometry(0, 158, this->width(), this->height() - 190);
    verticalLayout_dialog = new QVBoxLayout(dialog_Widget);
    verticalLayout_dialog->setSpacing(22);
    verticalLayout_dialog->setObjectName(QStringLiteral("verticalLayout_dialog"));
    verticalLayout_dialog->setContentsMargins(31, 0, 27, 0);
    verticalSpacer_dialog = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

    verticalLayout_dialog->addItem(verticalSpacer_dialog);

    horizontalLayout_dialog_5 = new QHBoxLayout();
    horizontalLayout_dialog_5->setObjectName(QStringLiteral("horizontalLayout_dialog_5"));
    horizontalSpacer_dialog_6 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    horizontalLayout_dialog_5->addItem(horizontalSpacer_dialog_6);

    verticalLayout_dialog_2 = new QVBoxLayout();
    verticalLayout_dialog_2->setSpacing(22);
    verticalLayout_dialog_2->setObjectName(QStringLiteral("verticalLayout_dialog_2"));
    horizontalLayout_dialog_2 = new QHBoxLayout();
    horizontalLayout_dialog_2->setObjectName(QStringLiteral("horizontalLayout_dialog_2"));
    label_information_dialog = new QLabel(dialog_Widget);
    label_information_dialog->setObjectName(QStringLiteral("label_information_dialog"));
    QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(label_information_dialog->sizePolicy().hasHeightForWidth());
    label_information_dialog->setSizePolicy(sizePolicy);
    label_information_dialog->setMinimumSize(QSize(209, 18));
    label_information_dialog->setMaximumSize(QSize(209, 18));
    label_information_dialog->setStyleSheet(QLatin1String("background-color: transparent;\n"
"color: #ffffff;"));

    horizontalLayout_dialog_2->addWidget(label_information_dialog);

    horizontalSpacer_dialog = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    horizontalLayout_dialog_2->addItem(horizontalSpacer_dialog);


    verticalLayout_dialog_2->addLayout(horizontalLayout_dialog_2);

    horizontalLayout_dialog_4 = new QHBoxLayout();
    horizontalLayout_dialog_4->setSpacing(61);
    horizontalLayout_dialog_4->setObjectName(QStringLiteral("horizontalLayout_dialog_4"));
    horizontalLayout_dialog_4->setContentsMargins(-1, 2, -1, -1);
    label_uri_dialog = new QLabel(dialog_Widget);
    label_uri_dialog->setObjectName(QStringLiteral("label_uri_dialog"));
    sizePolicy.setHeightForWidth(label_uri_dialog->sizePolicy().hasHeightForWidth());
    label_uri_dialog->setSizePolicy(sizePolicy);
    label_uri_dialog->setMinimumSize(QSize(37, 18));
    label_uri_dialog->setMaximumSize(QSize(37, 18));
    label_uri_dialog->setStyleSheet(QLatin1String("background-color: transparent;\n"
"color: #ffffff;"));

    horizontalLayout_dialog_4->addWidget(label_uri_dialog);

    label_uri_value_dialog = new QLabel(dialog_Widget);
    label_uri_value_dialog->setObjectName(QStringLiteral("label_uri_value_dialog"));
    QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Fixed);
    sizePolicy1.setHorizontalStretch(0);
    sizePolicy1.setVerticalStretch(0);
    sizePolicy1.setHeightForWidth(label_uri_value_dialog->sizePolicy().hasHeightForWidth());
    label_uri_value_dialog->setSizePolicy(sizePolicy1);
    label_uri_value_dialog->setMinimumSize(QSize(0, 18));
    label_uri_value_dialog->setMaximumSize(QSize(16777215, 18));
    label_uri_value_dialog->setStyleSheet(QLatin1String("background-color: transparent;\n"
"color: #ffdfb5;"));

    horizontalLayout_dialog_4->addWidget(label_uri_value_dialog);

    horizontalSpacer_dialog_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    horizontalLayout_dialog_4->addItem(horizontalSpacer_dialog_2);


    verticalLayout_dialog_2->addLayout(horizontalLayout_dialog_4);

    horizontalLayout_dialog_3 = new QHBoxLayout();
    horizontalLayout_dialog_3->setSpacing(14);
    horizontalLayout_dialog_3->setObjectName(QStringLiteral("horizontalLayout_dialog_3"));
    label_address_dialog = new QLabel(dialog_Widget);
    label_address_dialog->setObjectName(QStringLiteral("label_address_dialog"));
    sizePolicy.setHeightForWidth(label_address_dialog->sizePolicy().hasHeightForWidth());
    label_address_dialog->setSizePolicy(sizePolicy);
    label_address_dialog->setMinimumSize(QSize(84, 18));
    label_address_dialog->setMaximumSize(QSize(84, 81));
    label_address_dialog->setStyleSheet(QLatin1String("background-color: transparent;\n"
"color: #ffffff;"));

    horizontalLayout_dialog_3->addWidget(label_address_dialog);

    label_address_value_dialog = new QLabel(dialog_Widget);
    label_address_value_dialog->setObjectName(QStringLiteral("label_address_value_dialog"));
    sizePolicy1.setHeightForWidth(label_address_value_dialog->sizePolicy().hasHeightForWidth());
    label_address_value_dialog->setSizePolicy(sizePolicy1);
    label_address_value_dialog->setMinimumSize(QSize(0, 18));
    label_address_value_dialog->setMaximumSize(QSize(16777215, 18));
    label_address_value_dialog->setStyleSheet(QLatin1String("background-color: transparent;\n"
"color: #ffdfb5;"));

    horizontalLayout_dialog_3->addWidget(label_address_value_dialog);

    horizontalSpacer_dialog_3 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    horizontalLayout_dialog_3->addItem(horizontalSpacer_dialog_3);


    verticalLayout_dialog_2->addLayout(horizontalLayout_dialog_3);

    horizontalLayout_dialog = new QHBoxLayout();
    horizontalLayout_dialog->setSpacing(22);
    horizontalLayout_dialog->setObjectName(QStringLiteral("horizontalLayout_dialog"));
    horizontalLayout_dialog->setContentsMargins(-1, 51, -1, -1);
    button_uri_dialog = new QPushButton(dialog_Widget);
    button_uri_dialog->setObjectName(QStringLiteral("button_uri_dialog"));
    sizePolicy.setHeightForWidth(button_uri_dialog->sizePolicy().hasHeightForWidth());
    button_uri_dialog->setSizePolicy(sizePolicy);
    button_uri_dialog->setMinimumSize(QSize(150, 49));
    button_uri_dialog->setMaximumSize(QSize(150, 49));
    button_uri_dialog->setStyleSheet(QLatin1String("background-color: #b6a27e;\n"
"color: #f2f2f2;"));

    horizontalLayout_dialog->addWidget(button_uri_dialog);

    button_address_dialog = new QPushButton(dialog_Widget);
    button_address_dialog->setObjectName(QStringLiteral("button_address_dialog"));
    QSizePolicy sizePolicy2(QSizePolicy::Minimum, QSizePolicy::Fixed);
    sizePolicy2.setHorizontalStretch(0);
    sizePolicy2.setVerticalStretch(0);
    sizePolicy2.setHeightForWidth(button_address_dialog->sizePolicy().hasHeightForWidth());
    button_address_dialog->setSizePolicy(sizePolicy2);
    button_address_dialog->setMinimumSize(QSize(180, 49));
    button_address_dialog->setMaximumSize(QSize(180, 49));
    button_address_dialog->setStyleSheet(QLatin1String("background-color: #b6a27e;\n"
"color: #f2f2f2;"));

    horizontalLayout_dialog->addWidget(button_address_dialog);

    button_remove_dialog = new QPushButton(dialog_Widget);
    button_remove_dialog->setObjectName(QStringLiteral("button_remove_dialog"));
    sizePolicy.setHeightForWidth(button_remove_dialog->sizePolicy().hasHeightForWidth());
    button_remove_dialog->setSizePolicy(sizePolicy);
    button_remove_dialog->setMinimumSize(QSize(150, 49));
    button_remove_dialog->setStyleSheet(QLatin1String("background-color: #b6a27e;\n"
"color: #f2f2f2;"));

    horizontalLayout_dialog->addWidget(button_remove_dialog);

    horizontalSpacer_dialog_4 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    horizontalLayout_dialog->addItem(horizontalSpacer_dialog_4);


    verticalLayout_dialog_2->addLayout(horizontalLayout_dialog);

    horizontalSpacer_dialog_7 = new QSpacerItem(585, 104, QSizePolicy::Fixed, QSizePolicy::Minimum);

    verticalLayout_dialog_2->addItem(horizontalSpacer_dialog_7);


    horizontalLayout_dialog_5->addLayout(verticalLayout_dialog_2);

    horizontalSpacer_dialog_5 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    horizontalLayout_dialog_5->addItem(horizontalSpacer_dialog_5);


    verticalLayout_dialog->addLayout(horizontalLayout_dialog_5);

    verticalSpacer_dialog_2 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

    verticalLayout_dialog->addItem(verticalSpacer_dialog_2);

    label_information_dialog->setText(QApplication::translate("ReceiveDailog", "PAYMENT INFORMATION", Q_NULLPTR));
    label_uri_dialog->setText(QApplication::translate("ReceiveDailog", "URI: ", Q_NULLPTR));
    label_uri_value_dialog->setText(QApplication::translate("ReceiveDailog", "dogecash:DF4zEBdZeG5UyQvmz4a9LDk4w6QrHMdBa4", Q_NULLPTR));
    label_address_dialog->setText(QApplication::translate("ReceiveDailog", "ADDRESS:", Q_NULLPTR));
    label_address_value_dialog->setText(QApplication::translate("ReceiveDailog", "DF4zEBdZeG5UyQvmz4a9LDk4w6QrHMdBa4", Q_NULLPTR));
    button_uri_dialog->setText(QApplication::translate("ReceiveDailog", "COPY URI", Q_NULLPTR));
    button_address_dialog->setText(QApplication::translate("ReceiveDailog", "COPY ADDRESS", Q_NULLPTR));
    button_remove_dialog->setText(QApplication::translate("ReceiveDailog", "Remove", Q_NULLPTR));

    dialog_Widget->hide();
}



ReceiveCoinsDialog::~ReceiveCoinsDialog()
{
    delete ui;
}

void ReceiveCoinsDialog::clear()
{
    ui->reqAmount->clear();
    ui->reqLabel->setText("");
    ui->reqMessage->setText("");
    ui->reuseAddress->setChecked(false);
    updateDisplayUnit();
}

void ReceiveCoinsDialog::reject()
{
    clear();
}

void ReceiveCoinsDialog::accept()
{
    clear();
}

void ReceiveCoinsDialog::updateDisplayUnit()
{
    if (model && model->getOptionsModel()) {
        ui->reqAmount->setDisplayUnit(model->getOptionsModel()->getDisplayUnit());
    }
}

void ReceiveCoinsDialog::on_receiveButton_clicked()
{
    if (!model || !model->getOptionsModel() || !model->getAddressTableModel() || !model->getRecentRequestsTableModel())
        return;

    QString address;
    QString label = ui->reqLabel->text();
    if (ui->reuseAddress->isChecked()) {
        /* Choose existing receiving address */
        AddressBookPage dlg(AddressBookPage::ForSelection, AddressBookPage::ReceivingTab, this);
        dlg.setModel(model->getAddressTableModel());
        if (dlg.exec()) {
            address = dlg.getReturnValue();
            if (label.isEmpty()) /* If no label provided, use the previously used label */
            {
                label = model->getAddressTableModel()->labelForAddress(address);
            }
        } else {
            return;
        }
    } else {
        /* Generate new receiving address */
        address = model->getAddressTableModel()->addRow(AddressTableModel::Receive, label, "");
    }
    SendCoinsRecipient info(address, label,
        ui->reqAmount->value(), ui->reqMessage->text());
    ReceiveRequestDialog* dialog = new ReceiveRequestDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setModel(model->getOptionsModel());
    dialog->setInfo(info);
    dialog->show();
    clear();

    /* Store request for later reference */
    model->getRecentRequestsTableModel()->addNewRequest(info);
}

void ReceiveCoinsDialog::on_recentRequestsView_doubleClicked(const QModelIndex& index)
{
    const RecentRequestsTableModel* submodel = model->getRecentRequestsTableModel();
    ReceiveRequestDialog* dialog = new ReceiveRequestDialog(this);
    dialog->setModel(model->getOptionsModel());
    dialog->setInfo(submodel->entry(index.row()).recipient);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}

void ReceiveCoinsDialog::recentRequestsView_selectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
    // Enable Show/Remove buttons only if anything is selected.
    bool enable = !ui->recentRequestsView->selectionModel()->selectedRows().isEmpty();
    ui->showRequestButton->setEnabled(enable);
    ui->removeRequestButton->setEnabled(enable);
}

void ReceiveCoinsDialog::on_showRequestButton_clicked()
{
    if (!model || !model->getRecentRequestsTableModel() || !ui->recentRequestsView->selectionModel())
        return;
    QModelIndexList selection = ui->recentRequestsView->selectionModel()->selectedRows();

    foreach (QModelIndex index, selection) {
        on_recentRequestsView_doubleClicked(index);
    }
}

void ReceiveCoinsDialog::on_removeRequestButton_clicked()
{
    if (!model || !model->getRecentRequestsTableModel() || !ui->recentRequestsView->selectionModel())
        return;
    QModelIndexList selection = ui->recentRequestsView->selectionModel()->selectedRows();
    if (selection.empty())
        return;
    // correct for selection mode ContiguousSelection
    QModelIndex firstIndex = selection.at(0);
    model->getRecentRequestsTableModel()->removeRows(firstIndex.row(), selection.length(), firstIndex.parent());
}

// We override the virtual resizeEvent of the QWidget to adjust tables column
// sizes as the tables width is proportional to the dialogs width.
void ReceiveCoinsDialog::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    columnResizingFixer->stretchColumnWidth(RecentRequestsTableModel::Message);
    main_Widget->setGeometry(0, 158, this->width(), this->height()-190);
    dialog_Widget->setGeometry(0, 158, this->width(), this->height()-190);
}

void ReceiveCoinsDialog::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Return) {
        // press return -> submit form
        if (ui->reqLabel->hasFocus() || ui->reqAmount->hasFocus() || ui->reqMessage->hasFocus()) {
            event->ignore();
            on_receiveButton_clicked();
            return;
        }
    }

    this->QDialog::keyPressEvent(event);
}

// copy column of selected row to clipboard
void ReceiveCoinsDialog::copyColumnToClipboard(int column)
{
    if (!model || !model->getRecentRequestsTableModel() || !ui->recentRequestsView->selectionModel())
        return;
    QModelIndexList selection = ui->recentRequestsView->selectionModel()->selectedRows();
    if (selection.empty())
        return;
    // correct for selection mode ContiguousSelection
    QModelIndex firstIndex = selection.at(0);
    GUIUtil::setClipboard(model->getRecentRequestsTableModel()->data(firstIndex.child(firstIndex.row(), column), Qt::EditRole).toString());
}

// context menu
void ReceiveCoinsDialog::showMenu(const QPoint& point)
{
    if (!model || !model->getRecentRequestsTableModel() || !ui->recentRequestsView->selectionModel())
        return;
    QModelIndexList selection = ui->recentRequestsView->selectionModel()->selectedRows();
    if (selection.empty())
        return;
    contextMenu->exec(QCursor::pos());
}

// context menu action: copy label
void ReceiveCoinsDialog::copyLabel()
{
    copyColumnToClipboard(RecentRequestsTableModel::Label);
}

// context menu action: copy message
void ReceiveCoinsDialog::copyMessage()
{
    copyColumnToClipboard(RecentRequestsTableModel::Message);
}

// context menu action: copy amount
void ReceiveCoinsDialog::copyAmount()
{
    copyColumnToClipboard(RecentRequestsTableModel::Amount);
}

void ReceiveCoinsDialog::select_item(int i)
{
    dialog_Widget->show();
}
