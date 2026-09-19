# M6-T6 rapid-Tab live smoke (issue #52)

Date (UTC): 2026-09-19 ~19:58–20:10
Tester: nest-тестировщик (subagent)
Scope: live nested smoke — 200-cycle rapid-Tab stress + apply checkpoints, no-side-effect invariant, teardown hygiene.
Verdict: **PASS a / PASS b** (одно наблюдение для триажа оркестратором, не блокер).

## 1. Environment / pin

| Item | Fact |
|---|---|
| Host compositor | Hyprland 0.56.2, tag v0.56.2, commit `efb50993`, abiHash `efb50993…_aq_0.15_hu_0.14_hg_0.5_hc_0.1_hlg_0.6`, wl `wayland-1`, pid 1250 |
| Nest | Same binary 0.56.2, NSIG `efb50993…_1789837139_416382689`, wl `wayland-2`, pid 207749. Запущен **без** `env -u WAYLAND_DISPLAY` (nested-бэкенд — клиент host-Wayland), `XDG_CACHE_HOME=/tmp/nest-t6run/cache`, конфиг `/tmp/nest-t6run/hypr-nest.conf` (monitor-строка + 4× `exec-once = foot`) |
| Terminal | `foot` (kitty/xterm отсутствуют; задокументировано, отдельный term-поиск не понадобился) |
| Repo pin | `/home/code_warlord/Work/DEV/mru-switcher`, branch `main`, commit `1f8bdd6` |
| .so | Собран в sandbox: `cmake -S <repo> -B /tmp/nest-t6run/build -DMRU_BUILD_PLUGIN=ON -DMRU_BUILD_TESTS=OFF -DCMAKE_CXX_COMPILER=g++` → conf=0, build=0; `/tmp/nest-t6run/build/mru-switcher.so` (1.6 MB). `plugin list` в nest: `mru-switcher by mru, Version 0.4.0` |
| Sandbox | `/tmp/nest-t6run` (build, cache, hypr-nest.conf, nest.log, cmake-*.log). Чужие `/tmp/nestprobe`, `/tmp/mru-nest-*` не тронуты |

## 2. Steps (шаг / ожидание / факт / вердикт)

| # | Шаг | Ожидание | Факт | Вердикт |
|---|---|---|---|---|
| 1 | Build .so в sandbox | conf=0, build=0 | conf=0, build=0, `mru-switcher.so` на месте | PASS |
| 2 | Поднять nest по рецепту (WAYLAND_DISPLAY **не** убирать) | 2 инстанса в `hyprctl instances` | host + nest (`…_1789837139_416382689`, wayland-2) | PASS |
| 3 | 4 окна | `clients -j` = 4 | 4× `foot` (`0x…ce60/d670/e300/f240`) с первой попытки | PASS |
| 4 | `plugin load` + `plugin list` + `mru:status` | `ok`, плагин в списке | load `ok`, list показывает mru-switcher 0.4.0, status `ok` | PASS |
| 5a | **Стреес a:** 200× `dispatch mru:cycle next`, затем `mru:apply`; activewindow до/после | 0 ошибок; фокус НЕ движется во время циклов; ровно одна смена фокуса на apply; nest жив | `real 0m0.890s` (~4.5 мс/dispatch), rc=0, без FAIL; `MID_NO_APPLY == BEFORE (0x…f240)` — сайд-эффекта нет; apply `ok`; nest жив (2 инстанса) | **PASS a** |
| 5b | **Стресс b:** 200 циклов, `mru:apply` на каждой 50-й (4 apply), `mru:status` между | 0 ошибок; status `ok`; фокус меняется только на apply-чекпоинтах; nest жив | `real 0m1.094s`, без FAIL; `APPLY@50/100/150/200 status=ok`, фокус между apply неподвижен; `END=0x…3ce60`; nest жив | **PASS b** |
| 6 | No-side-effect доп-проверки: 1 цикл (фокус стоит) → apply (фокус сменился); 8 циклов без apply (фокус стоит) | Инвариант SPEC: `mru:cycle` не трогает фокус | `after-cycle focus == A0 (0x…3ce60)`, `after-apply focus = 0x…5e300 ≠ A0`; 8 циклов: `focus == BASE (0x…5e300)` | PASS |
| 7 | История реагирует на ручной фокус: `focuswindow d670` → cycle+apply | Лендинг — сосед по обновлённой истории, не исходное окно | `after-cycle+apply = 0x…5e300` (≠ d670) — трекер живой | PASS |
| 8 | Teardown: `mru:cancel`, `plugin unload`, TERM nest, `pgrep`, хост `plugin list` | nest пуст, хост чист, остался только host-композитор | cancel `ok`, unload `ok`, nest `no plugins loaded`; `pgrep` — только pid 1250; хост `no plugins loaded` | PASS |

Примечания к таймингам: стресс-a 200 dispatch за **0.890s** (user 0.451 / sys 0.439); стресс-b 200 dispatch + 4 apply за **1.094s**. Оба прогона — последовательные `hyprctl dispatch` через bash-цикл (не `--batch`), т.е. цифры включают IPC-оверхеды; ни одного `FAIL@i`.

## 3. Наблюдение для триажа (не провал, S3/none)

Связанные apply без промежуточного ручного фокуса приземляются на **одно и то же** окно: из `BASE=0x…5e300` четыре подряд сессии `cycle next → apply` дали лендинг `0x…6f240` все 4 раза, а в стрессе-b стартовое `0x…f240` после 200 циклов вернулось на `0x…f240` (200 mod 4 = 0 — арифметика снимка, не баг). Похоже, каждая новая сессия делает снимок статичного MRU-порядка и не «продвигает» историю самим фактом apply-лендинга (ручной `focuswindow` историю двигает — шаг 7 это доказывает). Возможный SPEC-вопрос: должен ли apply-лендинг сам ротировать историю для chained Alt+Tab? На реальный rapid-Tab с зажатым Alt это не влияет (там одна сессия), инвариант «cycle не трогает фокус» соблюдён. Решение — за оркестратором (уточнить SPEC/ADR или завести тикет).

`mru:status` на 0.56.2 через hyprctl возвращает только `ok` без тела (session_id не виден) — сверка велась по `activewindow -j`, как предписано планом.

## 4. Вывод

- **a: PASS** — 200 rapid-циклов за 0.89s без ошибок и без движения фокуса; apply — ровно одна смена фокуса; nest стабилен.
- **b: PASS** — 200 циклов + 4 apply-чекпоинта за 1.09s; статусы `ok`; фокус неподвижен вне apply; nest стабилен.
- Репозиторий: создан только этот файл, не коммитился, не пушился. Хост-композитор не тронут (`plugin list` чист, лишний Hyprland-процессов нет).
