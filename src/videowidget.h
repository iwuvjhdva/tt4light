#ifndef VIDEOWIDGET_H
#define VIDEOWIDGET_H

#include <QGLWidget>
#include <QHash>
#include <QSettings>

#include "userwidget.h"

class VideoWidget : public QGLWidget
{
    Q_OBJECT

public:
    VideoWidget(QWidget *parent, QSettings *settings);
    ~VideoWidget();

    void removeUser(int userID);
    void getUserFrame(int userID, int framesCount);

private:
    QSettings *settings;
    QImage titleImage;
    GLuint titleTexture;
    QRect titleRect;
    QRegExp *userNameRegExp;

    QHash <int, UserWidget*> userWidgets;

private:
    void drawQuad(QImage &image, GLuint texture, QRect &rect);
    void initTitle();
    void updateUsers();
    void drawUsers();
    void drawTitle();

protected:
    void initializeGL();
    void paintGL();
    void resizeGL(int width, int height);

signals:
    void debug(QString messageText);
};

#endif // VIDEOWIDGET_H
