#include <QDebug>

#include <QPainter>
#include <QLayout>
#include <qmath.h>

#include "common.h"
#include "constants.h"
#include "videowidget.h"

#include <QFile>

extern TTInstance* ttInst;

VideoWidget::VideoWidget(QWidget *parent, QSettings *settings) :
    QGLWidget(QGLFormat(QGL::SampleBuffers), parent)
{
    this->settings = settings;
    userNameRegExp = new QRegExp(settings->value("video/username_regexp", ".*scandinavia.*").toString(),
                                 Qt::CaseInsensitive);
}

VideoWidget::~VideoWidget()
{    
    QHash <int, UserWidget*>::iterator iter;

    for (iter = this->userWidgets.begin(); iter != this->userWidgets.end(); iter++)
    {
        glDeleteTextures(1, &iter.value()->texture);
    }

    qDeleteAll(this->userWidgets.values());

    glDeleteTextures(1, &titleTexture);

    delete userNameRegExp;
}

void VideoWidget::initializeGL()
{
    glClearColor(0, 0, 0, 0);

    glViewport(0, 0, width(), height());
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glOrtho(0, width(), height(), 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Initializing textures
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Title texture
    glGenTextures(1, &titleTexture);
    glBindTexture(GL_TEXTURE_2D, titleTexture);

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_TEXTURE_RECTANGLE_ARB);

    initTitle();
}

void VideoWidget::initTitle()
{
    titleRect = QRect(0, 0, width(), height() * TITLE_HEIGHT_PERCENTS / 100.0);
    titleImage = QImage(width(),
                        height() * TITLE_HEIGHT_PERCENTS / 100.0,
                        QImage::Format_RGB32);
    titleImage.fill(Qt::blue);

    QPainter painter;
    QString text = this->settings->value("video/title", "BB Scandinavia").toString();

    painter.begin(&titleImage);
    painter.setPen(Qt::white);

    QRect textRect = painter.fontMetrics().boundingRect(text);

    float factor = qMin((float)titleRect.width() / textRect.width(),
                        (float)titleRect.height() / textRect.height());

    QFont font = painter.font();
    font.setPointSizeF(font.pointSizeF() * factor);
    painter.setFont(font);

    QRect newTextRect = painter.fontMetrics().boundingRect(text);
    QRect boundingRect = QRect(titleRect.width() - (titleRect.width() + newTextRect.width()) / 2,
                         titleRect.height() - (titleRect.height() + newTextRect.height()) / 2,
                         width(), titleRect.height());
    painter.drawText(boundingRect, text);
    painter.end();
}

void VideoWidget::drawQuad(QImage &image, GLuint texture, QRect &rect)
{
    glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, image.width(), image.height(),
                 0, GL_BGRA, GL_UNSIGNED_BYTE, image.bits());
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, texture);

    glBegin(GL_QUADS);

    glTexCoord2i(0, 0);
    glVertex2f(rect.left(), rect.top());

    glTexCoord2i(image.width()-1, 0);
    glVertex2f(rect.right(), rect.top());

    glTexCoord2i(image.width()-1, image.height()-1);
    glVertex2f(rect.right(), rect.bottom());

    glTexCoord2i(0, image.height()-1);
    glVertex2f(rect.left(), rect.bottom());

    glEnd();
}

void VideoWidget::drawTitle()
{
    this->drawQuad(titleImage,
                   titleTexture,
                   titleRect);
}

bool userLessThan(UserWidget *user1, UserWidget *user2)
{
    return user1->name < user2->name;
}

void VideoWidget::updateUsers()
{
    if (this->userWidgets.isEmpty())
        return;

    int displayWidth = width();
    int padding = height() * PADDING_PERCENTS/100.;
    int displayHeight = height() - titleRect.height() - padding * 2;

    float aspectRatio = (float)width() / height();

    int pixelsTotal = displayWidth * displayHeight / this->userWidgets.count();

    int widgetWidth = qRound(qSqrt((float)pixelsTotal * aspectRatio));
    int widgetHeight = qRound(qSqrt((float)pixelsTotal / aspectRatio));

    int columnsNumber = qRound((float)displayWidth / widgetWidth);
    int rowsNumber = qRound((float)displayHeight / widgetHeight);

    if (columnsNumber * widgetWidth > displayWidth)
    {
        widgetWidth = displayWidth / columnsNumber;
        widgetHeight = widgetWidth / aspectRatio;
    }

    if (rowsNumber * widgetHeight > displayHeight)
    {
        widgetHeight = displayHeight / rowsNumber;
        widgetWidth = widgetHeight * aspectRatio;
    }

    int left = width() - (displayWidth + widgetWidth * columnsNumber)/2;
    int top = height() - (displayHeight + widgetHeight * rowsNumber)/2 - padding;

    QList <UserWidget *> widgetList = this->userWidgets.values();
    qSort(widgetList.begin(), widgetList.end(), userLessThan);

    for(int index = 0; index < widgetList.size(); ++index)
    {
        UserWidget *widget = widgetList.at(index);
        int x = left + (index % columnsNumber  * widgetWidth);
        int y = top + index / columnsNumber * widgetHeight;

        widget->rect = QRect(x, y, widgetWidth, widgetHeight);

        int seconds = widget->updated.secsTo(QDateTime::currentDateTime());
        if (seconds >= this->settings->value("video/user_timeout", 5).toInt())
            this->userWidgets.remove(widget->userID);
    }
}

void VideoWidget::drawUsers()
{
    QHash <int, UserWidget *>::iterator iter;

    for (iter = this->userWidgets.begin(); iter != this->userWidgets.end(); ++iter)
    {
        UserWidget *widget = iter.value();
        this->drawQuad(widget->image, widget->texture, widget->rect);
    }
}

void VideoWidget::removeUser(int userID)
{
    if (this->userWidgets.contains(userID))
    {
        delete this->userWidgets[userID];
        this->userWidgets.remove(userID);
        this->update();
    }
}

void VideoWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT);

    drawTitle();
    updateUsers();
    drawUsers();
}

void VideoWidget::resizeGL(int width, int height)
{
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glOrtho(0, width, height, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void VideoWidget::getUserFrame(int userID, int framesCount)
{
    Q_UNUSED(framesCount);

    if (userID == 0)
        return;

    User user;
    TT_GetUser(ttInst, userID, &user);
    QString name = _Q(user.szNickname);

    if (userNameRegExp->exactMatch(name))
    {
        UserWidget *userWidget;

        if (this->userWidgets.contains(userID))
            userWidget = userWidgets[userID];
        else {
            userWidget = new UserWidget(userID);
            this->userWidgets.insert(userID, userWidget);
        }

        userWidget->name = name;

        if (userWidget->update())
            this->update();
    }
}
