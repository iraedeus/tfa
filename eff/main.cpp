#include <QApplication>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QSplitter>
#include <QTabWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

// ============================================================================
// БЭКЕНД: СТРОГАЯ МАТЕМАТИЧЕСКАЯ ЛОГИКА (FIRST_k и EFF_k)
// ============================================================================

using Symbol = std::string;
using Chain = std::vector<Symbol>;
using ChainSet = std::set<Chain>;

struct Grammar {
  std::set<Symbol> VN;
  std::set<Symbol> VT;
  std::map<Symbol, std::vector<Chain>> P;
  Symbol S;

  bool is_terminal(const Symbol &sym) const {
    return VT.count(sym) > 0 || sym == "eps";
  }
};

ChainSet truncate_concat(const ChainSet &s1, const ChainSet &s2, size_t k) {
  if (s1.empty() || s2.empty())
    return {};
  ChainSet res;
  for (const auto &c1 : s1) {
    for (const auto &c2 : s2) {
      Chain comb;
      for (const auto &sym : c1)
        if (sym != "eps")
          comb.push_back(sym);
      for (const auto &sym : c2)
        if (sym != "eps")
          comb.push_back(sym);

      if (comb.empty()) {
        res.insert({"eps"});
      } else {
        if (comb.size() > k)
          comb.resize(k);
        res.insert(comb);
      }
    }
  }
  return res;
}

class LogicEngine {
public:
  std::map<Symbol, ChainSet> FIRST;
  std::map<Symbol, ChainSet> EFF;
  Grammar g;
  size_t k;

  LogicEngine(const Grammar &grammar, size_t length) : g(grammar), k(length) {}

  void compute() {
    compute_FIRST();
    compute_EFF();
  }

  ChainSet get_eff_for_alpha(const Chain &alpha) {
    if (alpha.empty() || (alpha.size() == 1 && alpha[0] == "eps"))
      return {{"eps"}};

    ChainSet res = EFF.at(alpha[0]);
    for (size_t i = 1; i < alpha.size(); ++i) {
      res = truncate_concat(res, FIRST.at(alpha[i]), k);
    }
    return res;
  }

private:
  void compute_FIRST() {
    for (const auto &t : g.VT)
      FIRST[t] = {{t}};
    FIRST["eps"] = {{"eps"}};
    for (const auto &n : g.VN)
      FIRST[n] = {};

    bool changed = true;
    while (changed) {
      changed = false;
      for (const auto &n : g.VN) {
        if (!g.P.count(n))
          continue;
        for (const auto &rule : g.P.at(n)) {
          ChainSet current = {{"eps"}};
          for (const auto &sym : rule) {
            current = truncate_concat(current, FIRST[sym], k);
          }
          for (const auto &c : current) {
            if (FIRST[n].insert(c).second)
              changed = true;
          }
        }
      }
    }
  }

  void compute_EFF() {
    for (const auto &t : g.VT)
      EFF[t] = {{t}};
    EFF["eps"] = {{"eps"}};
    for (const auto &n : g.VN)
      EFF[n] = {};

    bool changed = true;
    while (changed) {
      changed = false;
      for (const auto &n : g.VN) {
        if (!g.P.count(n))
          continue;
        for (const auto &rule : g.P.at(n)) {
          if (rule.size() == 1 && rule[0] == "eps")
            continue;

          ChainSet current = EFF[rule[0]];
          for (size_t i = 1; i < rule.size(); ++i) {
            current = truncate_concat(current, FIRST[rule[i]], k);
          }
          for (const auto &c : current) {
            if (EFF[n].insert(c).second)
              changed = true;
          }
        }
      }
    }
  }
};

std::string trim(const std::string &str) {
  size_t first = str.find_first_not_of(" \t\r\n");
  if (first == std::string::npos)
    return "";
  size_t last = str.find_last_not_of(" \t\r\n");
  return str.substr(first, (last - first + 1));
}

