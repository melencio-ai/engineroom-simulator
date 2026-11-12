#pragma once

namespace enginewatch {

// This is an educational toy model, not a shipboard operating reference.
struct MachineryState {
    bool electrical_bus_energized = true;
    bool lo_pump_requested = false;
    bool lo_pump_running = false;
    bool lo_pump_fault = false;
    bool lo_pressure_alarm = false;
    bool objective_complete = false;
    float lo_pressure_bar = 0.0f;
    float elapsed_seconds = 0.0f;
};

class EngineSimulation {
public:
    EngineSimulation();
    void reset();
    void step(float dt_seconds);
    void requestLoPump(bool run);
    void setElectricalBus(bool energized); // Fault-injection hook for tests and later missions.
    void setLoPumpFault(bool fault);        // Fault-injection hook for tests and later missions.
    void inspectEcrConsole();
    const MachineryState& state() const { return state_; }

private:
    MachineryState state_;
    float start_delay_seconds_ = 0.0f;
    float pressure_alarm_delay_seconds_ = 0.0f;
};

} // namespace enginewatch
