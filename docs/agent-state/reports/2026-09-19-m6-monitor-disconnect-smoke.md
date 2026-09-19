# M6-T8 monitor-disconnect storm live smoke (issue #51)

Date (UTC): 2026-09-19 ~20:40–21:00
Tester: nest-тестировщик (subagent)
Scope: live nested smoke — feasibility второго монитора в nested Hyprland 0.56.2 +
матрица дисконнекта scope=monitor сессии (доменный сценарий 5 T-H-07; REQ-S-006,
REQ-F-004/006/007).
Verdict: **PASS (3а) / PASS (4)** — второй монитор ЕСТЬ, дисконнект и позитив без краша.

## 1. Environment / pin

| Item | Fact |
|---|---|
| Host compositor | Hyprland 0.56.2, commit `efb5099`, wl `wayland-1`, pid 1250 |
| Nest | Same binary 0.56.2, NSIG `efb50993…_1789837783_806343980`, wl `wayland-2`, pid 214087. Запущен **без** `env -u WAYLAND_DISPLAY` (nested-бэкенд — клиент host-Wayland), `XDG_CACHE_HOME=/tmp/nest-t8run/cache`, конфиг `/tmp/nest-t8run/hypr-nest.conf` (monitor-строка + 4x `exec-once = foot`) |
| Terminal | `foot` |
| Repo pin | `/home/code_warlord/Work/DEV/mru-switcher`, branch `main`, commit `07805ad` |
| .so | Собран в sandbox: `cmake -S <repo> -B /tmp/nest-t8run/build -DMRU_BUILD_PLUGIN=ON -DMRU_BUILD_TESTS=OFF -DCMAKE_CXX_COMPILER=g++` → conf=0, build=0; `/tmp/nest-t8run/build/mru-switcher.so` (1.6 MB). `plugin list`: mru-switcher 0.4.0 |
| Sandbox | `/tmp/nest-t8run` (build, cache, hypr-nest[-1mon].conf, nest.log, cmake/build.log, aw.json, aft.json). Чужие sandbox'ы не тронуты |

## 2. Feasibility второго монитора: ЕСТЬ (доказательство)

Исходное состояние: `monitors -j` → ровно 1 (`WAYLAND-1`, ws1), 4x `foot`
(`0x…35f0/3c00/49c0/54d0`, все ws1 mon=0).

1. **monitor-строки в конфиге НЕ работают**: `monitor = HEADLESS-1,1280x720@60,auto,1`
   (+ `WAYLAND-2`, `DP-99`, `headless-1`, `HEADLESS-2`) + `hyprctl reload` →
   `monitors` остался 1 (`WAYLAND-1`), в `nest.log` ни строки про HEADLESS.
   Nested-бэкенд aquamarine 0.15.0 держит один Wayland-surface; второй выход через
   `monitor=` не поднимается.
2. **Dispatcher `hyprctl output create headless` РАБОТАЕТ** (синтаксис:
   `output <create <backend: wayland|x11|headless|auto> | remove <name>>`):
   `output create headless` → `ok` → `monitors -j`: **count=2**
   (`0 WAYLAND-1` ws1 @0,0 + `1 HEADLESS-1` ws2 @1920,0, оба `disabled=false`).
   `workspaces -j`: `1 1 WAYLAND-1`, `2 2 HEADLESS-1`.
3. `focusmonitor HEADLESS-1` → `ok`; 2x `exec foot` → 2 окна на ws2 mon=1
   (`0x…1840`, `0x…7be0`; всего 6: 4 на mon=0 + 2 на mon=1).

Итог: **2 монитора возможны**, но только через runtime-диспетчер `output create`,
не через `monitor=` в конфиге. Выполнена полная матрица 3а, fallback 3b не понадобился.

## 3. Steps (шаг / ожидание / факт / вердикт)

