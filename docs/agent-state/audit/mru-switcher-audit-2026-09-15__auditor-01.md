# Аудиторский отчёт: mru-switcher

**Проект:** `code-warlord-dev/mru-switcher`  
**Дата аудита:** 2026-09-15  
**Стадия разработки:** M0–M2 завершены; M3 и последующие этапы ещё не реализовывались  
**Цель:** оценить качество уже реализованного кода и архитектуры без предъявления незавершённых требований как дефектов.

---

## 1. Executive Summary

На момент аудита проект находится **на стадии M0–M2**, поэтому оценивать его как законченный production/Enterprise-продукт было бы некорректно.

По фактически реализованной части проект выглядит **сильно выше обычного MVP**:

- архитектура разделена на domain/core и Hyprland adapter;
- бизнес-логика не завязана напрямую на API Hyprland;
- предусмотрены абстракции для scheduler, focus gateway, window source и UI;
- состояние сессии и история разделены достаточно чисто;
- для идентификации окон используется адрес + generation, что защищает от повторного использования адреса;
- есть явная работа с lifetime и teardown;
- присутствует fail-closed проверка API/ABI совместимости;
- конфигурация и parsing вынесены отдельно;
- код ориентирован на тестируемость.

### Итоговая оценка

| Область | Оценка M0–M2 |
|---|---|
| Архитектура | **Expert / Pro** |
| Domain/core | **Expert / Pro** |
| Управление состоянием | **Expert / Pro** |
| Ownership / lifetime | **Expert / Pro** |
| C++ качество | **Expert / Pro** |
| Тестируемость | **Pro** |
| Конфигурация | **Pro** |
| Hyprland integration | **Pro** |
| Документация / ADR-подход | **Pro** |
| Production hardening | **N/A для текущей стадии** |

**Главный вывод:** переписывать M0–M2 не требуется. Архитектурный фундамент хороший и пригоден для дальнейшего развития в сторону Enterprise/Expert уровня.

При этом **Enterprise | Expert | Pro** — это не утверждение, что уже реализован весь Enterprise-функционал. Это оценка качества инженерных решений в реализованном объёме.

---

# 2. Важное уточнение границ аудита

Предыдущая версия аудита смешивала три разных категории:

1. реальные проблемы уже написанного кода;
2. функции, запланированные на M3+;
3. требования, которые разумно проверять только перед production-релизом.

Это некорректно для проекта на стадии M0–M2.

Поэтому в настоящем отчёте:

- **M3 scope filtering не считается дефектом;**
- `external_socket` не считается отсутствующей функциональностью;
- переход на `hyprlang V2` не считается текущей ошибкой;
- production-grade CI/packaging не оценивается как блокер M0–M2;
- требования M3 рассматриваются отдельно как **Design Gate**, а не как замечания к текущей реализации.

---

# 3. Архитектура

## 3.1. Разделение ответственности

Архитектурное разделение на:

```text
src/domain/
src/plugin/
src/plugin/hypr/
```

— правильное.

Особенно хорошо, что domain-слой использует собственные абстракции:

- `FocusGateway`
- `HistoryTracker`
- `SchedulerPort`
- `WindowSource`
- `UiPort`
- `SessionController`
- `WindowRef`
- `Snapshot`
- `Scope`

Это существенно лучше, чем реализация всей логики непосредственно в callback'ах Hyprland.

### Оценка

**Expert / Pro — PASS**

Архитектура уже предусматривает возможность заменить инфраструктурные реализации без переписывания core logic.

---

# 4. Domain / core logic

`HistoryTracker` реализован достаточно аккуратно.

Ключевые моменты:

- debounce выполняется через абстракцию scheduler;
- pending job отменяется перед созданием нового;
- при блокировке сессии pending job отменяется;
- история очищается/seed-ится через валидатор;
- duplicate entries предотвращаются;
- commit удаляет старую позицию и помещает окно в начало.

Концептуально:

```text
focus event
    ↓
cancel previous pending job
    ↓
schedule debounce
    ↓
validate window
    ↓
remove old occurrence
    ↓
insert at front
```

Это хорошая модель для MRU.

### Отдельно

Использование `std::vector` для MRU **не является проблемой**.

Для обычного размера истории окон стоимость линейного поиска/erase несущественна, а структура данных остаётся простой и предсказуемой.

Нет оснований преждевременно заменять её на более сложную комбинацию `list + unordered_map`.

### Оценка

**Expert / Pro — PASS**

---

# 5. Window identity

Один из наиболее сильных участков текущей реализации — `WindowIdentityRegistry`.

Используется:

```text
address + generation
```

вместо простого хранения указателя/адреса.

Это важно из-за возможного повторного использования адреса объекта после уничтожения окна.

Текущая логика:

```text
window created
    ↓
register
    ↓
(address, generation)

window closed
    ↓
entry marked closed

new object reuses address
    ↓
new generation
```

В результате старый `WindowRef` не начинает ошибочно ссылаться на новый объект.

