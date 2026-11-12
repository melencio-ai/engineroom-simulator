#include "core/EngineSimulation.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>

using namespace enginewatch;

static void require(bool condition, const char* msg) {
    if (!condition) { std::cerr << "FAIL: " << msg << '\n'; std::exit(1); }
}

static void advance(EngineSimulation& sim, float seconds) {
    const int ticks = static_cast<int>(seconds / 0.025f);
    for (int i = 0; i < ticks; ++i) sim.step(0.025f);
}

static void startAndVerifyFirstMission(EngineSimulation& sim) {
    sim.requestLoPump(true);
    advance(sim, 10.0f);
    require(sim.state().lo_pressure_bar > 4.0f, "normal main pump pressurizes LO header");
    sim.inspectEcrConsole();
    require(sim.state().objective_complete, "Mission 001 explicitly verified");
}

static void test_mission_one() {
    EngineSimulation sim;
    require(sim.state().mission == 1, "starts with mission 1");
    require(sim.state().electrical_bus_energized, "bus initially online");
    require(!sim.state().standby_pump.suction_open && !sim.state().standby_pump.discharge_open,
            "standby valves initially closed");
    require(!sim.beginMissionTwo(), "cannot skip Mission 001");
    sim.inspectEcrConsole();
    require(!sim.state().objective_complete, "cannot verify stopped system");
    sim.requestLoPump(true);
    advance(sim, 0.2f);
    require(!sim.state().main_pump.running, "motor start delay retained");
    advance(sim, 9.0f);
    require(sim.state().main_pump.running && sim.state().lo_pump_running, "legacy main running mirror");
    require(sim.state().lo_pressure_bar > 3.0f, "header has normal pressure");
    require(!sim.state().objective_complete, "verification is explicit, not automatic");
    sim.inspectEcrConsole();
    require(sim.state().objective_complete, "ECR verification works");
    sim.requestLoPump(false);
    advance(sim, 10.0f);
    require(sim.state().lo_pressure_bar < 0.1f, "pressure falls after stop");
    require(!sim.state().lo_pressure_alarm, "no false alarm after deliberate shutdown in Mission 001");
}

static void test_electrical_power_and_faults() {
    EngineSimulation sim;
    sim.setElectricalBus(false);
    sim.requestLoPump(true);
    advance(sim, 10.0f);
    require(!sim.state().main_pump.running, "bus is required");
    require(sim.state().lo_pressure_alarm, "demanded low pressure raises alarm");
    sim.setElectricalBus(true);
    advance(sim, 10.0f);
    require(sim.state().main_pump.running, "restoration restarts pump on request");
    require(!sim.state().lo_pressure_alarm, "low-pressure alarm clears on recovery");
    sim.setLoPumpFault(true);
    advance(sim, 10.0f);
    require(!sim.state().main_pump.running && sim.state().lo_pump_fault, "fault prevents main pump run");
    require(sim.state().lo_pressure_alarm, "fault causes delayed low header pressure alarm");
}

static void test_valves_and_standby() {
    EngineSimulation sim;
    sim.requestPump(StandbyLoPump, true);
    advance(sim, 10.0f);
    require(sim.state().standby_pump.running, "motor can spin with valve closed");
    require(sim.state().lo_pressure_bar < 0.05f, "standby cannot feed header through closed valves");
    sim.setValve(StandbyLoPump, SuctionValve, true);
    advance(sim, 4.0f);
    require(sim.state().lo_pressure_bar < 0.05f, "open suction alone is insufficient");
    sim.setValve(StandbyLoPump, DischargeValve, true);
    advance(sim, 9.0f);
    require(sim.state().lo_pressure_bar > 4.0f, "standby pressurizes with valves open");
    sim.requestPump(MainLoPump, true);
    advance(sim, 5.0f);
    require(sim.state().lo_pressure_bar <= 4.401f, "parallel pump target is bounded");
    sim.setValve(StandbyLoPump, SuctionValve, false);
    sim.setValve(MainLoPump, DischargeValve, false);
    advance(sim, 8.0f);
    require(sim.state().lo_pressure_bar < 0.1f, "both paths blocked -> header loses pressure");
}

