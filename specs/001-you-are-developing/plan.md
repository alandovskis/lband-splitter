# Implementation Plan: Ethernet-Controllable L-Band RF Splitter/Combiner

**Branch**: `001-you-are-developing` | **Date**: 2025-09-07 | **Spec**: [spec.md](./spec.md)
**Input**: Feature specification from `/Users/alex/src/splitter/specs/001-you-are-developing/spec.md`

## Execution Flow (/plan command scope)
```
1. Load feature spec from Input path
   → If not found: ERROR "No feature spec at {path}"
2. Fill Technical Context (scan for NEEDS CLARIFICATION)
   → Detect Project Type from context (web=frontend+backend, mobile=app+api)
   → Set Structure Decision based on project type
3. Evaluate Constitution Check section below
   → If violations exist: Document in Complexity Tracking
   → If no justification possible: ERROR "Simplify approach first"
   → Update Progress Tracking: Initial Constitution Check
4. Execute Phase 0 → research.md
   → If NEEDS CLARIFICATION remain: ERROR "Resolve unknowns"
5. Execute Phase 1 → contracts, data-model.md, quickstart.md, agent-specific template file (e.g., `CLAUDE.md` for Claude Code, `.github/copilot-instructions.md` for GitHub Copilot, or `GEMINI.md` for Gemini CLI).
6. Re-evaluate Constitution Check section
   → If new violations: Refactor design, return to Phase 1
   → Update Progress Tracking: Post-Design Constitution Check
7. Plan Phase 2 → Describe task generation approach (DO NOT create tasks.md)
8. STOP - Ready for /tasks command
```

**IMPORTANT**: The /plan command STOPS at step 7. Phases 2-4 are executed by other commands:
- Phase 2: /tasks command creates tasks.md
- Phase 3-4: Implementation execution (manual or via tools)

## Summary
Develop an Ethernet-controllable L-Band RF splitter/combiner system supporting 16 F-type combiner inputs, 16 F-type splitter outputs, and 2 SMA connectors. The system provides real-time signal monitoring (frequency, power, SNR, modcod) through CLI, web interface, and netconf protocol, with dual power supply redundancy for high availability.

## Technical Context
**Language/Version**: C++ 17/20 for backend services, Angular 17+ for web interface, STM32 bare-metal C for firmware  
**Primary Dependencies**: libwebsockets, CMSIS-DSP, Angular Material, libnetconf2  
**Storage**: Local configuration files, external measurement data storage via REST API  
**Testing**: Google Test (gtest) for C++, Jasmine/Karma for Angular, hardware-in-the-loop for firmware  
**Target Platform**: Linux embedded system (ARM/x86), STM32F4 microcontroller
**Project Type**: web - backend + frontend + firmware  
**Performance Goals**: 60Hz measurement updates, <100ms response time for control commands, real-time signal processing  
**Constraints**: L-Band frequency range 950-2450 MHz, signal integrity per FR-014, dual power supply support  
**Scale/Scope**: 34 total RF ports (32 F-type + 2 SMA), single operator interface, embedded system deployment

## Constitution Check
*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

**Simplicity**:
- Projects: 3 (backend C++ services, frontend Angular, firmware STM32) ✓
- Using framework directly? YES (libwebsockets, Angular Material, CMSIS-DSP) ✓
- Single data model? YES (RF port measurements, system status) ✓
- Avoiding patterns? YES (direct hardware access, no unnecessary abstractions) ✓

**Architecture**:
- EVERY feature as library? YES (RF measurement lib, network lib, power management lib) ✓
- Libraries listed: rf-measurement (signal processing), network-control (CLI/web/netconf), power-management (redundancy) ✓
- CLI per library: rf-control --help, net-config --help, power-status --help ✓
- Library docs: llms.txt format planned? YES ✓

**Testing (NON-NEGOTIABLE)**:
- RED-GREEN-Refactor cycle enforced? YES ✓
- Git commits show tests before implementation? YES ✓
- Order: Contract→Integration→E2E→Unit strictly followed? YES ✓
- Real dependencies used? YES (actual hardware, network interfaces) ✓
- Integration tests for: RF hardware, network protocols, power switching ✓
- FORBIDDEN: Implementation before test, skipping RED phase ✓

**Observability**:
- Structured logging included? YES (JSON format for all components) ✓
- Frontend logs → backend? YES (unified logging stream) ✓
- Error context sufficient? YES (hardware state, signal measurements) ✓

**Versioning**:
- Version number assigned? YES (0.1.0) ✓
- BUILD increments on every change? YES ✓
- Breaking changes handled? YES (API versioning, backward compatibility) ✓

## Project Structure

### Documentation (this feature)
```
specs/[###-feature]/
├── plan.md              # This file (/plan command output)
├── research.md          # Phase 0 output (/plan command)
├── data-model.md        # Phase 1 output (/plan command)
├── quickstart.md        # Phase 1 output (/plan command)
├── contracts/           # Phase 1 output (/plan command)
└── tasks.md             # Phase 2 output (/tasks command - NOT created by /plan)
```

### Source Code (repository root)
```
# Option 1: Single project (DEFAULT)
src/
├── models/
├── services/
├── cli/
└── lib/

tests/
├── contract/
├── integration/
└── unit/

# Option 2: Web application (when "frontend" + "backend" detected)
backend/
├── src/
│   ├── models/
│   ├── services/
│   └── api/
└── tests/

frontend/
├── src/
│   ├── components/
│   ├── pages/
│   └── services/
└── tests/

# Option 3: Mobile + API (when "iOS/Android" detected)
api/
└── [same as backend above]

ios/ or android/
└── [platform-specific structure]
```