bool parseRules(const std::string &text, Grammar &g, std::string &errorMsg) {
  std::stringstream ss(text);
  std::string line;
  while (std::getline(ss, line)) {
    line = trim(line);
    if (line.empty())
      continue;

    size_t arrowPos = line.find("->");
    if (arrowPos == std::string::npos) {
      errorMsg = "Ошибка синтаксиса: пропущено '->' в правиле: " + line;
      return false;
    }

    std::string left = trim(line.substr(0, arrowPos));
    std::string rightSide = line.substr(arrowPos + 2);

    if (left.empty()) {
      errorMsg = "Пустая левая часть в строке: " + line;
      return false;
    }

    g.VN.insert(left);

    std::stringstream rss(rightSide);
    std::string alternative;
    while (std::getline(rss, alternative, '|')) {
      alternative = trim(alternative);
      if (alternative.empty())
        continue;

      std::stringstream tss(alternative);
      std::string token;
      Chain rule;
      while (tss >> token) {
        rule.push_back(token);
      }
      if (!rule.empty())
        g.P[left].push_back(rule);
    }
  }
  return true;
}

// ============================================================================
// ФРОНТЕНД: СОВРЕМЕННЫЙ ИНТЕРФЕЙС НА QT
// ============================================================================

class GrammarApp : public QWidget {
public:
  GrammarApp(QWidget *parent = nullptr) : QWidget(parent) {
    setWindowTitle("LR(k) Анализатор: Функция EFF_k(α)");
    resize(950, 650);
    setupUI();
    applyStyles();
  }

private:
  QLineEdit *inputVT;
  QLineEdit *inputVN;
  QLineEdit *inputS;
  QTextEdit *inputRules;
  QSpinBox *spinK;
  QLineEdit *inputAlpha;
  QPushButton *btnCalculate;
  QLabel *errorLabel;

  QTextEdit *textResult;
  QTextEdit *textLog;

  void setupUI() {
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    mainLayout->setSpacing(15);

    QSplitter *splitter = new QSplitter(Qt::Horizontal);

    // --- ЛЕВАЯ ПАНЕЛЬ ---
    QWidget *leftPanel = new QWidget();
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(12);

    // Блок 1: Алфавит (надежная сетка QGridLayout)
    QGroupBox *groupAlphabet = new QGroupBox("Алфавит грамматики");
    QGridLayout *gridAlphabet = new QGridLayout(groupAlphabet);
    gridAlphabet->setContentsMargins(15, 20, 15, 15);
    gridAlphabet->setSpacing(10);

    QLabel *labelVT = new QLabel("V_T (Терминалы):");
    QLabel *labelVN = new QLabel("V_N (Нетерминалы):");
    QLabel *labelS = new QLabel("S (Аксиома):");

    inputVT = new QLineEdit();
    inputVT->setPlaceholderText("Например: a b c");
    inputVN = new QLineEdit();
    inputVN->setPlaceholderText("Например: S A B C");
    inputS = new QLineEdit();
    inputS->setPlaceholderText("Например: S");

    gridAlphabet->addWidget(labelVT, 0, 0, Qt::AlignVCenter | Qt::AlignLeft);
    gridAlphabet->addWidget(inputVT, 0, 1);
    gridAlphabet->addWidget(labelVN, 1, 0, Qt::AlignVCenter | Qt::AlignLeft);
    gridAlphabet->addWidget(inputVN, 1, 1);
    gridAlphabet->addWidget(labelS, 2, 0, Qt::AlignVCenter | Qt::AlignLeft);
    gridAlphabet->addWidget(inputS, 2, 1);
    gridAlphabet->setColumnStretch(1, 1);

    // Блок 2: Правила вывода
    QGroupBox *groupRules = new QGroupBox("Правила вывода (P)");
    QVBoxLayout *layoutRules = new QVBoxLayout(groupRules);
    layoutRules->setContentsMargins(15, 20, 15, 15);
    inputRules = new QTextEdit();
    inputRules->setFontFamily("Consolas");
    inputRules->setPlaceholderText("S -> A B\nA -> B a | eps");
    layoutRules->addWidget(inputRules);

    // Блок 3: Параметры анализа
    QGroupBox *groupParams = new QGroupBox("Параметры функции EFF");
    QGridLayout *gridParams = new QGridLayout(groupParams);
    gridParams->setContentsMargins(15, 20, 15, 15);
    gridParams->setSpacing(10);

    QLabel *labelK = new QLabel("Длина k:");
    QLabel *labelAlpha = new QLabel("Цепочка α:");

    spinK = new QSpinBox();
    spinK->setRange(1, 10);
    spinK->setValue(2);
    inputAlpha = new QLineEdit();
    inputAlpha->setPlaceholderText("Например: A b");

    gridParams->addWidget(labelK, 0, 0, Qt::AlignVCenter | Qt::AlignLeft);
    gridParams->addWidget(spinK, 0, 1);
    gridParams->addWidget(labelAlpha, 1, 0, Qt::AlignVCenter | Qt::AlignLeft);
    gridParams->addWidget(inputAlpha, 1, 1);
    gridParams->setColumnStretch(1, 1);

    errorLabel = new QLabel("");
    errorLabel->setStyleSheet("color: #dc3545; font-weight: bold;");
    errorLabel->setWordWrap(true);
    errorLabel->hide();

    btnCalculate = new QPushButton("Рассчитать EFF_k(α)");
    btnCalculate->setCursor(Qt::PointingHandCursor);

    leftLayout->addWidget(groupAlphabet);
    leftLayout->addWidget(groupRules, 1);
    leftLayout->addWidget(groupParams);
    leftLayout->addWidget(errorLabel);
    leftLayout->addWidget(btnCalculate);

    // --- ПРАВАЯ ПАНЕЛЬ ---
    QTabWidget *tabWidget = new QTabWidget();

    textResult = new QTextEdit();
    textResult->setReadOnly(true);
    tabWidget->addTab(textResult, "Результат EFF_k");

    textLog = new QTextEdit();
    textLog->setReadOnly(true);
    tabWidget->addTab(textLog, "Детальный лог");

    splitter->addWidget(leftPanel);
    splitter->addWidget(tabWidget);
    splitter->setSizes({400, 550});

    mainLayout->addWidget(splitter);

    connect(btnCalculate, &QPushButton::clicked, this,
            &GrammarApp::onCalculate);

    inputVT->setText("a b c");
    inputVN->setText("S A B C");
    inputS->setText("S");
    inputRules->setText("S -> A B\nA -> B a | eps\nB -> C b | C\nC -> c | eps");
    inputAlpha->setText("A C");
  }

