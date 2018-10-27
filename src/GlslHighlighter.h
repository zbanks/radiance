#pragma once
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>
#include <QQuickTextDocument>

class GlslHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT
    Q_PROPERTY(QQuickTextDocument *document READ qmlDocument WRITE setQmlDocument NOTIFY qmlDocumentChanged)

public:
    GlslHighlighter(QTextDocument *parent = nullptr);

public slots:
    QQuickTextDocument *qmlDocument();
    void setQmlDocument(QQuickTextDocument *document);

signals:
    void qmlDocumentChanged(QQuickTextDocument *document);

protected:
    QQuickTextDocument *m_document{};
    void highlightBlock(const QString &text) override;

private:
    struct HighlightingRule
    {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QVector<HighlightingRule> highlightingRules;

    QTextCharFormat multiLineCommentFormat;
    QRegularExpression commentStartExpression;
    QRegularExpression commentEndExpression;
};
