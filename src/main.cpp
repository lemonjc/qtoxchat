#include <curl/curl.h>

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

#include <iostream>

namespace
{
QString buildCurlSummary()
{
    const curl_version_info_data* info = curl_version_info(CURLVERSION_NOW);
    const QString version = info != nullptr ? QString::fromUtf8(info->version) : QStringLiteral("unknown");

    CURL* handle = curl_easy_init();
    const bool initOk = handle != nullptr;
    if (handle != nullptr) {
        curl_easy_cleanup(handle);
    }

    return QStringLiteral("libcurl %1 | curl_easy_init: %2")
        .arg(version, initOk ? QStringLiteral("ok") : QStringLiteral("failed"));
}
}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("qtoxchat"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.1.0"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Minimal Qt and libcurl example."));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption selfTestOption(
        QStringList{QStringLiteral("self-test")},
        QStringLiteral("Run a non-interactive Qt and libcurl self-test.")
    );
    parser.addOption(selfTestOption);
    parser.process(app);

    if (parser.isSet(selfTestOption)) {
        const QString qtSummary = QStringLiteral("Qt %1").arg(QString::fromUtf8(qVersion()));
        const QString curlSummary = buildCurlSummary();

        std::cout << qtSummary.toStdString() << '\n'
                  << curlSummary.toStdString() << std::endl;
        return 0;
    }

    QWidget window;
    window.setWindowTitle(QStringLiteral("qtoxchat"));

    auto* layout = new QVBoxLayout(&window);
    auto* introLabel = new QLabel(QStringLiteral("Minimal Qt Widgets + libcurl example"), &window);
    auto* qtLabel = new QLabel(
        QStringLiteral("Qt runtime: %1").arg(QString::fromUtf8(qVersion())),
        &window
    );
    auto* curlLabel = new QLabel(buildCurlSummary(), &window);
    auto* refreshButton = new QPushButton(QStringLiteral("Probe libcurl again"), &window);

    introLabel->setWordWrap(true);
    qtLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    curlLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    layout->addWidget(introLabel);
    layout->addWidget(qtLabel);
    layout->addWidget(curlLabel);
    layout->addWidget(refreshButton);

    QObject::connect(refreshButton, &QPushButton::clicked, [&]() {
        curlLabel->setText(buildCurlSummary());
    });

    window.resize(460, 180);
    window.show();

    return app.exec();
}
