#include "compareEngine.h"

CompareEngine::CompareEngine()
{

}

void CompareEngine::setPath(const QString _path)
{
    path = _path;
}

void CompareEngine::log(const QString text)
{
    qDebug() << text;
    emit currentAction(text);
}

void CompareEngine::clear()
{
    path.clear();
    filesById.clear();
    filesIdByHash.clear();
}

void CompareEngine::runCompare()
{
    qDebug() << "Engine thread: " << QThread::currentThreadId();
    startCompare();
    emit finishedCompare();
}

void CompareEngine::runFullCompare()
{
    startFullCompare();
    emit finishedFull();
}

void CompareEngine::runDelete()
{

}

QByteArray CompareEngine::getHash(const QString filePath)
{
    QFile file(filePath);
    if(file.open(QFile::ReadOnly))
    {
        QCryptographicHash hash(QCryptographicHash::Sha1);
        while(!file.atEnd()){
            hash.addData(file.read(8192));
        }
        return hash.result();
    }
    return QByteArray();
}

bool CompareEngine::startCompare()
{
    if(path != "0")
    {
        listDirs(path);

        log("\n\n!!! --- RESULT --- !!!");

        for(auto i : filesIdByHash.keys())
        {
            int id = filesIdByHash[i];
            if(filesById[id].size() > 1)
            {
                log("HASH:");
                log(i.toHex());
                log("FILES:");
                for(auto j : filesById[id])
                {
                    log(j.absoluteFilePath());
                }
                log("");
            }
        }
        return true;
    }

    return false;
}

QVector<QVector<QFileInfo> > CompareEngine::getFilesList() // Пересмотрен
{
    return filesById;
}

// Побитовое сравнение ранее отобранных файлов
void CompareEngine::startFullCompare()
{
    log("Full compare duplicate files started");
    for(int i = 0; i < filesById.size(); i++)
    {
        if(filesById[i].size() > 1)
        {
            for(int j = 1; j < filesById[i].size(); j++)
            {
                // Если файлы разные, удаляем его из копий и добавляем в основной список
                if(!fullCompare(filesById[i][0], filesById[i][j]))
                {
                    log("Not equal file detected: " + filesById[i][j].filePath());
                    QVector<QFileInfo> temp;
                    temp.push_back(filesById[i][j]);
                    filesById.push_back(temp);

                    filesById[i].erase(filesById[i].begin() + j);
                    j--;
                }
            }
        }
    }
}

bool CompareEngine::fullCompare(QFileInfo fileInfo1, QFileInfo fileInfo2)
{
    log("Compare files: " + fileInfo1.absoluteFilePath() + " & " + fileInfo2.absoluteFilePath());
    QFile file1(fileInfo1.absoluteFilePath());
    QFile file2(fileInfo2.absoluteFilePath());

    if(file1.open(QIODevice::ReadOnly) && file2.open(QIODevice::ReadOnly))
    {
        const uint bufSize = 1024;
        char buf1[bufSize];
        char buf2[bufSize];

        log("Files opened!");
        while(file1.read(buf1, bufSize) && file2.read(buf2, bufSize))
        {
            for(int i = 0; i < bufSize; i++)
            {
                if(buf1[i] != buf2[i]) return false;
            }
        }
    }
    log("Files is equal");
    return true;
}

int CompareEngine::getScannedFilesNum()
{
    return scannedFilesNum;
}

int CompareEngine::getOrigNum()
{
    return filesById.size();
}

int CompareEngine::getDupNum(int origId)
{
    return filesById[origId].size();
}

QString CompareEngine::getOrigName(int id)
{
    return filesById[id][0].fileName();
}

QFileInfo CompareEngine::getOrigInfo(int id)
{
    return filesById[id][0];
}

QString CompareEngine::getDupName(int origId, int dupId)
{
    return filesById[origId][dupId].fileName();
}

QFileInfo CompareEngine::getDubFileInfo(int origId, int dupId)
{
    return filesById[origId][dupId];
}

QVector<int> CompareEngine::getIdsWithDup(int dupNum, Tolerance value)
{
    QVector<int> result;
    for(int i = 0; i < filesById.size(); i++)
    {
        switch (value) {
        case MORE:
            if(filesById[i].size() > dupNum) result.push_back(i);
            break;

        case LESS:
            if(filesById[i].size() < dupNum) result.push_back(i);
            break;

        case EXACTLY:
            if(filesById[i].size() == dupNum) result.push_back(i);
            break;
        }
    }
    return result;
}

void CompareEngine::startDeleteDuplicates()
{
    for(int i = 0; i < filesById.size(); i++)
    {
        if(filesById[i].size() > 1)
        {
            for(int j = 0; j < filesById[i].size(); j++)
            {
                QFile::remove(filesById[i][j].absoluteFilePath());
                filesById[i].erase(filesById[i].begin() + j);
                j--;
            }
        }
    }
}

int CompareEngine::getDeletedFilesNum()
{
    return deletedFilesNum;
}

void CompareEngine::listDirs(const QString _path)
{
    QStringList dirsList = QDir(_path).entryList(QDir::AllDirs | QDir::Hidden | QDir::NoDotAndDotDot);
    for(const auto &i : dirsList)
    {
        QString currentDir = _path + splitter + i;
        log("Dir: " + currentDir);
        listDirs(currentDir);
    }

    listFiles(_path);
}

void CompareEngine::listFiles(const QString _path)
{
    QStringList filesList = QDir(_path).entryList(QDir::Files | QDir::Hidden);
    for(const auto &i : filesList)
    {
        QString currentFile = _path + splitter + i;
        log("File: " + currentFile);
        addFile(currentFile);
        scannedFilesNum++;
    }
}

void CompareEngine::addFile(const QString filePath)
{
    QByteArray hash = getHash(filePath); // Получаем хеш файла
    QFileInfo fileInfo(filePath); // получаем информацию о файле

    if(filesIdByHash.contains(hash))
    {
        // Если такой хеш файла уже имеется, добавляем в ид списока файлов текущий файл
        filesById[filesIdByHash[hash]].push_back(fileInfo);
    }
    else
    {
        // Иначе добавляем новый ид в список файлов
        QVector<QFileInfo> tempFileList;
        tempFileList.push_back(fileInfo);
        filesById.push_back(tempFileList);
        filesIdByHash[hash] = filesById.size()-1;
    }
}
