#include <QCoreApplication>
#include <QPair>
#include <QString>
#include <QMap>
#include <QSet>

#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QScopedPointer>
#include <QSettings>
#include <QUuid>
#include <QDebug>
#include <QDir>
#include <QPair>

#include <QtConcurrent/QtConcurrent>
#include <QFutureInterface>
#include <QFuture>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <QtHttpServer>
#include <QHostAddress>

#include <QCryptographicHash>
#include <QRegularExpression>

#include <optional>
#include <algorithm>

static const QString rapidApiKey {"58b56e00b4576935e6edf6b6cf4cc3ff6be3e5b1b8b3bc9396077d04de997f58c1ebf3a7c709a951aa8fd8afc06c731b9598691789750635d0f25fdb3faf2808"};

static const QString REGEX_BINARY {"^[0-1]{1,}$"};

qint64 greyCode(const qint64 &binaryNumber)
{
    if (binaryNumber == 0)
        return 0;

    const qint64 last {binaryNumber % 10};
    const qint64 second_last {(binaryNumber / 10) % 10};

    if ((last && !second_last) || (!last && second_last))
        return (1 + 10 * greyCode(binaryNumber / 10));

    return (10 * greyCode(binaryNumber / 10 ));
}

int main(int argc, char *argv[])
{
    QCoreApplication app {argc, argv};

    const quint16 PORT {50004};
    const QScopedPointer<QHttpServer> httpServer {new QHttpServer {&app}};

    httpServer->route("/ping", QHttpServerRequest::Method::Get,
    [](const QHttpServerRequest &request) -> QFuture<QHttpServerResponse>
    {
        qDebug() << "Ping verarbeitet";

#ifdef QT_DEBUG
        Q_UNUSED(request)
#else
        const bool requestIsFromRapidAPI = [](const QHttpServerRequest &request) -> bool
        {
            for (const QPair<QByteArray, QByteArray> &header : request.headers())
                if (header.first == "X-RapidAPI-Proxy-Secret" && QCryptographicHash::hash(header.second, QCryptographicHash::Sha512).toHex() == rapidApiKey)
                    return true;

            return false;

        }(request);

        if (!requestIsFromRapidAPI)
            return QtConcurrent::run([]()
            {
                return QHttpServerResponse
                {
                    QJsonObject
                    {
                        {"Message", "HTTP-Requests allowed only via RapidAPI-Gateway."}
                    }
                };
            });
#endif
        return QtConcurrent::run([]()
        {
            return QHttpServerResponse
            {
                QJsonObject
                {
                    {"Message", "pong"}
                }
            };
        });
    });

    httpServer->route("/calculate", QHttpServerRequest::Method::Get     |
                                   QHttpServerRequest::Method::Put     |
                                   QHttpServerRequest::Method::Head    |
                                   QHttpServerRequest::Method::Trace   |
                                   QHttpServerRequest::Method::Patch   |
                                   QHttpServerRequest::Method::Delete  |
                                   QHttpServerRequest::Method::Options |
                                   QHttpServerRequest::Method::Connect |
                                   QHttpServerRequest::Method::Unknown,
    [](const QHttpServerRequest &request) -> QFuture<QHttpServerResponse>
    {
#ifdef QT_DEBUG
        Q_UNUSED(request)
#else
       const bool requestIsFromRapidAPI = [](const QHttpServerRequest &request) -> bool
       {
           for (const QPair<QByteArray, QByteArray> &header : request.headers())
           {
               if (header.first == "X-RapidAPI-Proxy-Secret" && QCryptographicHash::hash(header.second, QCryptographicHash::Sha512).toHex() == rapidApiKey)
                   return true;
           }

           return false;

       }(request);

       if (!requestIsFromRapidAPI)
           return QtConcurrent::run([]()
           {
               return QHttpServerResponse
               {
                   QJsonObject
                   {
                       {"Message", "HTTP-Requests allowed only via RapidAPI-Gateway."}
                   }
               };
           });
#endif
       return QtConcurrent::run([]()
       {
           return QHttpServerResponse
           {
               QJsonObject
               {
                   {"Message", "The used HTTP-Method is not implemented."}
               }
           };
       });
    });

    httpServer->route("/calculate", QHttpServerRequest::Method::Post,
    [](const QHttpServerRequest &request) -> QFuture<QHttpServerResponse>
    {
        qDebug() << "Anfrage von IP: " << request.remoteAddress().toString();

#ifdef QT_DEBUG
        Q_UNUSED(request)
#else
        const bool requestIsFromRapidAPI = [](const QHttpServerRequest &request) -> bool
        {
            for (const QPair<QByteArray, QByteArray> &header : request.headers())
            {
                if (header.first == "X-RapidAPI-Proxy-Secret" && QCryptographicHash::hash(header.second, QCryptographicHash::Sha512).toHex() == rapidApiKey)
                    return true;
            }

            return false;

        }(request);

        if (!requestIsFromRapidAPI)
            return QtConcurrent::run([]()
            {
                return QHttpServerResponse
                {
                    QJsonObject
                    {
                        {"Message", "HTTP-Requests allowed only via RapidAPI-Gateway."}
                    }
                };
            });
#endif
        if (request.body().isEmpty())
            return QtConcurrent::run([]()
            {
                return QHttpServerResponse
                {
                    QJsonObject
                    {
                        {"Message", "HTTP-Request body is empty."}
                    }
                };
            });

        const QJsonDocument jsonDocument {QJsonDocument::fromJson(request.body())};

        if (jsonDocument.isNull())
            return QtConcurrent::run([]()
            {
                return QHttpServerResponse
                {
                    QJsonObject
                    {
                        {"Message", "Invalid data sent. Please send a valid JSON-Object."}
                    }
                };
            });

        const QJsonObject jsonObject {jsonDocument.object()};

        if (jsonObject.isEmpty())
            return QtConcurrent::run([]()
            {
                return QHttpServerResponse
                {
                    QJsonObject
                    {
                        {"Message", "Invalid data sent. Please send a valid JSON-Object."}
                    }
                };
            });

        if (!jsonObject.contains("Number"))
            return QtConcurrent::run([]()
            {
                return QHttpServerResponse
                {
                    QJsonObject
                    {
                        {"Message", "Invalid data sent. Please send a valid JSON-Object."}
                    }
                };
            });

        if (QRegularExpression{REGEX_BINARY}.match(QString::number(jsonObject.value("Number").toInteger())).hasMatch())
            return QtConcurrent::run([=]()
            {
                return QHttpServerResponse
                {
                    QJsonObject
                    {
                        {"Result", greyCode(jsonObject.value("Number").toInteger())}
                    }
                };
            });

        return QtConcurrent::run([=]()
        {
            return QHttpServerResponse
            {
                QJsonObject
                {
                    {"Message", "The submitted value is not a valid binary number. Please submit a number in binary format."}
                }
            };
        });
    });

    if (httpServer->listen(QHostAddress::Any, static_cast<quint16>(PORT)) == 0)
        return -1;

    return app.exec();
}
