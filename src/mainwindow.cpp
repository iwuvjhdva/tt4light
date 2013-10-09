#include <QDebug>


#include "mainwindow.h"
#include "ui_mainwindow.h"


extern TTInstance* ttInst;


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    statusMode = STATUSMODE_AVAILABLE;
    userID = 0;

    if (QApplication::arguments().length() > 1)
        settings = new QSettings(QApplication::arguments()[1], QSettings::IniFormat);
    else
        settings = new QSettings(QSettings::IniFormat, QSettings::UserScope, "Bnei Baruch", "tt4light", 0);

    settings->setIniCodec("UTF-8");

    videoWidget = new VideoWidget(this, settings);
    this->centralWidget()->layout()->addWidget(videoWidget);

    if(ttInst != NULL)
        qDebug() << "Video instance initialized successfully";
    else
        qCritical() << "Video instance initialization failed";

    connectServer();

    timers.insert(startTimer(1000), TIMER_ONE_SECOND);
    timers.insert(startTimer(50), TIMER_UI_UPDATE);

    this->centralWidget()->layout()->setContentsMargins(0, 0, 0, 0);
    videoWidget->setFixedSize(settings->value("video/width", 640).toInt(),
                       settings->value("video/height", 480).toInt());
    this->setFixedSize(this->sizeHint());

    qDebug() << "Window ID: " << this->videoWidget->winId();
    startBroadcast();
}

void MainWindow::killLocalTimer(TimerEvent e)
{
    timers_t::iterator ite = timers.begin();
    while(ite != timers.end())
    {
        if(*ite == e)
        {
            killTimer(ite.key());
            timers.erase(ite);
            break;
        }
        ite++;
    }
}

void MainWindow::timerEvent(QTimerEvent *event)
{
    QMainWindow::timerEvent(event);

    timers_t::iterator ite = timers.find(event->timerId());

    switch(*ite)
    {
        case TIMER_ONE_SECOND:
            break;
        case TIMER_UI_UPDATE:
            {
                TTMessage message;
                int msecs = 0;
                while(TT_GetMessage(ttInst, &message, &msecs))
                    processTTMessage(message);
            }
            break;
        case TIMER_RECONNECT:
            Q_ASSERT( (TT_GetFlags(ttInst) & CLIENT_CONNECTED) == 0);
            disconnectServer();
            connectServer();
            break;
    }
}

void MainWindow::connectServer()
{
    if(TT_GetFlags(ttInst) & CLIENT_CONNECTION)
        return;

    qDebug() << "Connecting to server...";

    if(!settings->allKeys().count())
    {
        qCritical() << "No settings available, aborting";
        return;
    }

    TT_Connect(ttInst,
            _W(settings->value("server/host").toString()),
            settings->value("server/tcp_port").toInt(),
            settings->value("server/udp_port").toInt(),
            0, 0
            );
}

void MainWindow::startBroadcast()
{
    int devicesNumber = 0;
    TT_GetVideoCaptureDevices(ttInst, NULL, &devicesNumber);
    qDebug() << "Devices number: " << devicesNumber;

    QString deviceName = settings->value("video/device", "/dev/video0").toString();
    TTCHAR *deviceID = NULL;

    if (devicesNumber)
    {
        VideoCaptureDevice *videoDevices = new VideoCaptureDevice[devicesNumber];
        TT_GetVideoCaptureDevices(ttInst, videoDevices, &devicesNumber);
        for (int i = 0; i < devicesNumber; i++)
            if (_Q(videoDevices[i].szDeviceID).startsWith(deviceName))
                deviceID = videoDevices[i].szDeviceID;
    }

    if (!deviceID)
        qDebug() << QString("Device \"%1\" not found").arg(deviceName);
    else if (settings->value("video/start_broadcast", true).toBool())
    {
        qDebug() << QString("Device ID %1").arg(deviceID);

        VideoCodec codec;
        codec.nCodec = THEORA_CODEC;
        codec.theora.nQuality = settings->value("video/quality", 60).toInt();
        codec.theora.nTargetBitRate = 0;

        CaptureFormat format;
        format.nWidth = settings->value("user/width", 320).toInt();
        format.nHeight = settings->value("user/height", 240).toInt();
        format.nFPS_Numerator = settings->value("user/fps", 1).toInt();
        format.nFPS_Denominator = 1;
        format.picFourCC = FOURCC_RGB32;

        if(TT_InitVideoCaptureDevice(ttInst, deviceID, &format, &codec))
        {
            statusMode |= STATUSMODE_VIDEOTX;
            TT_DoChangeStatus(ttInst, statusMode, _W(QString("Broadcast started")));

            TT_EnableTransmission(ttInst, TRANSMIT_VIDEO, true);
            qDebug() << "Broadcast started";
        } else {
            qDebug() << "Error starting broadcast";
        }
    }
}

