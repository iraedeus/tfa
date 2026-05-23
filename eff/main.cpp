#include <QApplication>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QSplitter>
#include <QTabWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

// ============================================================================
// БЭКЕНД: ЛОГИКА ГРАММАТИКИ
// ============================================================================

using Symbol = std::string;
using Rule = std::vector<Symbol>;
using SymbolSet = std::set<std::string>;

struct Grammar {
  std::set<Symbol> VN;
  std::set<Symbol> VT;
  std::map<Symbol, std::vector<Rule>> P;
  Symbol S;

  bool is_terminal(const Symbol &sym) const {
    return VT.count(sym) > 0 || sym == "eps";
  }
};

SymbolSet truncate_concat(const SymbolSet &set1, const SymbolSet &set2,
                          size_t k) {
  if (set1.empty())
    return set2;
  if (set2.empty())
    return set1;

  SymbolSet result;
  for (const auto &s1 : set1) {
    for (const auto &s2 : set2) {
      std::string combined = (s1 == "eps" ? "" : s1) + (s2 == "eps" ? "" : s2);
      if (combined.empty()) {
        result.insert("eps");
      } else {
        if (combined.length() > k) {
          result.insert(combined.substr(0, k));
        } else {
          result.insert(combined);
        }
      }
    }
  }
  return result;
}

SymbolSet get_FIRST_chain(const Rule &alpha, const Grammar &g, size_t k);

SymbolSet get_FIRST_sym(const Symbol &sym, const Grammar &g, size_t k) {
  SymbolSet result;
  if (g.is_terminal(sym)) {
    result.insert(sym);
    return result;
  }

  if (g.P.count(sym)) {
    for (const auto &rule : g.P.at(sym)) {
      auto rule_first = get_FIRST_chain(rule, g, k);
      result.insert(rule_first.begin(), rule_first.end());
    }
  }
  return result;
}

SymbolSet get_FIRST_chain(const Rule &alpha, const Grammar &g, size_t k) {
  if (alpha.empty())
    return {"eps"};

  SymbolSet result = get_FIRST_sym(alpha[0], g, k);

  for (size_t i = 1; i < alpha.size(); ++i) {
    if (result.count("eps")) {
      result.erase("eps");
      SymbolSet next_first = get_FIRST_sym(alpha[i], g, k);
      result = truncate_concat(result, next_first, k);
      if (get_FIRST_sym(alpha[i], g, k).count("eps")) {
        result.insert("eps");
      }
    } else {
      break;
    }
  }
  return result;
}

SymbolSet get_EFF(const Rule &alpha, const Grammar &g, size_t k) {
  SymbolSet result;
  if (alpha.empty())
    return result;

  Symbol first_sym = alpha[0];

  if (g.is_terminal(first_sym)) {
    return get_FIRST_chain(alpha, g, k);
  }

  Rule gamma(alpha.begin() + 1, alpha.end());

  if (g.P.count(first_sym)) {
    for (const auto &prod : g.P.at(first_sym)) {
      if (prod.size() == 1 && prod[0] == "eps") {
        continue;
      }
      Rule current_chain = prod;
      current_chain.insert(current_chain.end(), gamma.begin(), gamma.end());

      SymbolSet first_of_chain = get_FIRST_chain(current_chain, g, k);
      result.insert(first_of_chain.begin(), first_of_chain.end());
    }
  }
  return result;
}

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
      errorMsg = "Неверный формат правила (пропущено '->'): " + line;
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
      Rule rule;
      while (tss >> token) {
        rule.push_back(token);
      }
      if (!rule.empty()) {
        g.P[left].push_back(rule);
      }
    }
  }
  return true;
}

// ============================================================================
// ФРОНТЕНД: ИНТЕРФЕЙС НА QT
// ============================================================================

