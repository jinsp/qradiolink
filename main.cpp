// Written by Adrian Musceac YO8RZZ at gmail dot com, started October 2013.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#include <QCoreApplication>
#include <QThread>
#include <QDebug>
#include <QObject>
#include <QDateTime>
#include <QString>
#include "dtmfdecoder.h"
#include "databaseapi.h"
#include "serverwrapper.h"
#include "config_defines.h"
#include "controller.h"
#include "mumbleclient.h"
#include "audioop.h"
#include "dtmfcommand.h"



int main(int argc, char *argv[])
{

    QCoreApplication a(argc, argv);
    QString start_time= QDateTime::currentDateTime().toString("d/MMM/yyyy hh:mm:ss");
    qDebug() << start_time;
    DatabaseApi db;
    Settings *settings = db.get_settings();
    MumbleClient client(settings);
    client.connectToServer(settings->_voice_server_ip, settings->_voice_server_port);
    //Controller *controller = new Controller(settings, &db,&client);
    DtmfCommand *dtmfcommand = new DtmfCommand(settings, &db,&client);
    QObject::connect(&client,SIGNAL(channelReady(int)),dtmfcommand,SLOT(channelReady(int)));

    QThread *t1= new QThread;
    DtmfDecoder *decoder = new DtmfDecoder(settings);
    decoder->moveToThread(t1);

    //QObject::connect(decoder,SIGNAL(haveCall(QVector<char>*)),controller,SLOT(haveCall(QVector<char>*)));
    //QObject::connect(decoder,SIGNAL(haveCommand(QVector<char>*)),controller,SLOT(haveCommand(QVector<char>*)));
    //QObject::connect(controller,SIGNAL(readyInput()),decoder,SLOT(resetInput()));

    QObject::connect(decoder,SIGNAL(haveCall(QVector<char>*)),dtmfcommand,SLOT(haveCall(QVector<char>*)));
    QObject::connect(decoder,SIGNAL(haveCommand(QVector<char>*)),dtmfcommand,SLOT(haveCommand(QVector<char>*)));
    QObject::connect(dtmfcommand,SIGNAL(readyInput()),decoder,SLOT(resetInput()));

    QObject::connect(t1, SIGNAL(started()), decoder, SLOT(run()));
    QObject::connect(decoder, SIGNAL(finished()), t1, SLOT(quit()));
    QObject::connect(decoder, SIGNAL(finished()), decoder, SLOT(deleteLater()));
    QObject::connect(t1, SIGNAL(finished()), t1, SLOT(deleteLater()));
    t1->start(QThread::HighPriority);


    QThread *t2= new QThread;
    ServerWrapper *telnet_server_wrapper = new ServerWrapper(settings, &db);
    telnet_server_wrapper->moveToThread(t2);
    //QObject::connect(controller,SIGNAL(speak(QString)),telnet_server_wrapper,SLOT(addSpeech(QString)));
    QObject::connect(dtmfcommand,SIGNAL(speak(QString)),telnet_server_wrapper,SLOT(addSpeech(QString)));
    QObject::connect(telnet_server_wrapper,SIGNAL(pingServer()),&client,SLOT(pingServer()));
    //QObject::connect(telnet_server_wrapper,SIGNAL(joinConference(int,int,int)),controller,SLOT(joinConference(int,int,int)));
    //QObject::connect(telnet_server_wrapper,SIGNAL(leaveConference(int,int,int)),controller,SLOT(leaveConference(int,int,int)));
    QObject::connect(t2, SIGNAL(started()), telnet_server_wrapper, SLOT(run()));
    QObject::connect(telnet_server_wrapper, SIGNAL(finished()), t2, SLOT(quit()));
    QObject::connect(telnet_server_wrapper, SIGNAL(finished()), telnet_server_wrapper, SLOT(deleteLater()));
    QObject::connect(t2, SIGNAL(finished()), t2, SLOT(deleteLater()));
    t2->start();

    QThread *t3 = new QThread;
    AudioOp *audio_op = new AudioOp(settings);
    audio_op->moveToThread(t3);
    QObject::connect(audio_op,SIGNAL(audioData(short*,short)),&client,SLOT(processAudio(short*,short)));
    QObject::connect(&client,SIGNAL(pcmAudio(short*,short)),audio_op,SLOT(pcmAudio(short*,short)));
    QObject::connect(t3, SIGNAL(started()), audio_op, SLOT(run()));
    QObject::connect(audio_op, SIGNAL(finished()), t3, SLOT(quit()));
    QObject::connect(audio_op, SIGNAL(finished()), audio_op, SLOT(deleteLater()));
    QObject::connect(t3, SIGNAL(finished()), t3, SLOT(deleteLater()));
    t3->start(QThread::HighPriority);

    return a.exec();
}
