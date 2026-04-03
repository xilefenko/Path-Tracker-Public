#pragma once
#include <QString>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QCoreApplication>

// Copies SourceFilePath into DestinationFolder (relative to applicationDirPath).
// Returns relative path (e.g. "icons/foo.png") or empty string on failure.
inline QString ImportPicture(const QString& SourceFilePath, const QString& DestinationFolder)
{
    if (SourceFilePath.isEmpty()) return {};

    const QString AppDir = QCoreApplication::applicationDirPath();
    QDir dir(AppDir);
    if (!dir.mkpath(DestinationFolder)) return {};

    const QString FileName = QFileInfo(SourceFilePath).fileName();
    const QString DestPath = AppDir + "/" + DestinationFolder + "/" + FileName;

    if (QFileInfo(SourceFilePath).absoluteFilePath() == QFileInfo(DestPath).absoluteFilePath())
        return DestinationFolder + "/" + FileName;

    if (QFile::exists(DestPath))
        QFile::remove(DestPath);

    if (!QFile::copy(SourceFilePath, DestPath)) return {};
    return DestinationFolder + "/" + FileName;
}

// Resolves a stored relative path to an absolute path for display.
inline QString ResolvePicturePath(const QString& RelativePath)
{
    if (RelativePath.isEmpty()) return {};
    return QCoreApplication::applicationDirPath() + "/" + RelativePath;
}