Это именно тот класс защиты, который имеет смысл в низкоуровневом C++/plugin-коде.

### Оценка

**Expert / Pro — PASS**

---

# 6. Ownership и lifetime

В коде нет признаков попытки решать жизненный цикл объектов через глобальное владение или чрезмерное использование raw pointers в domain-логике.

Особенно важен destructor path у `HistoryTracker`:

```cpp
HistoryTracker::~HistoryTracker() {
    cancel_pending();
}
```

То есть объект не оставляет scheduled callback, который продолжит обращаться к уничтоженному `this`.

Это хороший базовый lifetime invariant.

### Оценка

**Expert / Pro — PASS**

---

# 7. Scheduler / asynchronous behavior

Scheduler вынесен в `SchedulerPort`.

Это архитектурно правильнее, чем напрямую зашивать Hyprland timer API в `HistoryTracker`.

Плюсы:

- domain не знает инфраструктуру;
- scheduler можно заменить;
- unit tests не требуют реального event loop;
- lifetime behavior можно тестировать отдельно.

При этом сам механизм scheduling всё равно должен оставаться однопоточным относительно Hyprland event loop, если это является контрактом plugin API.

На текущей стадии нет оснований считать отсутствие multithreading ошибкой.

---

# 8. Configuration

`PluginConfig` содержит явные default values и отдельные parsing helpers:

- `parse_scope`
- `parse_scope_token`
- `clamp_debounce_ms`
- `scope_name`
- `parse_ui_backend`
- `parse_start_offset`

Это хорошее направление.

Особенно правильно, что значение debounce ограничивается:

```text
[0, 5000]
```

а parsing отделён от бизнес-логики.

### Небольшое замечание

Конфигурационный слой будет нуждаться в дальнейшей эволюции при M3, когда появятся дополнительные scope/API/backend semantics.

Это **не проблема M0–M2**.

### Оценка

**Pro — PASS**

---

# 9. Hyprland integration

Интеграционный слой выделен отдельно от domain.

Это критически важно для plugin architecture: Hyprland API должен рассматриваться как infrastructure boundary, а не как часть domain model.

Текущий подход позволяет реализовать дальнейшие M3 изменения без протаскивания Hyprland-specific типов через весь core.

### Что не следует считать дефектом сейчас

Если текущая реализация ещё использует API, который планируется заменить на `hyprlang V2`, это не является дефектом само по себе, пока M3 migration ещё не началась.

То же относится к новым socket/config surfaces.

### Оценка

**Pro — PASS**

---

# 10. Session state / transactional behavior

Архитектура snapshot + session lock выглядит зрелее типичного plugin MVP.

Важная идея:

```text
before session
    ↓
capture state
    ↓
lock history
    ↓
perform switching
    ↓
cancel / restore
```

Такой подход позволяет не смешивать:

- исходное состояние пользователя;
- временное состояние selector/session;
- обновляемую MRU history.

Это правильная база для дальнейшего расширения.

### Оценка

**Expert / Pro — PASS**

---

# 11. Fail-closed behavior

Проверки совместимости API/ABI и раннее прекращение работы при несовместимости — правильный выбор для Hyprland plugin.

Для plugin ecosystem принцип:

```text
unknown / incompatible environment
        ↓
do not load / do not operate
```

предпочтительнее попытки «авось заработает».

Это снижает вероятность hard-to-debug runtime corruption.

### Оценка

**Expert / Pro — PASS**

---

# 12. Что реально стоит улучшать

Ниже только вещи, которые имеют инженерный смысл. Это не список придирок.

## 12.1. Integration tests

Unit-testability архитектуры хорошая, но перед M3 стоит постепенно наращивать integration coverage именно вокруг boundary:

```text
Hyprland event
    ↓
adapter
    ↓
domain
    ↓
Hyprland action
```

Особенно полезны тесты на:

- window close;
- address reuse;
- invalid/stale `WindowRef`;
- session cancel;
- focus restoration;
- scheduler cancellation;
- config parsing;
- startup/shutdown.

Это не означает, что текущий код плох.

Это нормальный следующий шаг от **Pro codebase** к **production-grade codebase**.

---

## 12.2. CI

CI стоит проверять не только на уровне:

```text
compile / unit tests
```

но и на уровне:

```text
actual plugin shared object
        +
Hyprland headers
        +
ABI/API guard
        +
smoke load
```

Особенно после M3, когда появится больше Hyprland-specific behavior.

Это backlog quality gate, а не M0–M2 defect.

---

## 12.3. Distribution matrix

Для plugin проекта имеет смысл иметь минимум:

```text
Arch / CachyOS
Fedora / Nobara
Ubuntu / Debian-family
RHEL-family
```

с приоритетом той среды, под которую проект реально разрабатывается и тестируется.

Но делать из отсутствия полной distro matrix блокер текущего M0–M2 аудита неправильно.

---

# 13. Что НЕ является проблемой

Следующие вещи не требуют исправления только ради «Enterprise-вида»:

### `unique_ptr`

Нормальный современный C++ ownership primitive.

