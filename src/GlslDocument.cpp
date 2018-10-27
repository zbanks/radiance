#include "GlslDocument.h"
#include "Paths.h"
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QDir>

QQuickTextDocument *GlslDocument::document() {
    return m_document;
}

void GlslDocument::setDocument(QQuickTextDocument *document) {
    if (document != m_document) {
        if (m_document != nullptr) {
            disconnect(m_document->textDocument(), NULL, this, NULL);
        }
        m_document = document;
        if (m_document != nullptr) {
            connect(m_document->textDocument(), &QTextDocument::modificationChanged, this, &GlslDocument::modifiedChanged);
            connect(m_document->textDocument(), &QTextDocument::contentsChanged, this, &GlslDocument::onContentsChanged);
        }
        emit documentChanged(document);
    }
}

QString GlslDocument::message() {
    return m_message;
}

void GlslDocument::setMessage(QString message) {
    if (message != m_message) {
        m_message = message;
        emit messageChanged(message);
    }
}

bool GlslDocument::load(QString filename) {
    if (m_document == nullptr || m_document->textDocument() == nullptr) {
        qWarning() << "Cannot call load() with no document set";
        return false;
    }
    if (filename.isEmpty()) {
        qWarning() << "Cannot call load() with no filename set";
        return false;
    }
    filename = Paths::expandLibraryPath(filename);

    QFile f(filename);
    if (!f.open(QFile::ReadOnly | QFile::Text)) {
        setMessage(QString("Could not open \"%1\" for reading").arg(filename));
        return false;
    }
    QTextStream in(&f);
    auto doc = m_document->textDocument();
    doc->setPlainText(in.readAll());
    doc->setModified(false);
    setMessage(QString("Loaded \"%1\"").arg(filename));
    return true;
}

bool GlslDocument::save(QString filename) {
    if (m_document == nullptr || m_document->textDocument() == nullptr) {
        qWarning() << "Cannot call save() with no document set";
        return false;
    }

    if (filename.isEmpty()) {
        qWarning() << "Cannot call save() with no filename set";
        return false;
    }
    filename = Paths::ensureUserLibrary(filename);

    QFile f(filename);
    if (!f.open(QFile::WriteOnly | QFile::Text)) {
        setMessage(QString("Could not open \"%1\" for writing").arg(filename));
        return false;
    }
    QTextStream out(&f);
    auto doc = m_document->textDocument();
    out << doc->toPlainText();
    doc->setModified(false);
    setMessage(QString("Saved \"%1\"").arg(filename));
    return true;
}

void GlslDocument::clear() {
    if (m_document == nullptr || m_document->textDocument() == nullptr) {
        qWarning() << "Cannot call clear() with no document set";
        return;
    }
    auto doc = m_document->textDocument();
    doc->clear();
    doc->setModified(false);
    setMessage("");
}

bool GlslDocument::revert(QString filename) {
    if (m_document == nullptr || m_document->textDocument() == nullptr) {
        qWarning() << "Cannot call revert() with no document set";
        return false;
    }
    if (filename.isEmpty()) {
        qWarning() << "Cannot call revert() with no filename set";
        return false;
    }

    if (QFileInfo(filename).isAbsolute()) {
        setMessage(QString("\"%1\" is not a library path").arg(filename));
        return false;
    }
    auto systemPath = QDir::cleanPath(Paths::systemLibrary() + "/" + filename);
    if (!QFileInfo(systemPath).exists()) {
        setMessage(QString("\"%1\" not found in system library, nothing to revert to!").arg(filename));
        return false;
    }
    auto userPath = QDir::cleanPath(Paths::userLibrary() + "/" + filename);
    if (!QFileInfo(userPath).exists()) {
        setMessage(QString("\"%1\" not found in user library, no changes to revert!").arg(filename));
        return false;
    }
    if (QFile(userPath).remove()) {
        if (load(filename)) {
            setMessage(QString("Reverted to system version of \"%1\"").arg(filename));
            return true;
        }
        return false;
    }
    setMessage(QString("Could not remove \"%1\"").arg(userPath));
    return true;
}

bool GlslDocument::modified() {
    if (m_document == nullptr || m_document->textDocument() == nullptr) return false;
    return m_document->textDocument()->isModified();
}

void GlslDocument::onContentsChanged() {
    setMessage("");
}

QString GlslDocument::loadDirectory(QString filename) {
    if (filename.isEmpty()) {
        return Paths::userLibrary();
    }
    auto p = Paths::expandLibraryPath(filename);
    return QFileInfo(p).dir().absolutePath();
}

QString GlslDocument::saveDirectory(QString filename) {
    if (filename.isEmpty()) {
        return Paths::userLibrary();
    }
    auto p = Paths::ensureUserLibrary(filename);
    return QFileInfo(p).dir().absolutePath();
}

QString GlslDocument::contractLibraryPath(QString filename) {
    auto p = Paths::contractLibraryPath(filename);
    return p;
}
