#pragma once

namespace enginewatch {

// Educational process model. Values are deliberately illustrative; they must
// not be used for operation or training on a specific manufacturer's equipment.
enum PumpId { MainLoPump = 0, StandbyLoPump = 1 };
enum ValveId { SuctionValve = 0, DischargeValve = 1 };

struct PumpState {
    bool requested = false;
    bool running = false;
    bool fault = false;
    bool suction_open = true;
    bool discharge_open = true;
    float suction_pressure_bar = 0.75f;
    float discharge_pressure_bar = 0.0f;
};

struct MachineryState {
    bool electrical_bus_energized = true;

    PumpState main_pump;
    PumpState standby_pump;
    float main_strainer_dp_bar = 0.0f;
    float main_strainer_blockage = 0.0f; // 0 clear, 1 severely restricted
    float lo_oil_temp_c = 49.0f;
    float lo_pressure_bar = 0.0f;        // Common LO distribution header
    bool lo_pressure_alarm = false;
    bool alarm_acknowledged = false;
    bool alarm_ever_acknowledged = false;
    bool strainer_inspected = false;
    int mission = 1;
    bool objective_complete = false;   // Mission 001 (preserved on advancing)
    bool mission_two_complete = false;
    float elapsed_seconds = 0.0f;

    // Original v0.1 public fields remain mirrors for compatibility with tests.
    bool lo_pump_requested = false;
    bool lo_pump_running = false;
    bool lo_pump_fault = false;
};

class EngineSimulation {
public:
    EngineSimulation();
    void reset();
    void step(float dt_seconds);
    void requestLoPump(bool run); // v0.1 alias for the main pump
    void requestPump(PumpId pump, bool run);
    void setValve(PumpId pump, ValveId valve, bool open);
    void setElectricalBus(bool energized);
    void setLoPumpFault(bool fault); // v0.1 fault injection for the main pump
    void setPumpFault(PumpId pump, bool fault);
    void inspectEcrConsole(); // Mission 001 explicit verification
    bool beginMissionTwo();
    void inspectStrainer();
    void acknowledgeAlarm();
    void verifyMissionTwo();
    const MachineryState& state() const { return state_; }

private:
    MachineryState state_;
    float main_start_delay_ = 0.0f;
    float standby_start_delay_ = 0.0f;
    float pressure_alarm_delay_ = 0.0f;
    float restored_pressure_duration_ = 0.0f;
    void syncLegacy();
    static float clamp(float x, float lo, float hi);
};

} // namespace enginewatch
