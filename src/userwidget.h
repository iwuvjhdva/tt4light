#ifndef USERWIDGET_H
#define USERWIDGET_H

#include <QDateTime>
#include <QGLWidget>

class UserWidget
{
public:
    int userID;
    QString name;
    QImage image;
    GLuint texture;
    QRect rect;
    QDateTime updated;

public:
    UserWidget(int id);

    bool update();
};

#endif // USERWIDGET_H
