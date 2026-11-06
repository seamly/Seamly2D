// @file handoff_capture_probe.inc
// @author slspencer
// @brief Captures real-pattern writer outputs in a test build after LoadPattern.
// Copyright (C) 2026 Seamly2D Project
// SPDX-License-Identifier: GPL-3.0-or-later
// License: https://www.gnu.org/licenses/gpl-3.0.html

if (loaded && cmd->IsTestModeEnabled() && !qEnvironmentVariable("HANDOFF_CAPTURE_DIRECTORY").isEmpty())
{
    preparePiecesForLayout(currentLayoutPieces());
    if (!prepareMultisizePieceLists())
    {
        qApp->exit(V_EX_DATAERR);
        return;
    }
    const QString directory = qEnvironmentVariable("HANDOFF_CAPTURE_DIRECTORY");
    if (!QDir().mkpath(directory))
    {
        qApp->exit(V_EX_IOERR);
        return;
    }
    QStringList warnings;
    const QString aligned = generateHandoffSvgDocument(&warnings);
    const QList<QPair<QString, QString>> outputs = {
        {QStringLiteral("_old.svg"), generatePiecesSvgDocument()},
        {QStringLiteral("_new.svg"), generateHandoffSvgDocument()},
        {QStringLiteral("_aligned.svg"), aligned},
        {QStringLiteral("_warnings.txt"), warnings.join(QLatin1Char('\n'))}};
    for (const auto &output : outputs)
    {
        QFile file(QDir(directory).filePath(FileName() + output.first));
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)
            || file.write(output.second.toUtf8()) < 0)
        {
            qApp->exit(V_EX_IOERR);
            return;
        }
    }
}