void MainWindow::commandProcessing(int commandID, bool complete)
{
    cmdreply_t::iterator ite;

    if(complete && (ite = commands.find(commandID)) != commands.end())
    {
        switch(*ite)
        {
            case CMD_COMPLETE_LOGIN:
                {
                    TT_DoChangeStatus(ttInst, statusMode, _W(QString("groupcam")));

                    QString path = settings->value("server/channel_path").toString();
                    int channelID = TT_GetChannelIDFromPath(ttInst, _W(path));

                    if(channelID)
                    {
                        int commandID = TT_DoJoinChannelByID(ttInst, channelID, _W(settings->value("server/channel_password").toString()));

                        if(commandID>0)
                            commands.insert(commandID, CMD_COMPLETE_JOINCHANNEL);

                        qDebug() << QString("Joining the channel %1...").arg(path);
                    } else {
                        qDebug() << "Invalid channel path";
                    }
                }
                break;
            case CMD_COMPLETE_JOINCHANNEL:
                qDebug() <<"Joined the channel";
                break;
        }
    }
}

void MainWindow::processTTMessage(const TTMessage& msg)
{
    switch(msg.wmMsg)
    {
        case WM_TEAMTALK_CON_SUCCESS:
            {
                // disable reconnect timer
                killLocalTimer(TIMER_RECONNECT);

                int commandID = TT_DoLogin(ttInst,
                        _W(settings->value("server/nickname").toString()),
                        _W(settings->value("server/server_password").toString()),
                        _W(settings->value("server/user_name").toString()),
                        _W(settings->value("server/password").toString())
                        );
                if(commandID>0)
                    commands.insert(commandID, CMD_COMPLETE_LOGIN);

                qDebug() << "Connected to server";
            }
            break;
        case WM_TEAMTALK_CON_FAILED:
            disconnectServer();
            qDebug() << "Failed to connect to server";
            break;
        case WM_TEAMTALK_CON_LOST:
            disconnectServer();
            timers[startTimer(5000)] = TIMER_RECONNECT;
            qDebug() << "Connection to server lost, reconnecting...";
            break;
        case WM_TEAMTALK_CMD_MYSELF_LOGGEDIN:
            this->userID = msg.wParam;
            qDebug() << "Logged in to server";
            break;
        case WM_TEAMTALK_CMD_MYSELF_LOGGEDOUT:
            qDebug() << "Logged out from server";
            disconnectServer();
            break;
        case WM_TEAMTALK_USER_VIDEOFRAME:
            if (msg.wParam != this->userID)
                videoWidget->getUserFrame(msg.wParam, msg.lParam);
            break;
        case WM_TEAMTALK_CMD_PROCESSING:
            commandProcessing(msg.wParam, msg.lParam);
            break;
        case WM_TEAMTALK_CMD_ERROR:
            qDebug() << QString("Error performing the command (error code %1").arg(msg.wParam);
            disconnectServer();
            break;
        case WM_TEAMTALK_CMD_USER_LOGGEDOUT:
        case WM_TEAMTALK_CMD_USER_LEFT:
            this->videoWidget->removeUser(msg.wParam);
            break;
        default:
            break;
    }
}

void MainWindow::disconnectServer()
{
    TT_Disconnect(ttInst);
}


MainWindow::~MainWindow()
{
    delete videoWidget;
    delete layout;
    delete ui;
    delete settings;
}
