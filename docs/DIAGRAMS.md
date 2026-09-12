# MRU Window Switcher — Diagrams

Standalone sequence and structure diagrams.  
Also embedded in `ARCHITECTURE.md`. Render with any Mermaid-compatible viewer (GitHub, GitLab, VS Code, mermaid.live).

---

## 1. Component overview

```mermaid
flowchart TB
    subgraph Facade["Plugin Facade"]
        INIT["PLUGIN_INIT / EXIT"]
        DISP["Dispatchers<br/>mru:cycle / apply / cancel"]
        EV["Event::bus listeners"]
    end

    subgraph App["Application"]
        SC["SessionController"]
        HT["HistoryTracker"]
        SR["ScopeResolver"]
        FG["FocusGateway"]
    end

    subgraph Domain["Domain (pure)"]
        SNAP["Snapshot"]
        SEL["Selection"]
        POL["SessionPolicy"]
        WREF["WindowRef"]
    end

    subgraph Adapters["Adapters"]
        COMP["CompositorPort<br/>(Hyprland)"]
        CFG["ConfigPort"]
        UI["UIPort<br/>Null | Border | External"]
    end

    INIT --> SC
    DISP --> SC
    EV --> HT
    EV --> SC

    SC --> HT
    SC --> SR
    SC --> FG
    SC --> UI
    SC --> SNAP
    SC --> SEL

    HT --> WREF
    SR --> COMP
    FG --> COMP
    CFG --> SC
```

---

## 2. Session lifecycle

```mermaid
sequenceDiagram
    actor User
    participant KB as KeybindManager
    participant Fac as Plugin Facade
    participant SC as SessionController
    participant Hist as HistoryTracker
    participant Scope as ScopeResolver
    participant UI as UIPort
    participant FG as FocusGateway
    participant HL as Hyprland

    User->>KB: Alt+Tab (first)
    KB->>Fac: mru:cycle next
    Fac->>SC: Cycle(Next)
    SC->>SC: state == Idle?
    SC->>Scope: resolve(default_scope)
    Scope->>HL: list windows / history
    Scope-->>SC: candidates
    SC->>SC: build Snapshot + Selection (start_offset)
    SC->>UI: on_session_start(snapshot, selection)
    SC-->>Fac: ok

    User->>KB: Alt+Tab (again)
    KB->>Fac: mru:cycle next
    Fac->>SC: Cycle(Next)
    SC->>SC: advance Selection.index
    SC->>UI: on_selection_changed(selection)
    SC-->>Fac: ok

    User->>KB: release Alt (bindrt)
    KB->>Fac: mru:apply
    Fac->>SC: Apply
    SC->>FG: focus(selected)
    FG->>HL: fullWindowFocus / focusWindow
    SC->>UI: on_session_end(Applied)
    SC->>SC: state = Idle
    SC->>Hist: unlock (allow updates again)
```

---

## 3. Focus path and history lock-in

```mermaid
sequenceDiagram
    participant HL as Hyprland
    participant Bus as Event::bus
    participant Fac as Plugin Facade
    participant Hist as HistoryTracker
    participant SC as SessionController

    HL->>Bus: window.active(window, reason)
    Bus->>Fac: listener
    Fac->>SC: is session Active?
    alt Session Active (lock-in)
        SC-->>Hist: ignore focus for history
    else Session Idle
        Fac->>Hist: onFocus(window, reason)
        Hist->>Hist: reset/restart debounce timer
        Note over Hist: after debounce_ms
        Hist->>Hist: commit window to MRU list
    end
```

---

## 4. Cancel path

```mermaid
sequenceDiagram
    actor User
    participant KB as KeybindManager
    participant Fac as Plugin Facade
    participant SC as SessionController
    participant UI as UIPort
    participant Hist as HistoryTracker

    User->>KB: Escape (during session)
    KB->>Fac: mru:cancel
    Fac->>SC: Cancel
    SC->>UI: on_session_end(Cancelled)
    SC->>SC: state = Idle, clear snapshot
    SC->>Hist: unlock
    Note over SC: optional restore_focus_on_cancel
```

---

## 5. State machine

```mermaid
stateDiagram-v2
    [*] --> Idle
    Idle --> Active: Cycle (first)
    Active --> Active: Cycle (next/prev)
    Active --> Idle: Apply
    Active --> Idle: Cancel
    Active --> Idle: Snapshot empty / timeout
```

---

## 6. PlantUML — session lifecycle (alternative)

```plantuml
@startuml
actor User
participant "KeybindManager" as KB
participant "Facade" as Fac
participant "SessionController" as SC
participant "ScopeResolver" as Scope
participant "UIPort" as UI
participant "FocusGateway" as FG
participant "Hyprland" as HL

User -> KB: Alt+Tab
KB -> Fac: mru:cycle next
Fac -> SC: Cycle(Next)
SC -> Scope: resolve(scope)
Scope -> HL: windows / history
Scope --> SC: candidates
SC -> SC: Snapshot + Selection
SC -> UI: on_session_start
SC --> Fac: ok

User -> KB: Alt+Tab
KB -> Fac: mru:cycle next
Fac -> SC: Cycle(Next)
SC -> SC: index++
SC -> UI: on_selection_changed

User -> KB: release Alt
KB -> Fac: mru:apply
Fac -> SC: Apply
SC -> FG: focus(selected)
FG -> HL: focusWindow / fullWindowFocus
SC -> UI: on_session_end(Applied)
SC -> SC: Idle
@enduml
```

---

## 7. Data flow on first cycle

```mermaid
flowchart LR
    A[mru:cycle next] --> B{Session Idle?}
    B -->|yes| C[ScopeResolver]
    C --> D[CompositorPort / History]
    D --> E[candidates]
    E --> F[Snapshot + Selection]
    F --> G[UI on_session_start]
    F --> H[Session = Active]
    B -->|no| I[advance index]
    I --> J[UI on_selection_changed]
```
