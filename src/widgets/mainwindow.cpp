#include "widgets/mainwindow.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSplitter>
#include <QTabWidget>
#include <QTextBrowser>
#include <QTextEdit>
#include <QVBoxLayout>

class ChatInputEdit : public QTextEdit {
public:
    explicit ChatInputEdit(QWidget* parent = nullptr) : QTextEdit(parent) {}

    void SetPlaceHolderText(const QString& /*text*/) {}
};

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) { SetupUi_(); }

void MainWindow::SetupUi_() {
    this->setWindowTitle(tr("qtoxchat"));
    this->resize(1000, 650);

    // clang-format off
    /*
    UI layout sketch:

    +---------------------+----------------------------------------------------------------+
    | account_name_label_ | edit | add | remove | create | invite | leave | notice | theme |
    | Tox ID              | self_id_edit_                                                  |
    | status_btn_         |                                                                |
    +---------------------+----------------------------------------------------------------+
    | contact_tab_widget_ | chat_view_                                                     |
    | Friends / Groups    |                                                                |
    |                     |                                                                |
    |                     |                                                                |
    |                     +----------------------------------------------------------------+
    |                     | chat_input_ | send | file | call | video                       |
    +---------------------+----------------------------------------------------------------+

    Layout tree:

    MainWindow
    `-- central_widget
        `-- root_layout (vertical)
            |-- top_row_layout
            |   |-- user_info_layout
            |   |   |-- account_name_layout
            |   |   |   `-- account_name_label_
            |   |   |-- tox_id_layout
            |   |   |   |-- tox_id_label
            |   |   |   `-- self_id_edit_
            |   |   `-- status_btn_
            |   |-- edit_nickname_btn_
            |   |-- add_friend_btn_
            |   |-- remove_friend_btn_
            |   |-- create_group_btn_
            |   |-- invite_group_btn_
            |   |-- leave_group_btn_
            |   |-- notice_btn_
            |   `-- theme_toggle_btn_
            `-- splitter
                |-- contact_tab_widget_
                |   |-- friend_list_
                |   `-- group_list_
                `-- chat_panel
                    `-- chat_panel_layout
                        |-- chat_view_
                        `-- input_row_layout
                            |-- chat_input_
                            |-- send_btn_
                            |-- send_file_btn_
                            |-- call_btn_
                            `-- video_call_btn_

    Setup order:

    1. Create the empty layout skeleton first. This makes the page structure
       visible before individual controls add detail.
    2. Create and configure every widget by area. At this point no widget is
       placed yet, so style and default state stay easy to review.
    3. Populate the layouts from small groups to large containers. The final
       calls attach the completed top row and splitter to the central widget.
    */
    // clang-format on

    // 1. Define the layout skeleton from outside to inside.
    //
    // QMainWindow displays one central widget. The root layout on that widget
    // is vertical: a compact header/action row on top, then the main content
    // splitter below it.
    auto* central_widget = new QWidget(this);
    auto* root_layout = new QVBoxLayout(central_widget);

    // Header area. The left side is a vertical user-info block; the right side
    // is a flat row of action buttons.
    auto* top_row_layout = new QHBoxLayout();
    auto* user_info_layout = new QVBoxLayout();
    auto* account_name_layout = new QHBoxLayout();
    auto* tox_id_layout = new QHBoxLayout();

    // Main content area. QSplitter gives the contact list and chat panel a
    // draggable divider while still keeping both widgets in one horizontal row.
    auto* splitter = new QSplitter(this);

    // Right side of the splitter. It stacks chat history over the message input
    // row, matching the visual flow of a chat window.
    auto* chat_panel = new QWidget(this);
    auto* chat_panel_layout = new QVBoxLayout(chat_panel);
    auto* input_row_layout = new QHBoxLayout();

    // 2. Create and configure widgets by UI area.
    //
    // User identity area: account name, own Tox ID, and online status. These
    // widgets will later be grouped into user_info_layout.
    account_name_label_ = new QLabel("test", this);
    account_name_label_->setStyleSheet(
        QStringLiteral("font-weight: bold; font-size: 14pt; color: #4fc3f7;"));

    auto* tox_id_label = new QLabel(tr("Tox ID: "), this);
    tox_id_label->setStyleSheet(QStringLiteral("color: #808090;"));

    self_id_edit_ = new QLineEdit(this);
    self_id_edit_->setText(
        tr("1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef"));
    self_id_edit_->setReadOnly(true);
    self_id_edit_->setFrame(false);
    self_id_edit_->setStyleSheet(
        QStringLiteral("QLineEdit {"
                       "  background: transparent; "
                       "  color: #a0a0b0; "
                       "  font-size: 9pt; "
                       "  padding: 0px; "
                       "  border: none; "
                       "}"));
    self_id_edit_->setMinimumWidth(580);

    status_btn_ = new QPushButton(this);
    status_btn_->setText(tr("Online"));
    status_btn_->setFlat(true);
    status_btn_->setStyleSheet(
        QStringLiteral("QPushButton {"
                       "  text-align: left; "
                       "  padding: 0px; "
                       "  color: #808090; "
                       "  font-size: 10pt; "
                       "  border: none; "
                       "  background: transparent; "
                       "}"
                       "QPushButton:hover {"
                       "  color: #4fc3f7; "
                       "  text-decoration: underline; "
                       "}"));
    status_btn_->setCursor(Qt::PointingHandCursor);

    // Header action buttons. They are created together because they occupy the
    // same top-row button strip and share the same parent window.
    edit_nickname_btn_ = new QPushButton(tr("Edit Nickname"), this);
    add_friend_btn_ = new QPushButton(tr("Add Friend"), this);
    remove_friend_btn_ = new QPushButton(tr("Remove Friend"), this);
    create_group_btn_ = new QPushButton(tr("Create Group"), this);
    invite_group_btn_ = new QPushButton(tr("Invite to Group"), this);
    leave_group_btn_ = new QPushButton(tr("Leave Group"), this);

    notice_btn_ = new QPushButton(tr("Notice"), this);
    notice_btn_->setToolTip(
        tr("Open Notification Center (Logs/Alerts/Configuration)"));
    notice_btn_->setFixedHeight(add_friend_btn_->sizeHint().height());

    theme_toggle_btn_ = new QPushButton(this);

    // Contact area. The two list widgets are pages inside the tab widget, which
    // becomes the left pane of the splitter.
    friend_list_ = new QListWidget(this);
    group_list_ = new QListWidget(this);
    contact_tab_widget_ = new QTabWidget(this);
    contact_tab_widget_->setMinimumWidth(240);

    // Chat area. The browser is the message history; the custom text edit and
    // buttons form the bottom input row.
    chat_view_ = new QTextBrowser(this);
    chat_view_->setOpenExternalLinks(false);

    chat_input_ = new ChatInputEdit(this);
    chat_input_->SetPlaceHolderText(
        tr("Input message, press Enter to send, Shift+Enter to line break"));

    send_btn_ = new QPushButton(tr("Send"), this);
    send_file_btn_ = new QPushButton(tr("Send File"), this);
    call_btn_ = new QPushButton(tr("Call"), this);
    video_call_btn_ = new QPushButton(tr("Video Call"), this);

    // 3. Populate layouts after every control has been configured.
    //
    // Build the small user-info rows first. Stretch keeps the text aligned on
    // the left and consumes extra horizontal space without adding dummy widgets.
    account_name_layout->setSpacing(0);
    account_name_layout->addWidget(account_name_label_);
    account_name_layout->addStretch(1);

    tox_id_layout->addWidget(tox_id_label);
    tox_id_layout->addWidget(self_id_edit_);
    tox_id_layout->addStretch(1);

    user_info_layout->setSpacing(2);
    user_info_layout->addLayout(account_name_layout);
    user_info_layout->addLayout(tox_id_layout);
    user_info_layout->addWidget(status_btn_);

    // The header row combines the user-info block with the action buttons. The
    // add order is the visible left-to-right order in the window.
    top_row_layout->addLayout(user_info_layout);
    top_row_layout->addWidget(edit_nickname_btn_);
    top_row_layout->addWidget(add_friend_btn_);
    top_row_layout->addWidget(remove_friend_btn_);
    top_row_layout->addWidget(create_group_btn_);
    top_row_layout->addWidget(invite_group_btn_);
    top_row_layout->addWidget(leave_group_btn_);
    top_row_layout->addWidget(notice_btn_);
    top_row_layout->addWidget(theme_toggle_btn_);

    // Add the contact lists as tab pages before inserting the tab widget into
    // the splitter. The tab widget owns the visual page switching behavior.
    contact_tab_widget_->addTab(friend_list_, tr("Friends"));
    contact_tab_widget_->addTab(group_list_, tr("Groups"));

    // The input row keeps the text editor wide and places all send/call actions
    // to its right, matching the chat_panel sketch above.
    input_row_layout->addWidget(chat_input_);
    input_row_layout->addWidget(send_btn_);
    input_row_layout->addWidget(send_file_btn_);
    input_row_layout->addWidget(call_btn_);
    input_row_layout->addWidget(video_call_btn_);

    // Finish the right pane: chat history gets the upper flexible area, and the
    // input row stays below it.
    chat_panel_layout->addWidget(chat_view_);
    chat_panel_layout->addLayout(input_row_layout);

    // Finish the main body. Stretch factor 1 on chat_panel lets it take the
    // extra horizontal space after the contact pane reaches its preferred size.
    splitter->addWidget(contact_tab_widget_);
    splitter->addWidget(chat_panel);
    splitter->setStretchFactor(1, 1);

    // Attach the completed page to QMainWindow only after every child layout is
    // populated, so the setup reads from structure to details to final assembly.
    root_layout->addLayout(top_row_layout);
    root_layout->addWidget(splitter, 1);

    setCentralWidget(central_widget);
}
