#pragma once

#include <QMainWindow>

class QListWidget;
class QListWidgetItem;
class QLabel;
class QTextBrowser;
class QLineEdit;
class QPushButton;
class QCloseEvent;
class QDockWidget;
class QTabWidget;

class ChatInputEdit;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() = default;

private:
    void SetupUi_();

private:
    // ================= UI Elements =================

    // top widgets
    QLabel *account_name_label_{nullptr};
    QLineEdit *self_id_edit_{nullptr};
    QPushButton *status_btn_{nullptr};

    QPushButton *theme_toggle_btn_{nullptr};
    QPushButton *notice_btn_{nullptr};

    // friend list
    QListWidget *friend_list_{nullptr};
    QPushButton *add_friend_btn_{nullptr};
    QPushButton *remove_friend_btn_{nullptr};
    QPushButton *edit_nickname_btn_{nullptr};

    // group list
    QListWidget *group_list_{nullptr};
    QPushButton *create_group_btn_{nullptr};
    QPushButton *invite_group_btn_{nullptr};
    QPushButton *leave_group_btn_{nullptr};

    QTabWidget *contact_tab_widget_{nullptr};

    // chat area
    QTextBrowser *chat_view_{nullptr};
    ChatInputEdit *chat_input_{nullptr};
    QPushButton *send_btn_{nullptr};
    QPushButton *send_file_btn_{nullptr};
    QPushButton *call_btn_{nullptr};
    QPushButton *video_call_btn_{nullptr};

    // notice area
    QDockWidget *notice_dock_{nullptr};
    QTabWidget *notice_tabs_{nullptr};
    QTextBrowser *notice_log_view_{nullptr};
    QTextBrowser *notice_config_view_{nullptr};
};