  void applyStyles() {
    QString style = R"(
            QWidget {
                background-color: #F8F9FA;
                font-family: "Segoe UI", Arial, sans-serif;
                font-size: 13px;
                color: #212529;
            }
            QLabel {
                font-weight: normal;
                color: #495057;
                padding-right: 5px;
            }
            QGroupBox {
                background-color: #FFFFFF;
                border: 1px solid #DEE2E6;
                border-radius: 6px;
                margin-top: 18px; /* Достаточно места для заголовка */
                font-weight: bold;
            }
            QGroupBox::title {
                subcontrol-origin: margin;
                subcontrol-position: top left;
                left: 15px;
                top: 0px;
                padding: 0 5px;
                color: #0d6efd;
            }
            QLineEdit, QTextEdit, QSpinBox {
                background-color: #FFFFFF;
                border: 1px solid #CED4DA;
                border-radius: 4px;
                padding: 6px;
                selection-background-color: #0d6efd;
            }
            QLineEdit:focus, QTextEdit:focus, QSpinBox:focus {
                border: 1px solid #86b7fe;
            }
            QPushButton {
                background-color: #0d6efd;
                color: white;
                font-weight: bold;
                border: none;
                border-radius: 6px;
                padding: 12px;
                font-size: 14px;
            }
            QPushButton:hover { background-color: #0b5ed7; }
            QPushButton:pressed { background-color: #0a58ca; }
            QTabWidget::pane {
                border: 1px solid #DEE2E6;
                background: white;
                border-radius: 6px;
            }
            QTabBar::tab {
                background: #E9ECEF;
                border: 1px solid #DEE2E6;
                padding: 10px 20px;
                margin-right: 2px;
                border-top-left-radius: 6px;
                border-top-right-radius: 6px;
                color: #495057;
            }
            QTabBar::tab:selected {
                background: #FFFFFF;
                border-bottom-color: #FFFFFF;
                font-weight: bold;
                color: #0d6efd;
            }
            QSplitter::handle {
                background-color: #DEE2E6;
                width: 3px;
                margin: 0 5px;
            }
        )";
    this->setStyleSheet(style);
  }

  void showError(const std::string &msg) {
    errorLabel->setText(QString::fromStdString("⚠ " + msg));
    errorLabel->show();
  }

  QString printChainSet(const ChainSet &s) {
    if (s.empty())
      return "&empty;";
    QString res;
    bool first = true;
    for (const auto &chain : s) {
      if (!first)
        res += ", ";
      QString cStr;
      for (const auto &sym : chain)
        cStr += QString::fromStdString(sym);
      if (cStr.isEmpty())
        cStr = "&epsilon;";
      res += "<span style='color: #198754; font-weight: bold;'>" + cStr +
             "</span>";
      first = false;
    }
    return res;
  }

  void onCalculate() {
    errorLabel->hide();
    Grammar g;
    std::string errorMsg;

    std::stringstream ssVT(inputVT->text().toStdString());
    std::string token;
    while (ssVT >> token)
      g.VT.insert(token);

    std::stringstream ssVN(inputVN->text().toStdString());
    while (ssVN >> token)
      g.VN.insert(token);

    g.S = inputS->text().trimmed().toStdString();

    if (!parseRules(inputRules->toPlainText().toStdString(), g, errorMsg)) {
      showError(errorMsg);
      return;
    }

    size_t k = spinK->value();
    Chain alpha;
    std::stringstream ssAlpha(inputAlpha->text().toStdString());
    while (ssAlpha >> token)
      alpha.push_back(token);

    if (alpha.empty() && inputAlpha->text().trimmed().isEmpty()) {
      // Пустая цепочка
    } else {
      for (const auto &sym : alpha) {
        if (!g.VT.count(sym) && !g.VN.count(sym) && sym != "eps") {
          showError("Символ '" + sym +
                    "' в цепочке α не принадлежит алфавиту.");
          return;
        }
      }
    }

    LogicEngine engine(g, k);
    engine.compute();
    ChainSet eff_alpha = engine.get_eff_for_alpha(alpha);

    QString logStr =
        "<h3 style='color: #0d6efd;'>Внутреннее состояние анализатора</h3>";
    logStr += "<b>Таблица FIRST_k:</b><br><table style='margin-left:10px;'>";
    for (const auto &n : g.VN) {
      logStr += "<tr><td style='padding-right:15px;'>FIRST<sub>" +
                QString::number(k) + "</sub>(" + QString::fromStdString(n) +
                ")</td>";
      logStr += "<td>= { " + printChainSet(engine.FIRST[n]) + " }</td></tr>";
    }
    logStr += "</table><br>";

    logStr +=
        "<b>Таблица базовых EFF_k:</b><br><table style='margin-left:10px;'>";
    for (const auto &n : g.VN) {
      logStr += "<tr><td style='padding-right:15px;'>EFF<sub>" +
                QString::number(k) + "</sub>(" + QString::fromStdString(n) +
                ")</td>";
      logStr += "<td>= { " + printChainSet(engine.EFF[n]) + " }</td></tr>";
    }
    logStr += "</table>";
    textLog->setHtml(logStr);

    QString resultHtml = "<div style='font-family: \"Segoe UI\", sans-serif; "
                         "font-size: 16px; margin: 20px;'>";
    resultHtml += "Определение функции завершено.<br><br>";

    QString alphaStr;
    if (alpha.empty())
      alphaStr = "&epsilon;";
    else {
      for (size_t i = 0; i < alpha.size(); ++i) {
        alphaStr += QString::fromStdString(alpha[i]) +
                    (i == alpha.size() - 1 ? "" : " ");
      }
    }

    resultHtml += QString("<span style='color: #0d6efd; font-size: "
                          "20px;'><b>EFF<sub>%1</sub>( %2 )</b></span> = <span "
                          "style='font-size: 20px;'>{ ")
                      .arg(k)
                      .arg(alphaStr);
    resultHtml += printChainSet(eff_alpha);
    resultHtml += " }</span></div>";

    textResult->setHtml(resultHtml);
  }
};

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  GrammarApp window;
  window.show();
  return app.exec();
}