static void test_fault_diagnosis_and_transfer() {
    EngineSimulation sim;
    startAndVerifyFirstMission(sim);
    require(sim.beginMissionTwo(), "Mission 002 starts after M001");
    require(sim.state().mission == 2 && !sim.state().mission_two_complete, "Mission 002 status");
    advance(sim, 16.0f);
    require(sim.state().main_strainer_blockage >= 0.89f, "strainer restriction develops");
    require(sim.state().main_strainer_dp_bar > 1.5f, "high DP is diagnostic evidence");
    require(sim.state().main_pump.suction_pressure_bar < 0.30f, "main inlet pressure drops");
    require(sim.state().lo_pressure_bar < 2.5f, "header pressure deteriorates");
    require(sim.state().lo_pressure_alarm, "low pressure alarm triggers after delay");
    sim.verifyMissionTwo();
    require(!sim.state().mission_two_complete, "cannot complete before restoration");
    sim.acknowledgeAlarm();
    require(sim.state().alarm_acknowledged, "alarm acknowledgment recorded");
    sim.inspectStrainer();
    require(sim.state().strainer_inspected, "strainer inspection recorded");

    sim.setValve(StandbyLoPump, SuctionValve, true);
    sim.setValve(StandbyLoPump, DischargeValve, true);
    sim.requestPump(StandbyLoPump, true);
    advance(sim, 10.0f);
    require(sim.state().lo_pressure_bar > 4.0f, "standby restores header pressure");
    require(!sim.state().lo_pressure_alarm, "alarm clears on recovery");
    sim.verifyMissionTwo();
    require(!sim.state().mission_two_complete, "must isolate main failed suction path");
    sim.requestPump(MainLoPump, false);
    sim.setValve(MainLoPump, SuctionValve, false);
    sim.setValve(MainLoPump, DischargeValve, false);
    advance(sim, 3.0f);
    sim.verifyMissionTwo();
    require(sim.state().mission_two_complete, "Mission 002 verified in ECR after diagnosis and restoration");
}

static void test_no_false_mission_two_completion() {
    EngineSimulation sim;
    startAndVerifyFirstMission(sim);
    sim.beginMissionTwo();
    sim.setValve(StandbyLoPump, SuctionValve, true);
    sim.setValve(StandbyLoPump, DischargeValve, true);
    sim.requestPump(StandbyLoPump, true);
    advance(sim, 10.0f);
    sim.requestPump(MainLoPump, false);
    sim.setValve(MainLoPump, SuctionValve, false);
    sim.setValve(MainLoPump, DischargeValve, false);
    sim.verifyMissionTwo();
    require(!sim.state().mission_two_complete, "diagnosis cannot be skipped");
    sim.inspectStrainer();
    require(sim.state().strainer_inspected, "strainer examination recorded after restriction starts");
    sim.verifyMissionTwo();
    require(sim.state().mission_two_complete, "early safe intervention can finish without an alarm");
}

static void test_timestep_and_reset() {
    EngineSimulation sim;
    sim.requestLoPump(true);
    sim.step(-1.0f);
    sim.step(0.0f);
    sim.step(NAN);
    sim.step(INFINITY);
    require(sim.state().elapsed_seconds == 0.0f, "invalid timesteps ignored");
    sim.step(100.0f);
    require(sim.state().elapsed_seconds < 0.051f, "timesteps clamped");
    startAndVerifyFirstMission(sim);
    sim.beginMissionTwo();
    advance(sim, 20.0f);
    sim.reset();
    require(sim.state().mission == 1, "reset restores Mission 001");
    require(!sim.state().objective_complete && !sim.state().mission_two_complete, "reset clears objectives");
    require(sim.state().lo_pressure_bar == 0.0f && sim.state().main_strainer_blockage == 0.0f,
            "reset clears pressure and strainer fault");
}

int main() {
    test_mission_one();
    test_electrical_power_and_faults();
    test_valves_and_standby();
    test_fault_diagnosis_and_transfer();
    test_no_false_mission_two_completion();
    test_timestep_and_reset();
    std::cout << "All six Engine Watch v0.2 simulation test suites passed.\n";
    return 0;
}
