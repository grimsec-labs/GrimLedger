#include "markdown/SyntaxHighlighter.h"

#include <QColor>
#include <algorithm>

namespace {

QTextCharFormat makeFormat(const QColor& fg, bool bold = false, bool italic = false) {
    QTextCharFormat fmt;
    fmt.setForeground(fg);
    fmt.setFontWeight(bold ? QFont::Bold : QFont::Normal);
    fmt.setFontItalic(italic);
    return fmt;
}

QVector<SyntaxHighlighterEngine::TokenRule> baseRules(
    const QHash<QString, QColor>& colors) {
    QVector<SyntaxHighlighterEngine::TokenRule> rules;

    auto add = [&](const QString& pattern, const QString& colorKey, bool italic = false) {
        SyntaxHighlighterEngine::TokenRule r;
        r.pattern = QRegularExpression(pattern);
        r.format = makeFormat(colors.value(colorKey, QColor("#e0e0e0")), false, italic);
        rules.append(r);
    };

    add(QStringLiteral(R"((\b\d+\b))"), QStringLiteral("number"));
    add(QStringLiteral(R"((#[^\s]+))"), QStringLiteral("comment"));
    add(QStringLiteral(R"((//[^\n]*))"), QStringLiteral("comment"));
    add(QStringLiteral(R"((--[^\n]*))"), QStringLiteral("comment"));
    add(QStringLiteral(R"((/\*.*?\*/))"), QStringLiteral("comment"));
    add(QStringLiteral(R"(("(?:\\.|[^"\\])*"))"), QStringLiteral("string"));
    add(QStringLiteral(R"(('(?:\\.|[^'\\])*'))"), QStringLiteral("string"));

    return rules;
}

QHash<QString, QColor> defaultColors() {
    return {
        {QStringLiteral("keyword"), QColor("#ff4466")},
        {QStringLiteral("builtin"), QColor("#ff8844")},
        {QStringLiteral("string"), QColor("#66ff99")},
        {QStringLiteral("number"), QColor("#ffaa44")},
        {QStringLiteral("comment"), QColor("#666688")},
        {QStringLiteral("type"), QColor("#aa66ff")},
        {QStringLiteral("function"), QColor("#44ccff")},
        {QStringLiteral("variable"), QColor("#ccccdd")},
        {QStringLiteral("operator"), QColor("#ff6688")},
        {QStringLiteral("regex"), QColor("#ff99cc")},
    };
}

QString keywordPattern(const QStringList& keywords) {
    QStringList parts;
    for (const QString& kw : keywords) {
        parts.append(QRegularExpression::escape(kw));
    }
    return QStringLiteral(R"(\b(?:%1)\b)").arg(parts.join('|'));
}

void addKeywordRule(
    QVector<SyntaxHighlighterEngine::TokenRule>& rules,
    const QStringList& keywords,
    const QHash<QString, QColor>& colors,
    bool bold = true) {
    if (keywords.isEmpty()) return;
    SyntaxHighlighterEngine::TokenRule r;
    r.pattern = QRegularExpression(keywordPattern(keywords));
    r.format = makeFormat(colors.value(QStringLiteral("keyword")), bold);
    rules.append(r);
}

} // namespace

QStringList SyntaxHighlighterEngine::supportedLanguages() {
    return {
        QStringLiteral("powershell"), QStringLiteral("ps1"),
        QStringLiteral("cmd"), QStringLiteral("batch"), QStringLiteral("bat"),
        QStringLiteral("bash"), QStringLiteral("sh"), QStringLiteral("zsh"),
        QStringLiteral("shell"), QStringLiteral("kali"),
        QStringLiteral("git"),
        QStringLiteral("c"), QStringLiteral("cpp"), QStringLiteral("c++"),
        QStringLiteral("csharp"), QStringLiteral("cs"),
        QStringLiteral("java"), QStringLiteral("javascript"), QStringLiteral("js"),
        QStringLiteral("typescript"), QStringLiteral("ts"),
        QStringLiteral("python"), QStringLiteral("py"),
        QStringLiteral("html"), QStringLiteral("css"), QStringLiteral("json"),
        QStringLiteral("xml"), QStringLiteral("yaml"), QStringLiteral("yml"),
        QStringLiteral("sql"), QStringLiteral("php"), QStringLiteral("rust"),
        QStringLiteral("go"), QStringLiteral("ruby"), QStringLiteral("rb"),
        QStringLiteral("lua"), QStringLiteral("swift"), QStringLiteral("kotlin"),
        QStringLiteral("markdown"), QStringLiteral("md"),
        QStringLiteral("dockerfile"), QStringLiteral("docker"),
        QStringLiteral("regex"), QStringLiteral("regexp"),
    };
}

QString SyntaxHighlighterEngine::normalizeLanguage(const QString& lang) {
    const QString l = lang.trimmed().toLower();
    static const QHash<QString, QString> map = {
        {QStringLiteral("ps1"), QStringLiteral("powershell")},
        {QStringLiteral("bat"), QStringLiteral("cmd")},
        {QStringLiteral("batch"), QStringLiteral("cmd")},
        {QStringLiteral("sh"), QStringLiteral("bash")},
        {QStringLiteral("shell"), QStringLiteral("bash")},
        {QStringLiteral("kali"), QStringLiteral("bash")},
        {QStringLiteral("zsh"), QStringLiteral("bash")},
        {QStringLiteral("c++"), QStringLiteral("cpp")},
        {QStringLiteral("cs"), QStringLiteral("csharp")},
        {QStringLiteral("js"), QStringLiteral("javascript")},
        {QStringLiteral("ts"), QStringLiteral("typescript")},
        {QStringLiteral("py"), QStringLiteral("python")},
        {QStringLiteral("yml"), QStringLiteral("yaml")},
        {QStringLiteral("rb"), QStringLiteral("ruby")},
        {QStringLiteral("md"), QStringLiteral("markdown")},
        {QStringLiteral("docker"), QStringLiteral("dockerfile")},
        {QStringLiteral("regexp"), QStringLiteral("regex")},
    };
    return map.value(l, l);
}

QVector<SyntaxHighlighterEngine::TokenRule> SyntaxHighlighterEngine::rulesForLanguage(
    const QString& lang) {
    const QString normalized = normalizeLanguage(lang);
    const auto colors = defaultColors();
    QVector<TokenRule> rules = baseRules(colors);

    if (normalized == QStringLiteral("powershell")) {
        addKeywordRule(rules, {
            QStringLiteral("function"), QStringLiteral("if"), QStringLiteral("else"),
            QStringLiteral("foreach"), QStringLiteral("while"), QStringLiteral("return"),
            QStringLiteral("param"), QStringLiteral("switch"), QStringLiteral("try"),
            QStringLiteral("catch"), QStringLiteral("Import-Module"), QStringLiteral("Get-"),
            QStringLiteral("Set-"), QStringLiteral("New-"), QStringLiteral("Remove-"),
        }, colors);
    } else if (normalized == QStringLiteral("cmd")) {
        addKeywordRule(rules, {
            QStringLiteral("if"), QStringLiteral("else"), QStringLiteral("for"),
            QStringLiteral("goto"), QStringLiteral("echo"), QStringLiteral("set"),
            QStringLiteral("cd"), QStringLiteral("dir"), QStringLiteral("copy"),
            QStringLiteral("del"), QStringLiteral("call"), QStringLiteral("exit"),
        }, colors);
    } else if (normalized == QStringLiteral("bash")) {
        addKeywordRule(rules, {
            QStringLiteral("if"), QStringLiteral("then"), QStringLiteral("else"),
            QStringLiteral("fi"), QStringLiteral("for"), QStringLiteral("while"),
            QStringLiteral("do"), QStringLiteral("done"), QStringLiteral("echo"),
            QStringLiteral("export"), QStringLiteral("sudo"), QStringLiteral("apt"),
            QStringLiteral("chmod"), QStringLiteral("grep"), QStringLiteral("curl"),
            QStringLiteral("ssh"), QStringLiteral("nmap"), QStringLiteral("cat"),
        }, colors);
    } else if (normalized == QStringLiteral("git")) {
        addKeywordRule(rules, {
            QStringLiteral("git"), QStringLiteral("commit"), QStringLiteral("push"),
            QStringLiteral("pull"), QStringLiteral("clone"), QStringLiteral("checkout"),
            QStringLiteral("branch"), QStringLiteral("merge"), QStringLiteral("rebase"),
            QStringLiteral("stash"), QStringLiteral("log"), QStringLiteral("status"),
        }, colors);
    } else if (normalized == QStringLiteral("c")) {
        addKeywordRule(rules, {
            QStringLiteral("if"), QStringLiteral("else"), QStringLiteral("for"),
            QStringLiteral("while"), QStringLiteral("return"), QStringLiteral("struct"),
            QStringLiteral("typedef"), QStringLiteral("include"), QStringLiteral("define"),
            QStringLiteral("int"), QStringLiteral("char"), QStringLiteral("void"),
            QStringLiteral("const"), QStringLiteral("static"),
        }, colors);
    } else if (normalized == QStringLiteral("cpp")) {
        addKeywordRule(rules, {
            QStringLiteral("class"), QStringLiteral("namespace"), QStringLiteral("template"),
            QStringLiteral("public"), QStringLiteral("private"), QStringLiteral("protected"),
            QStringLiteral("virtual"), QStringLiteral("override"), QStringLiteral("const"),
            QStringLiteral("return"), QStringLiteral("if"), QStringLiteral("else"),
            QStringLiteral("for"), QStringLiteral("while"), QStringLiteral("include"),
            QStringLiteral("std"), QStringLiteral("auto"), QStringLiteral("bool"),
        }, colors);
    } else if (normalized == QStringLiteral("csharp")) {
        addKeywordRule(rules, {
            QStringLiteral("class"), QStringLiteral("namespace"), QStringLiteral("using"),
            QStringLiteral("public"), QStringLiteral("private"), QStringLiteral("protected"),
            QStringLiteral("static"), QStringLiteral("void"), QStringLiteral("return"),
            QStringLiteral("if"), QStringLiteral("else"), QStringLiteral("foreach"),
            QStringLiteral("async"), QStringLiteral("await"), QStringLiteral("var"),
        }, colors);
    } else if (normalized == QStringLiteral("java")) {
        addKeywordRule(rules, {
            QStringLiteral("class"), QStringLiteral("public"), QStringLiteral("private"),
            QStringLiteral("protected"), QStringLiteral("static"), QStringLiteral("void"),
            QStringLiteral("return"), QStringLiteral("if"), QStringLiteral("else"),
            QStringLiteral("import"), QStringLiteral("package"), QStringLiteral("new"),
        }, colors);
    } else if (normalized == QStringLiteral("javascript")) {
        addKeywordRule(rules, {
            QStringLiteral("function"), QStringLiteral("const"), QStringLiteral("let"),
            QStringLiteral("var"), QStringLiteral("return"), QStringLiteral("if"),
            QStringLiteral("else"), QStringLiteral("for"), QStringLiteral("while"),
            QStringLiteral("class"), QStringLiteral("import"), QStringLiteral("export"),
            QStringLiteral("async"), QStringLiteral("await"), QStringLiteral("true"),
            QStringLiteral("false"), QStringLiteral("null"), QStringLiteral("undefined"),
        }, colors);
    } else if (normalized == QStringLiteral("typescript")) {
        addKeywordRule(rules, {
            QStringLiteral("function"), QStringLiteral("const"), QStringLiteral("let"),
            QStringLiteral("interface"), QStringLiteral("type"), QStringLiteral("enum"),
            QStringLiteral("return"), QStringLiteral("if"), QStringLiteral("else"),
            QStringLiteral("import"), QStringLiteral("export"), QStringLiteral("async"),
            QStringLiteral("await"), QStringLiteral("readonly"),
        }, colors);
    } else if (normalized == QStringLiteral("python")) {
        addKeywordRule(rules, {
            QStringLiteral("def"), QStringLiteral("class"), QStringLiteral("import"),
            QStringLiteral("from"), QStringLiteral("return"), QStringLiteral("if"),
            QStringLiteral("elif"), QStringLiteral("else"), QStringLiteral("for"),
            QStringLiteral("while"), QStringLiteral("with"), QStringLiteral("as"),
            QStringLiteral("True"), QStringLiteral("False"), QStringLiteral("None"),
            QStringLiteral("lambda"), QStringLiteral("pass"), QStringLiteral("raise"),
        }, colors);
    } else if (normalized == QStringLiteral("html")) {
        addKeywordRule(rules, {
            QStringLiteral("html"), QStringLiteral("head"), QStringLiteral("body"),
            QStringLiteral("div"), QStringLiteral("span"), QStringLiteral("script"),
            QStringLiteral("style"), QStringLiteral("meta"), QStringLiteral("link"),
        }, colors);
    } else if (normalized == QStringLiteral("css")) {
        addKeywordRule(rules, {
            QStringLiteral("color"), QStringLiteral("background"), QStringLiteral("margin"),
            QStringLiteral("padding"), QStringLiteral("display"), QStringLiteral("flex"),
            QStringLiteral("important"), QStringLiteral("font"), QStringLiteral("border"),
        }, colors);
    } else if (normalized == QStringLiteral("json")) {
        // strings/numbers handled by base rules
    } else if (normalized == QStringLiteral("xml")) {
        SyntaxHighlighterEngine::TokenRule tag;
        tag.pattern = QRegularExpression(QStringLiteral(R"(</?[\w:-]+)"));
        tag.format = makeFormat(colors.value(QStringLiteral("keyword")));
        rules.append(tag);
    } else if (normalized == QStringLiteral("yaml")) {
        addKeywordRule(rules, {
            QStringLiteral("true"), QStringLiteral("false"), QStringLiteral("null"),
        }, colors);
    } else if (normalized == QStringLiteral("sql")) {
        addKeywordRule(rules, {
            QStringLiteral("SELECT"), QStringLiteral("FROM"), QStringLiteral("WHERE"),
            QStringLiteral("INSERT"), QStringLiteral("UPDATE"), QStringLiteral("DELETE"),
            QStringLiteral("JOIN"), QStringLiteral("CREATE"), QStringLiteral("TABLE"),
            QStringLiteral("INDEX"), QStringLiteral("ORDER"), QStringLiteral("BY"),
        }, colors);
    } else if (normalized == QStringLiteral("php")) {
        addKeywordRule(rules, {
            QStringLiteral("function"), QStringLiteral("class"), QStringLiteral("public"),
            QStringLiteral("private"), QStringLiteral("return"), QStringLiteral("if"),
            QStringLiteral("else"), QStringLiteral("echo"), QStringLiteral("namespace"),
        }, colors);
    } else if (normalized == QStringLiteral("rust")) {
        addKeywordRule(rules, {
            QStringLiteral("fn"), QStringLiteral("let"), QStringLiteral("mut"),
            QStringLiteral("pub"), QStringLiteral("struct"), QStringLiteral("enum"),
            QStringLiteral("impl"), QStringLiteral("trait"), QStringLiteral("use"),
            QStringLiteral("match"), QStringLiteral("if"), QStringLiteral("else"),
            QStringLiteral("return"), QStringLiteral("async"), QStringLiteral("await"),
        }, colors);
    } else if (normalized == QStringLiteral("go")) {
        addKeywordRule(rules, {
            QStringLiteral("func"), QStringLiteral("package"), QStringLiteral("import"),
            QStringLiteral("var"), QStringLiteral("const"), QStringLiteral("type"),
            QStringLiteral("struct"), QStringLiteral("interface"), QStringLiteral("return"),
            QStringLiteral("if"), QStringLiteral("else"), QStringLiteral("for"),
            QStringLiteral("range"), QStringLiteral("go"), QStringLiteral("defer"),
        }, colors);
    } else if (normalized == QStringLiteral("ruby")) {
        addKeywordRule(rules, {
            QStringLiteral("def"), QStringLiteral("class"), QStringLiteral("module"),
            QStringLiteral("end"), QStringLiteral("if"), QStringLiteral("else"),
            QStringLiteral("elsif"), QStringLiteral("return"), QStringLiteral("require"),
        }, colors);
    } else if (normalized == QStringLiteral("lua")) {
        addKeywordRule(rules, {
            QStringLiteral("function"), QStringLiteral("local"), QStringLiteral("return"),
            QStringLiteral("if"), QStringLiteral("then"), QStringLiteral("else"),
            QStringLiteral("end"), QStringLiteral("for"), QStringLiteral("while"),
        }, colors);
    } else if (normalized == QStringLiteral("swift")) {
        addKeywordRule(rules, {
            QStringLiteral("func"), QStringLiteral("class"), QStringLiteral("struct"),
            QStringLiteral("enum"), QStringLiteral("let"), QStringLiteral("var"),
            QStringLiteral("return"), QStringLiteral("if"), QStringLiteral("else"),
            QStringLiteral("import"), QStringLiteral("guard"),
        }, colors);
    } else if (normalized == QStringLiteral("kotlin")) {
        addKeywordRule(rules, {
            QStringLiteral("fun"), QStringLiteral("class"), QStringLiteral("val"),
            QStringLiteral("var"), QStringLiteral("return"), QStringLiteral("if"),
            QStringLiteral("else"), QStringLiteral("when"), QStringLiteral("import"),
            QStringLiteral("package"), QStringLiteral("data"),
        }, colors);
    } else if (normalized == QStringLiteral("dockerfile")) {
        addKeywordRule(rules, {
            QStringLiteral("FROM"), QStringLiteral("RUN"), QStringLiteral("CMD"),
            QStringLiteral("LABEL"), QStringLiteral("EXPOSE"), QStringLiteral("ENV"),
            QStringLiteral("COPY"), QStringLiteral("ADD"), QStringLiteral("WORKDIR"),
            QStringLiteral("ENTRYPOINT"), QStringLiteral("VOLUME"), QStringLiteral("USER"),
        }, colors);
    } else if (normalized == QStringLiteral("regex")) {
        SyntaxHighlighterEngine::TokenRule r;
        r.pattern = QRegularExpression(QStringLiteral(R"((\\[dDsSwWw.+*?^$|()\[\]{}])"));
        r.format = makeFormat(colors.value(QStringLiteral("regex")));
        rules.append(r);
    }

    return rules;
}

QString SyntaxHighlighterEngine::highlightToHtml(const QString& code, const QString& lang) {
    const auto rules = rulesForLanguage(lang);
    struct Span { int start; int length; QString color; };
    QVector<Span> spans;

    for (const auto& rule : rules) {
        auto it = rule.pattern.globalMatch(code);
        while (it.hasNext()) {
            const auto m = it.next();
            Span s;
            s.start = m.capturedStart();
            s.length = m.capturedLength();
            s.color = rule.format.foreground().color().name();
            spans.append(s);
        }
    }

    std::sort(spans.begin(), spans.end(), [](const Span& a, const Span& b) {
        return a.start < b.start;
    });

    QString html;
    int pos = 0;
    for (const Span& s : spans) {
        if (s.start < pos) continue;
        html += code.mid(pos, s.start - pos).toHtmlEscaped();
        html += QStringLiteral("<span style='color:%1'>").arg(s.color);
        html += code.mid(s.start, s.length).toHtmlEscaped();
        html += QStringLiteral("</span>");
        pos = s.start + s.length;
    }
    html += code.mid(pos).toHtmlEscaped();
    return html;
}

MarkdownSyntaxHighlighter::MarkdownSyntaxHighlighter(QTextDocument* parent)
    : QSyntaxHighlighter(parent)
    , m_accent(QColor("#cc2200")) {
    m_headingFormat = makeFormat(QColor("#ff5533"), true);
    m_boldFormat = makeFormat(QColor("#ffffff"), true);
    m_italicFormat = makeFormat(QColor("#cccccc"), false, true);
    m_inlineCodeFormat = makeFormat(QColor("#66ff99"));
    m_inlineCodeFormat.setFontFamily(QStringLiteral("Consolas"));
    m_linkFormat = makeFormat(QColor("#44ccff"));
    m_blockquoteFormat = makeFormat(QColor("#998877"));
    m_listFormat = makeFormat(QColor("#ddbbaa"));
    m_fenceFormat = makeFormat(QColor("#554433"));
}

void MarkdownSyntaxHighlighter::setAccentColor(const QColor& color) {
    m_accent = color;
    m_headingFormat.setForeground(color.lighter(130));
    rehighlight();
}

void MarkdownSyntaxHighlighter::highlightBlock(const QString& text) {
    static const QRegularExpression fenceRe(
        QStringLiteral(R"(^```([\w\+\#\.-]*)\s*$)"));

    const bool wasInCode = previousBlockState() == 1;
    const auto fenceMatch = fenceRe.match(text);

    if (fenceMatch.hasMatch()) {
        setFormat(0, text.length(), m_fenceFormat);
        setCurrentBlockState(wasInCode ? 0 : 1);
        return;
    }

    if (wasInCode) {
        QString lang;
        const int blockNum = currentBlock().blockNumber();
        for (int i = blockNum - 1; i >= 0; --i) {
            const QTextBlock b = document()->findBlockByNumber(i);
            const auto m = fenceRe.match(b.text());
            if (m.hasMatch()) {
                lang = m.captured(1);
                break;
            }
        }
        setCurrentBlockState(1);
        highlightCodeBlock(text, lang);
        return;
    }

    setCurrentBlockState(0);
    highlightMarkdown(text);
}

void MarkdownSyntaxHighlighter::highlightMarkdown(const QString& text) {
    // Headings
    if (text.startsWith('#')) {
        setFormat(0, text.length(), m_headingFormat);
        return;
    }

    // Blockquote
    if (text.startsWith('>')) {
        setFormat(0, text.length(), m_blockquoteFormat);
        return;
    }

    // Lists
    static const QRegularExpression listRe(QStringLiteral(R"(^\s*(?:[-*+]|\d+\.)\s)"));
    if (listRe.match(text).hasMatch()) {
        setFormat(0, text.length(), m_listFormat);
    }

    // Bold
    static const QRegularExpression boldRe(QStringLiteral(R"(\*\*[^*]+\*\*)"));
    auto it = boldRe.globalMatch(text);
    while (it.hasNext()) {
        const auto m = it.next();
        setFormat(m.capturedStart(), m.capturedLength(), m_boldFormat);
    }

    // Italic
    static const QRegularExpression italicRe(QStringLiteral(R"(\*[^*]+\*)"));
    it = italicRe.globalMatch(text);
    while (it.hasNext()) {
        const auto m = it.next();
        setFormat(m.capturedStart(), m.capturedLength(), m_italicFormat);
    }

    // Inline code
    static const QRegularExpression codeRe(QStringLiteral(R"(`[^`]+`)"));
    it = codeRe.globalMatch(text);
    while (it.hasNext()) {
        const auto m = it.next();
        setFormat(m.capturedStart(), m.capturedLength(), m_inlineCodeFormat);
    }

    // Links
    static const QRegularExpression linkRe(QStringLiteral(R"(\[[^\]]+\]\([^)]+\))"));
    it = linkRe.globalMatch(text);
    while (it.hasNext()) {
        const auto m = it.next();
        setFormat(m.capturedStart(), m.capturedLength(), m_linkFormat);
    }
}

void MarkdownSyntaxHighlighter::highlightCodeBlock(const QString& text, const QString& lang) {
    QTextCharFormat base;
    base.setFontFamily(QStringLiteral("Consolas"));
    base.setForeground(QColor("#c8c8d0"));
    setFormat(0, text.length(), base);

    const auto rules = SyntaxHighlighterEngine::rulesForLanguage(lang);
    for (const auto& rule : rules) {
        auto it = rule.pattern.globalMatch(text);
        while (it.hasNext()) {
            const auto m = it.next();
            QTextCharFormat fmt = rule.format;
            fmt.setFontFamily(QStringLiteral("Consolas"));
            setFormat(m.capturedStart(), m.capturedLength(), fmt);
        }
    }
}
