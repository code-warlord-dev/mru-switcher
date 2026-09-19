# M6-T7 window-close storm live smoke (issue #53)

Date (UTC): 2026-09-19 ~20:15–20:35
Tester: nest-тестировщик (subagent)
Scope: live nested smoke — close-storm матрица: убить 1 окно / все кроме одного / все (SPEC §2.8, §2.6, REQ-S-006, REQ-F-004/006/007).
Verdict: **PASS 2 / PASS 3 / PASS 4 / PASS 5** (два наблюдения S3/none для триажа, не блокеры).

## 1. Environment / pin

| Item | Fact |
|---|---|
| Host compositor | Hyprland 0.56.2, commit `efb5099`, wl `wayland-1`, pid 1250 |
| Nest | Same binary 0.56.2, NSIG `efb50993…_1789837458_852528719`, wl `wayland-2`, pid 210312. Запущен **без** `env -u WAYLAND_DISPLAY` (nested-бэкенд — клиент host-Wayland), `XDG_CACHE_HOME=/tmp/nest-t7run/cache`, конфиг `/tmp/nest-t7run/hypr-nest.conf` (monitor-строка + 4× `exec-once = foot`) |
| Terminal | `foot` |
| Repo pin | `/home/code_warlord/Work/DEV/mru-switcher`, branch `main`, commit `1f8bdd6` |
| .so | Собран в sandbox: `cmake -S <repo> -B /tmp/nest-t7run/build -DMRU_BUILD_PLUGIN=ON -DMRU_BUILD_TESTS=OFF -DCMAKE_CXX_COMPILER=g++` → conf=0, build=0; `/tmp/nest-t7run/build/mru-switcher.so`. `plugin list` в nest: `mru-switcher by mru, Version 0.4.0` |
| Sandbox | `/tmp/nest-t7run` (build, cache, hypr-nest.conf, nest.log, cmake.log). Хост не тронут |
| MRU-seed | Перед каждой матричной сессией — контролируемая focus-лесенка (`focuswindow address:…` + sleep 0.7 > `debounce_ms` 400) до известного активного окна; одиночный `mru:cycle next` при дефолте `start_offset=second` приземляет apply на MRU-соседа (проверено зондом: `BEF=0x…a90 → apply → 0x…8080`) |

## 2. Steps (шаг / ожидание / факт / вердикт)

| # | Шаг | Ожидание | Факт | Вердикт |
|---|---|---|---|---|
| 1 | Поднять nest, 4 окна, load .so | 2 инстанса; `clients -j` = 4; load `ok`, плагин в списке | host + nest (wayland-2); 4× `foot` с первой попытки (`0x…a90/8080/8b40/9900`); load `ok`, list показывает mru-switcher 0.4.0, `mru:status` `ok` | PASS |
| 2 | Сессия `mru:cycle next`, убить ОДНО окно из середины (C=`0x…8b40`, не selection), `mru:apply` | Фокус ровно один раз (на соседа по снимку), сессия завершилась, композитор жив, плагин в списке | `BEF=0x…a90`, mid-cycle фокус неподвижен (`0x…a90`); kill C `ok` → clients=3; `apply ok` → `AFT=0x…8080` (MRU-сосед, ≠ BEF — ровно одна смена фокуса); `pgrep` — оба Hyprland живы; `plugin list` — mru-switcher на месте | **PASS** |
| 3 | Новая сессия, убить ВСЕ кроме одного (D=`0x…9900`, B=`0x…8080`; выжил A=`0x…a90`), `mru:cycle` → `apply` | Фокус на выжившем (SPEC §2.8 шаг 3: prune → clamp → focus → Applied) | После убийств clients=1 (`0x…a90`); `cycle ok` → `apply ok` → `after: 0x…a90`; nest жив (оба pid), последующий `mru:cancel ok` (сессия корректно закрыта, не залипла) | **PASS** |
| 4 | 3 окна (A + 2 свежих foot), сессия, закрыть ВСЕ mid-session → `apply`/`cancel` | Корректный конец без краша: `mru:status ok`, nest жив (SPEC §2.8 шаг 2 / REQ-S-006: Cancelled, `no windows`) | kill ×3 `ok` → clients=0; `apply ok` (rc=0), `status ok`, `cancel ok`, `status ok`; `pgrep` — nest pid 210312 жив; `plugin list` — mru-switcher на месте | **PASS** |
| 5 | Снова `exec foot` ×3, финальная сессия cycle→apply | PASS: cycle не двигает фокус (REQ-F-003), apply — ровно одна смена фокуса | 3 окна (`0x…a610/9900/56a0`); `BEF=0x…56a0`, `MID=0x…56a0` (cycle без сайд-эффекта), `apply ok` → `AFT=0x…9900` (≠ BEF); `status ok`; nest жив | **PASS** |
| 6 | Teardown | `mru:cancel`, `plugin unload`, TERM nest, `pgrep`-контроль, хост `plugin list` чист | cancel `ok`, unload `ok`, TERM 210312 → `pgrep` только pid 1250; `instances -j` только host; хост `plugin list` = `no plugins loaded` | PASS |

## 3. Наблюдения для триажа (не провалы, S3/none)

1. **`hyprctl dispatch` на 0.56.2 печатает только `ok`** и не показывает тело `SDispatchResult` — ветку `no windows` (§2.8 шаг 2 / REQ-DISP-002) вживую различить нельзя; graceful-end шага 4 подтверждён косвенно (nest жив, плагин в списке, `status`/`cancel` `ok`, сессия не залипла). Это задокументированное ограничение хоста (SPEC §3.4, `docs/COMPAT.md`), не gap плагина. Точная причина конца сессии (`last_end=NoWindows`) видна только через libwayland-биндинг диспетчера или лог плагина.
2. **Шаг 3 приземлился на уже-сфокусированного выжившего** (A был активен до убийств и остался единственным окном): observe «фокус на выжившем» выполнен, но различить «фокус-вызов на A» vs «no-op» через `activewindow` нельзя — по SPEC §2.8 шаг 3 вызов FocusGateway на разрешённое окно и конец `Applied` ожидаются в обоих случаях. Дискриминация — на уровне unit-тестов (§2.8 T-F-*).

## 4. Вывод

- **Шаг 2: PASS** — kill одного не-выбранного окна mid-session → apply ровно один фокус на MRU-соседа, без краша.
- **Шаг 3: PASS** — kill всех кроме одного → cycle+apply, фокус на выжившем, сессия закрыта.
- **Шаг 4: PASS** — kill всех mid-session → apply/cancel без краша, `mru:status ok`, nest и плагин живы.
- **Шаг 5: PASS** — свежие 3 окна, cycle без сайд-эффекта, apply ровно одна смена фокуса.
- Репозиторий: создан только этот файл, не коммитился, не пушился. Хост-композитор не тронут (`plugin list` чист, лишних Hyprland-процессов нет).