class GrammarApp : public QWidget {
public:
  GrammarApp(QWidget *parent = nullptr) : QWidget(parent) {
    setWindowTitle("LR(k) Анализ: Функция EFF_k(α)");
    resize(900, 600);
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

  QTextEdit *textResult;
  QTextEdit *textLog;

  void setupUI() {
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    QSplitter *splitter = new QSplitter(Qt::Horizontal);

    // --- ЛЕВАЯ ПАНЕЛЬ (Ввод данных) ---
    QWidget *leftPanel = new QWidget();
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    // Блок 1: Алфавит
    QGroupBox *groupAlphabet = new QGroupBox("Алфавит грамматики");
    QFormLayout *formAlphabet = new QFormLayout(groupAlphabet);
    inputVT = new QLineEdit("a b c");
    inputVN = new QLineEdit("S A B C");
    inputS = new QLineEdit("S");
    formAlphabet->addRow("Терминалы (V_T):", inputVT);
    formAlphabet->addRow("Нетерминалы (V_N):", inputVN);
    formAlphabet->addRow("Стартовый (S):", inputS);

    // Блок 2: Правила вывода
    QGroupBox *groupRules = new QGroupBox("Правила вывода (P)");
    QVBoxLayout *layoutRules = new QVBoxLayout(groupRules);
    inputRules = new QTextEdit();
    inputRules->setFontFamily("Consolas");
    inputRules->setPlaceholderText("Пример:\nS -> A B\nA -> B a | eps");
    inputRules->setText("S -> A B\nA -> B a | eps\nB -> C b | C\nC -> c | eps");
    layoutRules->addWidget(inputRules);

    // Блок 3: Параметры анализа
    QGroupBox *groupParams = new QGroupBox("Параметры анализа");
    QFormLayout *formParams = new QFormLayout(groupParams);
    spinK = new QSpinBox();
    spinK->setRange(1, 10);
    spinK->setValue(2);
    inputAlpha = new QLineEdit("S");
    formParams->addRow("Длина префикса (k):", spinK);
    formParams->addRow("Цепочка α:", inputAlpha);

    btnCalculate = new QPushButton("Вычислить EFF_k(α)");
    btnCalculate->setCursor(Qt::PointingHandCursor);

    leftLayout->addWidget(groupAlphabet);
    leftLayout->addWidget(groupRules, 1); // Тянется
    leftLayout->addWidget(groupParams);
    leftLayout->addWidget(btnCalculate);

    // --- ПРАВАЯ ПАНЕЛЬ (Результаты) ---
    QTabWidget *tabWidget = new QTabWidget();

    textResult = new QTextEdit();
    textResult->setReadOnly(true);
    textResult->setFontPointSize(14);
    tabWidget->addTab(textResult, "Результат вычисления");

    textLog = new QTextEdit();
    textLog->setReadOnly(true);
    textLog->setFontFamily("Consolas");
    tabWidget->addTab(textLog, "Лог парсинга грамматики");

    // Добавляем панели в сплиттер
    splitter->addWidget(leftPanel);
    splitter->addWidget(tabWidget);
    splitter->setSizes({350, 550}); // Пропорции по умолчанию

    mainLayout->addWidget(splitter);

    connect(btnCalculate, &QPushButton::clicked, this,
            &GrammarApp::onCalculate);
  }

  void applyStyles() {
    // Светлая тема с акцентами (как в ваших отчетах)
    QString style = R"(
            QWidget {
                background-color: #F8F9FA;
                font-family: "Segoe UI", "Helvetica Neue", Arial, sans-serif;
                font-size: 13px;
                color: #212529;
            }
            QGroupBox {
                background-color: #FFFFFF;
                border: 1px solid #DEE2E6;
                border-radius: 6px;
                margin-top: 12px;
                padding-top: 10px;
                font-weight: bold;
            }
            QGroupBox::title {
                subcontrol-origin: margin;
                subcontrol-position: top left;
                padding: 0 5px;
                color: #495057;
            }
            QLineEdit, QTextEdit, QSpinBox {
                background-color: #FFFFFF;
                border: 1px solid #CED4DA;
                border-radius: 4px;
                padding: 4px;
            }
            QLineEdit:focus, QTextEdit:focus, QSpinBox:focus {
                border: 1px solid #80BDFF;
            }
            QPushButton {
                background-color: #0d6efd;
                color: white;
                font-weight: bold;
                border: none;
                border-radius: 4px;
                padding: 10px;
                font-size: 14px;
            }
            QPushButton:hover {
                background-color: #0b5ed7;
            }
            QPushButton:pressed {
                background-color: #0a58ca;
            }
            QTabWidget::pane {
                border: 1px solid #DEE2E6;
                background: white;
                border-radius: 4px;
            }
            QTabBar::tab {
                background: #E9ECEF;
                border: 1px solid #DEE2E6;
                padding: 8px 16px;
                margin-right: 2px;
                border-top-left-radius: 4px;
                border-top-right-radius: 4px;
            }
            QTabBar::tab:selected {
                background: #FFFFFF;
                border-bottom-color: #FFFFFF;
                font-weight: bold;
                color: #0d6efd;
            }
            QSplitter::handle {
                background-color: #DEE2E6;
                width: 2px;
            }
        )";
    this->setStyleSheet(style);
  }