**Structure Decision**: Option 2: Web application (backend C++ services + Angular frontend + STM32 firmware)

## Phase 0: Outline & Research
1. **Extract unknowns from Technical Context** above:
   - For each NEEDS CLARIFICATION → research task
   - For each dependency → best practices task
   - For each integration → patterns task

2. **Generate and dispatch research agents**:
   ```
   For each unknown in Technical Context:
     Task: "Research {unknown} for {feature context}"
   For each technology choice:
     Task: "Find best practices for {tech} in {domain}"
   ```

3. **Consolidate findings** in `research.md` using format:
   - Decision: [what was chosen]
   - Rationale: [why chosen]
   - Alternatives considered: [what else evaluated]

**Output**: research.md with all NEEDS CLARIFICATION resolved

## Phase 1: Design & Contracts
*Prerequisites: research.md complete*

1. **Extract entities from feature spec** → `data-model.md`:
   - Entity name, fields, relationships
   - Validation rules from requirements
   - State transitions if applicable

2. **Generate API contracts** from functional requirements:
   - For each user action → endpoint
   - Use standard REST/GraphQL patterns
   - Output OpenAPI/GraphQL schema to `/contracts/`

3. **Generate contract tests** from contracts:
   - One test file per endpoint
   - Assert request/response schemas
   - Tests must fail (no implementation yet)

4. **Extract test scenarios** from user stories:
   - Each story → integration test scenario
   - Quickstart test = story validation steps

5. **Update agent file incrementally** (O(1) operation):
   - Run `/scripts/update-agent-context.sh [claude|gemini|copilot]` for your AI assistant
   - If exists: Add only NEW tech from current plan
   - Preserve manual additions between markers
   - Update recent changes (keep last 3)
   - Keep under 150 lines for token efficiency
   - Output to repository root

**Output**: data-model.md, /contracts/*, failing tests, quickstart.md, agent-specific file

## Phase 2: Task Planning Approach
*This section describes what the /tasks command will do - DO NOT execute during /plan*

**Task Generation Strategy**:
Based on design artifacts created in Phase 1:

**From contracts/rest-api.yaml**:
- REST endpoint contract tests for system status, ports, power, configuration APIs
- WebSocket connection and message contract tests 
- API integration tests for all CRUD operations

**From data-model.md entities**:
- RFPort model class with validation rules (frequency range, power limits)
- PowerSupply model class with redundancy logic
- SystemConfiguration model class with persistence
- SignalMeasurement time-series data handling
- ControlSession management for multi-interface access

**From quickstart.md test scenarios**:
- RF signal combining integration test (Scenario 1)
- RF signal splitting integration test (Scenario 2) 
- Multi-interface control integration test (Scenario 3)
- Power supply redundancy integration test (Scenario 4)
- Signal quality monitoring integration test (Scenario 5)
- Network resilience integration test (Scenario 6)

**Task Structure**:
1. **Contract Tests** [P]: One per REST endpoint + WebSocket API
2. **Model Tests** [P]: Core data entities with validation 
3. **Library Tests**: rf-measurement, network-control, power-management
4. **Integration Tests**: Hardware interaction, network protocols
5. **Implementation Tasks**: Make failing tests pass using TDD
6. **Frontend Tasks**: Angular components consuming backend APIs
7. **Firmware Tasks**: STM32 signal processing and hardware control

**Ordering Strategy**:
- **Phase A** [P]: Contract tests (can run in parallel)
- **Phase B**: Model implementation → Library implementation  
- **Phase C**: Integration tests → Backend services
- **Phase D**: Frontend components → E2E tests
- **Phase E**: STM32 firmware → Hardware integration
- **Phase F**: System validation using quickstart.md scenarios

**Specific Task Categories**:
- **Backend C++**: 12-15 tasks (models, services, REST API, WebSocket)
- **Frontend Angular**: 8-10 tasks (components, services, real-time updates)
- **STM32 Firmware**: 6-8 tasks (ADC, DSP, communication, power management)
- **Integration**: 8-10 tasks (hardware tests, network tests, system tests)

**Estimated Output**: 35-40 numbered, ordered tasks in tasks.md

**IMPORTANT**: This phase is executed by the /tasks command, NOT by /plan

## Phase 3+: Future Implementation
*These phases are beyond the scope of the /plan command*

**Phase 3**: Task execution (/tasks command creates tasks.md)  
**Phase 4**: Implementation (execute tasks.md following constitutional principles)  
**Phase 5**: Validation (run tests, execute quickstart.md, performance validation)

## Complexity Tracking
*Fill ONLY if Constitution Check has violations that must be justified*

| Violation | Why Needed | Simpler Alternative Rejected Because |
|-----------|------------|-------------------------------------|
| [e.g., 4th project] | [current need] | [why 3 projects insufficient] |
| [e.g., Repository pattern] | [specific problem] | [why direct DB access insufficient] |


## Progress Tracking
*This checklist is updated during execution flow*

**Phase Status**:
- [x] Phase 0: Research complete (/plan command)
- [x] Phase 1: Design complete (/plan command)
- [x] Phase 2: Task planning complete (/plan command - describe approach only)
- [x] Phase 3: Tasks generated (/tasks command)
- [ ] Phase 4: Implementation complete
- [ ] Phase 5: Validation passed

**Gate Status**:
- [x] Initial Constitution Check: PASS
- [x] Post-Design Constitution Check: PASS
- [x] All NEEDS CLARIFICATION resolved
- [x] Complexity deviations documented (none required)

---
*Based on Constitution v2.1.1 - See `/memory/constitution.md`*