| # | Шаг | Ожидание | Факт | Вердикт |
|---|---|---|---|---|
| 1 | Поднять nest, 4 окна, load .so | 2 инстанса; `clients -j` = 4; load `ok`, плагин в списке | host + nest (wayland-2); 4x `foot` (`0x…35f0/3c00/49c0/54d0`); `plugin load ok`, list — mru-switcher 0.4.0, `mru:status ok` | PASS |
| 2 | Feasibility: HEADLESS-1 через конфиг+reload (5 имён), затем `output create headless` | Честный факт есть/нет с доказательством | Конфиг-путь: 5/5 попыток → monitors=1 (не работает). `output create headless` → `ok`, monitors=2 (`WAYLAND-1` + `HEADLESS-1`, ws2); 2 новых foot на mon=1 (`0x…1840/7be0`) | **2 монитора ЕСТЬ (через output create)** |
| 3а | MRU-seed на HEADLESS-1 (лесенка A→B→A, sleep 0.7 > debounce 400ms); `mru:cycle next monitor` (сессия открыта, фокус неподвижен); **модель дисконнекта: `output remove HEADLESS-1`**; `mru:apply` | Корректный конец mid-session инвалидации без краша: `apply ok`, nest и плагин живы, фокус ровно один раз (MRU-сосед) | `BEF=0x…1840` (A), `MID=0x…1840` (cycle без сайд-эффекта); `output remove HEADLESS-1 ok` → monitors=1, все 6 окон пережили (A/B на ws2 mon=0 — компоновщик мигрировал окна на WAYLAND-1); nest pid 214087 жив, плагин в списке; `apply ok` (rc=0) → `AFT=0x…7be0` (B ≠ BEF — ровно одна смена фокуса); `status ok`, `cancel ok`; `grep crash|segfault|SIGSEGV nest.log` → 0 | **PASS** |
| 4 | scope=monitor позитив: окна живы (6), `mru:cycle next monitor` → `mru:apply` | cycle не двигает фокус; apply — ровно одна смена фокуса; nest жив | `BEF=0x…7be0`, `MID=0x…7be0` (SAME); `apply ok` → `AFT=0x…1840` (≠ BEF, mon=0 ws=2); `cancel ok`; оба Hyprland живы; crash-хитов 0 | **PASS** |
| 5 | Teardown | `mru:cancel`, `plugin unload`, TERM nest, `pgrep`-контроль, хост `plugin list` чист | cancel `ok`, unload `ok`, `plugin list` = `no plugins loaded`; TERM 214087 → `pgrep` только pid 1250; `instances -j` только host (wayland-1); хост `plugin list` = `no plugins loaded` | PASS |

Примечания:

- Модель дисконнекта — `hyprctl output remove HEADLESS-1` (настоящий unplug
  виртуального выхода mid-session), не kill окон: компоновщик при этом **мигрировал
  оба окна ws2 на WAYLAND-1** (clients=6 до и после) — окна-призраки на мёртвом
  мониторе на этом пине не воспроизводятся, окна переживают дисконнект. Сессия
  scope=monitor корректно завершилась через apply с ровно одним фокусом на
  MRU-соседа; краша/залипания нет.
- `hyprctl dispatch` на 0.56.2 печатает только `ok` без тела `SDispatchResult` —
  ветку `no windows` / `last_end` вживую различить нельзя; graceful-end подтверждён
  косвенно (nest жив, плагин в списке, `status`/`cancel ok`, фокус сменился ровно
  один раз). Задокументированное ограничение хоста (SPEC §3.4, `docs/COMPAT.md`).
- Ложный `Command exited with code 1` в середине прогона — это `grep -c` с 0
  совпадений (exit 1 при нуле), не ошибка матрицы; перепроверено → 0.

## 4. Вывод

- **Feasibility: 2 монитора ЕСТЬ** — через `hyprctl output create headless`
  (count=2, `WAYLAND-1` + `HEADLESS-1`, окна на обоих). Через `monitor=` в конфиге
  второй выход НЕ поднимается (5/5 имён → monitors=1) — задокументировать в COMPAT.
- **Шаг 3а (дисконнект): PASS** — scope=monitor сессия + `output remove`
  mid-session → `apply ok`, ровно один фокус на MRU-соседа, nest и плагин живы.
- **Шаг 4 (позитив): PASS** — cycle без сайд-эффекта (BEF==MID), apply ровно одна
  смена фокуса, nest стабилен.
- Репозиторий: создан только этот файл, не коммитился, не пушился. Хост не тронут.
