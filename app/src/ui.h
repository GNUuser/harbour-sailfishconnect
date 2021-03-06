#ifndef UI_H
#define UI_H

#include <QObject>
#include <QString>
#include <QSettings>

class QQuickView;
class QQmlImageProviderBase;

namespace SailfishConnect {

class AppDaemon;

/**
 * @brief manages ui
 *
 * - dbus interface to ui
 * - creates the QQuickView with the main window
 */
class UI : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.harbour.SailfishConnect.UI")

    Q_PROPERTY(
            bool runInBackground
            READ runInBackground
            WRITE setRunInBackground
            NOTIFY runInBackgroundChanged)

public:
    static const QString DBUS_INTERFACE_NAME;
    static const QString DBUS_PATH;

    explicit UI(AppDaemon* daemon, QObject *parent = nullptr);

    /**
     * @brief notify other main daemon to show app window
     */
    static void raise();

    bool runInBackground();
    void setRunInBackground(bool value);

    static QVariant openDevicePageDbusAction(const QString &deviceId);

signals:
    void runInBackgroundChanged();
    Q_SCRIPTABLE void openingDevicePage(const QString& deviceId);

public slots:
    /**
     * @brief show the main window
     */
    Q_SCRIPTABLE void showMainWindow();

    /**
     * @brief quit application
     */
    Q_SCRIPTABLE void quit();

    /**
     * @brief open page of device
     * @param deviceId id of device
     */
    Q_SCRIPTABLE void openDevicePage(const QString& deviceId);

private:
    QQuickView* m_view = nullptr;
    AppDaemon *m_daemon;

    QSettings m_settings;
    bool m_runInBackground = false;

    void onMainWindowDestroyed();

    bool eventFilter(QObject *obj, QEvent *event) override;
};

} // namespace SailfishConnect

#endif // UI_H
