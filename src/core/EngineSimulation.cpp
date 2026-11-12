#include "core/EngineSimulation.hpp"

#include <algorithm>
#include <cmath>

namespace enginewatch {

EngineSimulation::EngineSimulation() { reset(); }

float EngineSimulation::clamp(float x, float lo, float hi) {
    return x < lo ? lo : (x > hi ? hi : x);
}

void EngineSimulation::reset() {
    state_ = MachineryState{};
    state_.standby_pump.suction_open = false;
    state_.standby_pump.discharge_open = false;
    main_start_delay_ = 0.0f;
    standby_start_delay_ = 0.0f;
    pressure_alarm_delay_ = 0.0f;
    restored_pressure_duration_ = 0.0f;
    syncLegacy();
}

void EngineSimulation::syncLegacy() {
    state_.lo_pump_requested = state_.main_pump.requested;
    state_.lo_pump_running = state_.main_pump.running;
    state_.lo_pump_fault = state_.main_pump.fault;
}

void EngineSimulation::requestPump(PumpId pump, bool run) {
    PumpState& p = pump == MainLoPump ? state_.main_pump : state_.standby_pump;
    if (p.requested == run) return;
    p.requested = run;
    if (!run) p.running = false;
    if (pump == MainLoPump) main_start_delay_ = 0.0f;
    else standby_start_delay_ = 0.0f;
    syncLegacy();
}

void EngineSimulation::requestLoPump(bool run) { requestPump(MainLoPump, run); }

void EngineSimulation::setValve(PumpId pump, ValveId valve, bool open) {
    PumpState& p = pump == MainLoPump ? state_.main_pump : state_.standby_pump;
    if (valve == SuctionValve) p.suction_open = open;
    else p.discharge_open = open;
}

void EngineSimulation::setElectricalBus(bool energized) {
    state_.electrical_bus_energized = energized;
    if (!energized) {
        state_.main_pump.running = false;
        state_.standby_pump.running = false;
        main_start_delay_ = standby_start_delay_ = 0.0f;
    }
    syncLegacy();
}

void EngineSimulation::setPumpFault(PumpId pump, bool fault) {
    PumpState& p = pump == MainLoPump ? state_.main_pump : state_.standby_pump;
    p.fault = fault;
    if (fault) p.running = false;
    if (pump == MainLoPump) main_start_delay_ = 0.0f;
    else standby_start_delay_ = 0.0f;
    syncLegacy();
}

void EngineSimulation::setLoPumpFault(bool fault) { setPumpFault(MainLoPump, fault); }

bool EngineSimulation::beginMissionTwo() {
    if (!state_.objective_complete || state_.mission != 1) return false;
    state_.mission = 2;
    state_.strainer_inspected = false;
    state_.alarm_acknowledged = false;
    state_.alarm_ever_acknowledged = false;
    state_.mission_two_complete = false;
    restored_pressure_duration_ = 0.0f;
    // A developing suction-strainer restriction creates distinct indications:
    // increasing differential pressure, decreasing inlet pressure and header loss.
    return true;
}

void EngineSimulation::inspectStrainer() {
    if (state_.mission == 2 && state_.main_strainer_blockage > 0.30f)
        state_.strainer_inspected = true;
}

void EngineSimulation::acknowledgeAlarm() {
    if (state_.lo_pressure_alarm) {
        state_.alarm_acknowledged = true;
        state_.alarm_ever_acknowledged = true;
    }
}

void EngineSimulation::step(float dt_seconds) {
    if (!std::isfinite(dt_seconds) || dt_seconds <= 0.0f) return;
    const float dt = clamp(dt_seconds, 0.0f, 0.05f);
    state_.elapsed_seconds += dt;

    if (state_.mission == 2 && !state_.mission_two_complete) {
        state_.main_strainer_blockage =
            clamp(state_.main_strainer_blockage + dt * 0.14f, 0.0f, 0.90f);
    }

    PumpState* pumps[2] = {&state_.main_pump, &state_.standby_pump};
    float* delays[2] = {&main_start_delay_, &standby_start_delay_};
    for (int i = 0; i < 2; ++i) {
        PumpState& p = *pumps[i];
        if (p.requested && state_.electrical_bus_energized && !p.fault) {
            *delays[i] += dt;
            if (*delays[i] >= 0.45f) p.running = true;
        } else {
            p.running = false;
            *delays[i] = 0.0f;
        }
    }

    // Differential pressure rises only when the main pump draws through the
    // strainer. A high DP with low inlet pressure indicates a suction restriction.
    const PumpState& main = state_.main_pump;
    const PumpState& standby = state_.standby_pump;
    const bool main_intake = main.running && main.suction_open;
    state_.main_strainer_dp_bar = main_intake ?
        (0.12f + 2.15f * state_.main_strainer_blockage) : 0.0f;
    state_.main_pump.suction_pressure_bar = main.suction_open ?
        clamp(0.78f - state_.main_strainer_dp_bar * 0.29f, 0.05f, 0.78f) : 0.0f;
    state_.standby_pump.suction_pressure_bar = standby.suction_open ? 0.76f : 0.0f;

    const float main_flow = main_intake && main.discharge_open ?
        clamp(1.0f - state_.main_strainer_blockage * 0.83f, 0.0f, 1.0f) : 0.0f;
    const float standby_flow = standby.running && standby.suction_open && standby.discharge_open ? 1.0f : 0.0f;
    state_.main_pump.discharge_pressure_bar = main.running ? main_flow * 4.4f : 0.0f;
    state_.standby_pump.discharge_pressure_bar = standby.running ? standby_flow * 4.4f : 0.0f;
    const float target_bar = std::max(main_flow, standby_flow) * 4.4f;
    const float tau = target_bar >= state_.lo_pressure_bar ? 2.5f : 1.6f;
    state_.lo_pressure_bar += (target_bar - state_.lo_pressure_bar) * (1.0f - std::exp(-dt / tau));
    state_.lo_pressure_bar = clamp(state_.lo_pressure_bar, 0.0f, 4.4f);
    if (state_.lo_pressure_bar < 0.005f) state_.lo_pressure_bar = 0.0f;

    const float temp_target = target_bar > 2.5f ? 49.0f : 70.0f;
    state_.lo_oil_temp_c += (temp_target - state_.lo_oil_temp_c) * (1.0f - std::exp(-dt / 45.0f));
    const bool pressure_demand = state_.mission == 2 || main.requested || standby.requested;
    const bool pressure_low = pressure_demand && state_.lo_pressure_bar < 2.5f;
    pressure_alarm_delay_ = pressure_low ? pressure_alarm_delay_ + dt : 0.0f;
    const bool alarm_now = pressure_alarm_delay_ > 7.0f;
    if (alarm_now && !state_.lo_pressure_alarm) state_.alarm_acknowledged = false;
    state_.lo_pressure_alarm = alarm_now;

    if (state_.mission == 2) {
        const bool stable = standby.running && standby.suction_open && standby.discharge_open &&
                            state_.lo_pressure_bar >= 3.5f && !state_.lo_pressure_alarm;
        restored_pressure_duration_ = stable ? restored_pressure_duration_ + dt : 0.0f;
    }
    syncLegacy();
}

void EngineSimulation::inspectEcrConsole() {
    if (state_.mission == 1 && state_.main_pump.running &&
        state_.lo_pressure_bar >= 3.0f && state_.electrical_bus_energized &&
        !state_.lo_pressure_alarm)
        state_.objective_complete = true;
}

void EngineSimulation::verifyMissionTwo() {
    if (state_.mission != 2 || state_.mission_two_complete) return;
    const bool main_isolated = !state_.main_pump.running &&
         !state_.main_pump.suction_open && !state_.main_pump.discharge_open;
    const bool alarm_handled = !state_.lo_pressure_alarm &&
        (!state_.alarm_ever_acknowledged || state_.alarm_acknowledged);
    if (main_isolated && state_.strainer_inspected && alarm_handled &&
        restored_pressure_duration_ >= 2.0f)
        state_.mission_two_complete = true;
}

} // namespace enginewatch
