#include "GlslHighlighter.h"

// This code largely based off of the Qt syntax highlighter example
// http://doc.qt.io/qt-5/qtwidgets-richtext-syntaxhighlighter-example.html

GlslHighlighter::GlslHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    HighlightingRule rule;

    QTextCharFormat singleLineCommentFormat;
    singleLineCommentFormat.setForeground(Qt::cyan);
    rule.pattern = QRegularExpression("//[^\n]*");
    rule.format = singleLineCommentFormat;
    highlightingRules.append(rule);

    multiLineCommentFormat.setForeground(Qt::cyan);

    QTextCharFormat numberFormat;
    numberFormat.setForeground(Qt::magenta);
    rule.pattern = QRegularExpression("[\\d\\.]+");
    rule.format =  numberFormat;
    highlightingRules.append(rule);

    QTextCharFormat quotationFormat;
    quotationFormat.setForeground(Qt::magenta);
    rule.pattern = QRegularExpression("\".*\"");
    rule.format = quotationFormat;
    highlightingRules.append(rule);

    QTextCharFormat preProcessorFormat;
    preProcessorFormat.setForeground(QColor::fromRgb(100, 100, 255));
    rule.pattern = QRegularExpression("^\\s*#[A-Za-z0-9_]+\\s.*");
    rule.format = preProcessorFormat;
    highlightingRules.append(rule);

    // Keywords
    QTextCharFormat keywordFormat;
    keywordFormat.setForeground(Qt::yellow);
    QStringList keywords{
        "break", "continue", "do", "for", "while",
        "if", "else",
        "discard", "return",
    };
    foreach (const QString &keyword, keywords) {
        rule.pattern = QRegularExpression("\\b" + keyword + "\\b");
        rule.format = keywordFormat;
        highlightingRules.append(rule);
    }

    // Types
    QTextCharFormat typeFormat;
    typeFormat.setForeground(Qt::green);
    QStringList types{
        "attribute", "const", "uniform", "varying",
        "centroid",
        "in", "out", "inout",
        "float", "int", "void", "bool", "true", "false",
        "invariant",
        "mat2", "mat3", "mat4",
        "mat2x2", "mat2x3", "mat2x4",
        "mat3x2", "mat3x3", "mat3x4",
        "mat4x2", "mat4x3", "mat4x4",
        "vec2", "vec3", "vec4", "ivec2", "ivec3", "ivec4", "bvec2", "bvec3", "bvec4",
        "sampler1D", "sampler2D", "sampler3D", "samplerCube",
        "sampler1DShadow", "sampler2DShadow",
        "struct"
    };
    foreach (const QString &type, types) {
        rule.pattern = QRegularExpression("\\b" + type + "\\b");
        rule.format = typeFormat;
        highlightingRules.append(rule);
    }

    commentStartExpression = QRegularExpression("/\\*");
    commentEndExpression = QRegularExpression("\\*/");
}

void GlslHighlighter::highlightBlock(const QString &text) {
    foreach (const HighlightingRule &rule, highlightingRules) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
    setCurrentBlockState(0);

    int startIndex = 0;
    if (previousBlockState() != 1)
        startIndex = text.indexOf(commentStartExpression);

    while (startIndex >= 0) {
        QRegularExpressionMatch match = commentEndExpression.match(text, startIndex);
        int endIndex = match.capturedStart();
        int commentLength = 0;
        if (endIndex == -1) {
            setCurrentBlockState(1);
            commentLength = text.length() - startIndex;
        } else {
            commentLength = endIndex - startIndex
                            + match.capturedLength();
        }
        setFormat(startIndex, commentLength, multiLineCommentFormat);
        startIndex = text.indexOf(commentStartExpression, startIndex + commentLength);
    }
}

QQuickTextDocument *GlslHighlighter::qmlDocument() {
    return m_document;
}

void GlslHighlighter::setQmlDocument(QQuickTextDocument *document) {
    if (document != m_document) {
        m_document = document;
        setDocument(m_document->textDocument());
        emit qmlDocumentChanged(document);
    }
}