  void onCalculate() {
    Grammar g;
    std::string errorMsg;

    // Парсинг алфавита
    std::stringstream ssVT(inputVT->text().toStdString());
    std::string token;
    while (ssVT >> token)
      g.VT.insert(token);

    std::stringstream ssVN(inputVN->text().toStdString());
    while (ssVN >> token)
      g.VN.insert(token);

    g.S = inputS->text().trimmed().toStdString();
    if (g.S.empty()) {
      QMessageBox::warning(this, "Внимание", "Стартовый символ не указан.");
      return;
    }

    // Парсинг правил
    if (!parseRules(inputRules->toPlainText().toStdString(), g, errorMsg)) {
      QMessageBox::critical(this, "Ошибка парсинга",
                            QString::fromStdString(errorMsg));
      return;
    }

    size_t k = spinK->value();

    // Парсинг альфы
    Rule alpha;
    std::stringstream ssAlpha(inputAlpha->text().toStdString());
    while (ssAlpha >> token)
      alpha.push_back(token);

    if (alpha.empty()) {
      QMessageBox::warning(this, "Внимание", "Введите цепочку α.");
      return;
    }

    // Вывод лога парсинга на вторую вкладку
    QString logStr = "<b>Распознанная грамматика G:</b><br><br>";
    logStr += "<b>V<sub>T</sub></b> = { ";
    for (const auto &t : g.VT)
      logStr += QString::fromStdString(t) + " ";
    logStr += "}<br>";

    logStr += "<b>V<sub>N</sub></b> = { ";
    for (const auto &n : g.VN)
      logStr += QString::fromStdString(n) + " ";
    logStr += "}<br>";

    logStr += "<b>S</b> = " + QString::fromStdString(g.S) + "<br><br>";
    logStr += "<b>P (Правила):</b><br>";
    for (const auto &pair : g.P) {
      logStr += QString::fromStdString(pair.first) + " &rarr; ";
      for (size_t i = 0; i < pair.second.size(); ++i) {
        for (const auto &sym : pair.second[i]) {
          logStr += QString::fromStdString(sym) + " ";
        }
        if (i < pair.second.size() - 1)
          logStr += "| ";
      }
      logStr += "<br>";
    }
    textLog->setHtml(logStr);

    // Вычисление EFF
    SymbolSet eff_set = get_EFF(alpha, g, k);

    // Вывод результата на первую вкладку (форматировано)
    QString resultHtml =
        "<div style='font-family: monospace; color: #212529;'>";
    resultHtml +=
        QString("<span style='color: #0d6efd;'><b>EFF<sub>%1</sub>( ").arg(k);
    for (size_t i = 0; i < alpha.size(); ++i) {
      resultHtml +=
          QString::fromStdString(alpha[i]) + (i == alpha.size() - 1 ? "" : " ");
    }
    resultHtml += " )</b></span> = { ";

    if (eff_set.empty()) {
      resultHtml += "&empty;";
    } else {
      bool first = true;
      for (const auto &sym : eff_set) {
        if (!first)
          resultHtml += ", ";
        resultHtml += "<span style='color: #198754; font-weight: bold;'>" +
                      QString::fromStdString(sym) + "</span>";
        first = false;
      }
    }
    resultHtml += " }</div>";

    textResult->setHtml(resultHtml);
  }
};

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  GrammarApp window;
  window.show();
  return app.exec();
}