### `std::vector` для MRU

Адекватная структура для небольшого набора окон.

### Разделение на большое количество маленьких header'ов

В domain-driven plugin architecture это скорее плюс, пока зависимости остаются направленными.

### Отсутствие multithreading

Для Hyprland plugin это не недостаток само по себе.

### `-Werror`

Наоборот, полезный quality gate.

### Static/global plugin state

Сам по себе не является дефектом. Важны ownership, lifetime и отсутствие неконтролируемого состояния.

---

# 14. M3 Design Gate

Следующие вопросы относятся уже к проектированию M3 и **не являются замечаниями к M0–M2**.

---

## Q1 — Scope model

### Рекомендация: **A**

Scope должен оставаться частью domain model.

То есть примерно:

```text
Scope
 ├── Global
 ├── Workspace
 ├── Monitor
 └── ...
```

а Hyprland adapter должен только переводить реальные Hyprland objects/events в domain semantics.

Причина: иначе Hyprland-specific conditionals начнут постепенно проникать в core.

---

## Q2 — API pinning

### Рекомендация: **A**

Зафиксировать Hyprland API compatibility на конкретной версии:

```text
v0.56.2
```

и явно проверять совместимость.

Для plugin это намного безопаснее, чем пытаться поддерживать неопределённый диапазон версий.

Лучше:

```text
known API
    ↓
compile
    ↓
validate
```

чем:

```text
maybe compatible
    ↓
compile somehow
    ↓
runtime surprises
```

---

## Q3 — Scope architecture

### Рекомендация: **A**, с возможностью B только если M3 покажет реальную необходимость

Предикаты scope должны быть чистыми domain operations.

Например концептуально:

```text
is_in_scope(window, scope_context)
```

а получение:

- workspace;
- monitor;
- special workspace;
- app identity

остаётся ответственностью adapter.

Таким образом:

```text
Hyprland objects
        ↓
adapter
        ↓
ScopeContext
        ↓
domain predicate
```

Это сохранит тестируемость.

---

## Q4 — Special workspace visibility

### Рекомендация: **A**

Special workspace должен рассматриваться как отдельная, явно определённая категория.

Не стоит маскировать его под обычный workspace через набор исключений.

Нужен явный semantic rule:

```text
ordinary workspace
special workspace
```

а уже policy решает, участвует ли special workspace в конкретном scope.

Это будет намного проще сопровождать при дальнейшем расширении.

---

## Q5 — External socket / hyprlang V2

### Рекомендация: **A**

Не смешивать migration infrastructure с domain feature work.

Предпочтительный порядок:

```text
M3
 ├── domain scope model
 ├── Hyprland adapter
 ├── config semantics
 └── tests

после стабилизации
 └── external/config surface migration
```

Если конкретный API migration является обязательным prerequisite для M3, тогда сделать его отдельным техническим тикетом/PR с независимым validation.

---

# 15. Рекомендуемый порядок M3

Предлагаемый порядок:

```text
1. hyprlang/API compatibility groundwork
        ↓
2. scope domain model
        ↓
3. Hyprland scope adapter
        ↓
4. config surface
        ↓
5. special workspace / app semantics
        ↓
6. integration tests
        ↓
7. CI / distro validation
```

Главный принцип:

> Сначала определить domain semantics, затем переводить Hyprland state в эти semantics.

Не наоборот.

---

# 16. Final verdict

## M0–M2

**PASS**

Код выглядит как работа опытного C++/systems developer, а архитектура — как база, которую имеет смысл продолжать, а не переписывать.

### Уровень

**Enterprise-oriented architecture / Expert implementation quality / Pro maturity for current stage.**

При этом формулировка важна:

> Проект **ещё не является законченным Enterprise production product**, потому что M3+ функциональность и production hardening ещё не завершены.

Но:

> **Уже реализованная архитектура и кодовая база M0–M2 соответствуют уровню, с которого нормально продолжать разработку Enterprise/Expert-grade проекта.**

### Решение

**GO FOR M3.**

Не делать rewrite.  
Не усложнять существующий core без причины.  
Не исправлять то, что является сознательно отложенной M3 функциональностью.

Следующий разумный шаг — зафиксировать Q1–Q5 в **ADR-016**, затем декомпозировать M3 на небольшие независимые PR/tickets с отдельными тестовыми и compatibility gates.

---

## Краткая матрица решения

| Вопрос | Решение |
|---|---|
| Текущий M0–M2 код | **PASS** |
| Нужен rewrite | **Нет** |
| Архитектура | **Expert / Pro** |
| Domain | **Expert / Pro** |
| C++ / lifetime | **Expert / Pro** |
| Testability | **Pro** |
| M3 scope filtering | **N/A сейчас** |
| `hyprlang V2` | **N/A сейчас** |
| `external_socket` | **N/A сейчас** |
| Q1 | **A** |
| Q2 | **A** |
| Q3 | **A** |
| Q4 | **A** |
| Q5 | **A** |
| Общий gate | **GO FOR M3** |
