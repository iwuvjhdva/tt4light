#include <QtGui/QApplication>
#include "mainwindow.h"

TTInstance* ttInst = NULL;

int main(int argc, char *argv[])
{

    QApplication a(argc, argv);

    ttInst = TT_InitTeamTalkPoll();

    MainWindow w;
    w.show();

    int result = a.exec();
    TT_CloseTeamTalk(ttInst);

    return result;
}
