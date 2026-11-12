#include "core/EngineSimulation.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>

using enginewatch::EngineSimulation;

static void require(bool pass, const char* message) {
    if (!pass) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}

static void advance(EngineSimulation& sim, float seconds) {
    const int ticks = static_cast<int>(seconds / 0.025f);
    for (int i = 0; i < ticks; ++i) sim.step(0.025f);
}

static void test_start_and_pressure() {
    EngineSimulation sim;
    require(sim.state().electrical_bus_energized, "initial bus is energized");
    require(sim.state().lo_pressure_bar == 0.0f, "initial pressure zero");
    sim.requestLoPump(true);
    advance(sim, 0.20f);
    require(!sim.state().lo_pump_running, "start delay is honored");
    advance(sim, 0.50f);
    require(sim.state().lo_pump_running, "pump starts");
    advance(sim, 6.0f);
    require(sim.state().lo_pressure_bar > 3.0f, "pressure builds above target");
    require(sim.state().lo_pressure_bar < 4.5f, "pressure stays bounded");
    require(!sim.state().lo_pressure_alarm, "normal pump start has no lasting alarm");
}

static void test_stop_and_decay() {
    EngineSimulation sim;
    sim.requestLoPump(true);
    advance(sim, 10.0f);
    const float pressure_before_stop = sim.state().lo_pressure_bar;
    sim.requestLoPump(false);
    require(!sim.state().lo_pump_running, "pump stops immediately");
    advance(sim, 2.0f);
    require(sim.state().lo_pressure_bar < pressure_before_stop, "pressure decays");
    advance(sim, 12.0f);
    require(sim.state().lo_pressure_bar < 0.05f, "pressure decays almost to zero");
    require(!sim.state().lo_pressure_alarm, "no alarm while intentionally stopped");
}

static void test_loss_of_power_and_fault() {
    EngineSimulation sim;
    sim.setElectricalBus(false);
    sim.requestLoPump(true);
    advance(sim, 9.0f);
    require(!sim.state().lo_pump_running, "no running pump without power");
    require(sim.state().lo_pressure_alarm, "low pressure alarm on failed demand");
    sim.setElectricalBus(true);
    advance(sim, 9.0f);
    require(sim.state().lo_pump_running, "pump can restart after power returns");
    require(!sim.state().lo_pressure_alarm, "alarm clears after recovery");
    sim.setLoPumpFault(true);
    advance(sim, 8.0f);
    require(!sim.state().lo_pump_running, "fault stops pump");
    require(sim.state().lo_pressure_alarm, "fault sustained demand raises alarm");
}

static void test_objective_requires_ecr_verification() {
    EngineSimulation sim;
    sim.inspectEcrConsole();
    require(!sim.state().objective_complete, "cannot complete while stopped");
    sim.requestLoPump(true);
    advance(sim, 8.0f);
    require(!sim.state().objective_complete, "pressure alone does not complete mission");
    sim.inspectEcrConsole();
    require(sim.state().objective_complete, "ECR verification completes objective");
    sim.reset();
    require(!sim.state().objective_complete, "reset clears completion");
}

static void test_bad_timestep() {
    EngineSimulation sim;
    sim.requestLoPump(true);
    sim.step(-1.0f);
    sim.step(0.0f);
    sim.step(NAN);
    sim.step(INFINITY);
    require(sim.state().elapsed_seconds == 0.0f, "invalid delta times ignored");
    sim.step(100.0f);
    require(sim.state().elapsed_seconds < 0.051f, "large delta time clamped");
}

int main() {
    test_start_and_pressure();
    test_stop_and_decay();
    test_loss_of_power_and_fault();
    test_objective_requires_ecr_verification();
    test_bad_timestep();
    std::cout << "All five Engine Watch simulation tests passed.\n";
    return 0;
}
