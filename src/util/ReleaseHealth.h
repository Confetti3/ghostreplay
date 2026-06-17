#pragma once

#include "util/Config.h"

#include <QVariantList>
#include <QString>

namespace ReleaseHealth
{
struct Check
{
    QString id;
    QString label;
    QString severity;
    QString message;
};

QString appVersion();
QString locateFfmpegExecutable();
QVariantList toVariantList(const QList<Check>& checks);
QList<Check> validateConfig(Config& config);
QList<Check> collectStartupChecks(Config& config);
QString diagnosticsText(const Config& config, const QList<Check>& checks, bool captureAvailable, const QString& captureError);
}
