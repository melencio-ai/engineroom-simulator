#include "core/EngineSimulation.hpp"

#include <cmath>

namespace enginewatch {

EngineSimulation::EngineSimulation() { reset(); }

void EngineSimulation::reset() {
    state_ = MachineryState{};
    start_delay_seconds_ = 0.0f;
    pressure_alarm_delay_seconds_ = 0.0f;
}

void EngineSimulation::requestLoPump(bool run) {
    if (state_.lo_pump_requested == run) return;
    state_.lo_pump_requested = run;
    start_delay_seconds_ = 0.0f;
    pressure_alarm_delay_seconds_ = 0.0f;
    if (!run) state_.lo_pump_running = false;
}

void EngineSimulation::setElectricalBus(bool energized) {
    state_.electrical_bus_energized = energized;
    if (!energized) {
        state_.lo_pump_running = false;
        start_delay_seconds_ = 0.0f;
    }
}

void EngineSimulation::setLoPumpFault(bool fault) {
    state_.lo_pump_fault = fault;
    if (fault) {
        state_.lo_pump_running = false;
        start_delay_seconds_ = 0.0f;
    }
}

void EngineSimulation::step(float dt_seconds) {
    // Fixed upper bound prevents sudden jumps after suspension or backgrounding.
    if (!std::isfinite(dt_seconds) || dt_seconds <= 0.0f) return;
    const float dt = dt_seconds > 0.05f ? 0.05f : dt_seconds;
    state_.elapsed_seconds += dt;

    if (state_.lo_pump_requested && state_.electrical_bus_energized && !state_.lo_pump_fault) {
        start_delay_seconds_ += dt;
        if (start_delay_seconds_ >= 0.45f) state_.lo_pump_running = true;
    } else {
        state_.lo_pump_running = false;
        start_delay_seconds_ = 0.0f;
    }

    // Illustrative first-order dynamic system: pressure approaches a new equilibrium
    // gradually, rather than jumping when a switch changes state.
    const float target = state_.lo_pump_running ? 4.4f : 0.0f;
    const float tau = state_.lo_pump_running ? 2.5f : 1.6f;
    const float gain = 1.0f - std::exp(-dt / tau);
    state_.lo_pressure_bar += (target - state_.lo_pressure_bar) * gain;
    if (state_.lo_pressure_bar < 0.005f) state_.lo_pressure_bar = 0.0f;

    // Start-up low pressure is tolerated briefly. Alarm remains live until
    // pressure recovers, and clears after switching the pump back off.
    const bool demand_low = state_.lo_pump_requested && state_.lo_pressure_bar < 2.5f;
    if (demand_low) {
        pressure_alarm_delay_seconds_ += dt;
    } else {
        pressure_alarm_delay_seconds_ = 0.0f;
    }
    state_.lo_pressure_alarm = demand_low && pressure_alarm_delay_seconds_ > 7.0f;
}

void EngineSimulation::inspectEcrConsole() {
    // Mission 001 is only complete once the system is actually verified in ECR.
    if (state_.lo_pump_running && state_.lo_pressure_bar >= 3.0f &&
        state_.electrical_bus_energized && !state_.lo_pressure_alarm) {
        state_.objective_complete = true;
    }
}

} // namespace enginewatch
