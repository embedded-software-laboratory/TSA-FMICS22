#!/bin/sh

set -e

function main() {
    if [[ "$1" == "TSG" ]]; then
    	printf "# Table 1. PLCopen Safety library\n"
        BENCHMARK_FOLDER="./benchmark/PLCopen_safety"

	echo "--- SFAntivalent.st"
	./bin/ahorn --cse --cycle-bound 25 --input-file ${BENCHMARK_FOLDER}/SFAntivalent.st --unreachable-labels 80 81 104 --unreachable-branches 90_ff 103_tt 24_ff 170_ff 11_ff

	echo "--- SFEDM.st"
	./bin/ahorn --cse --cycle-bound 25 --input-file ${BENCHMARK_FOLDER}/SFEDM.st --unreachable-branches 50_ff 307_ff 257_ff

	echo "--- SFEmergencyStop.st"
	./bin/ahorn --cse --cycle-bound 25 --input-file ${BENCHMARK_FOLDER}/SFEmergencyStop.st --unreachable-branches 16_ff 26_ff 68_ff 128_ff

	echo "--- SFEnableSwitch.st"
	./bin/ahorn --cse --cycle-bound 25 --input-file ${BENCHMARK_FOLDER}/SFEnableSwitch.st --unreachable-branches 19_ff 30_ff 37_ff 80_ff 111_ff

	echo "--- SFEquivalent.st"
	./bin/ahorn --cse --cycle-bound 25 --input-file ${BENCHMARK_FOLDER}/SFEquivalent.st --unreachable-branches 11_ff 24_ff 72_ff 143_ff

	echo "--- SFESPE.st"
	./bin/ahorn --cse --cycle-bound 25 --input-file ${BENCHMARK_FOLDER}/SFESPE.st --unreachable-branches 16_ff 26_ff 68_ff 128_ff

	echo "--- SFGuardLocking.st"
	./bin/ahorn --cse --cycle-bound 25 --input-file ${BENCHMARK_FOLDER}/SFGuardLocking.st --unreachable-branches 97_ff 137_ff

	echo "--- SFGuardMonitoring.st"
	./bin/ahorn --cse --cycle-bound 25 --input-file ${BENCHMARK_FOLDER}/SFGuardMonitoring.st --unreachable-branches 17_ff 35_ff 45_ff 96_ff 111_ff 179_ff

	echo "--- SFModeSelector.st"
	./bin/ahorn --cse --cycle-bound 25 --input-file ${BENCHMARK_FOLDER}/SFModeSelector.st --unreachable-branches 23_ff 51_ff 150_ff 230_ff

	echo "--- SFMutingSeq.st"
	./bin/ahorn --cse --cycle-bound 25 --input-file ${BENCHMARK_FOLDER}/SFMutingSeq.st --unreachable-branches 81_ff 143_ff 186_tt 220_ff 224_tt --unreachable-labels 187 225

	echo "--- SFOutControl.st"
	./bin/ahorn --cse --cycle-bound 25 --input-file ${BENCHMARK_FOLDER}/SFOutControl.st --unreachable-branches 21_ff 84_ff 109_ff

	echo "--- SFSafeStop.st"
	./bin/ahorn --cse --cycle-bound 25 --input-file ${BENCHMARK_FOLDER}/SFSafeStop.st --unreachable-branches 44_ff 89_ff 110_ff 135_ff

	echo "--- SFSafelyLimitSpeed.st"
	./bin/ahorn --cse --cycle-bound 25 --input-file ${BENCHMARK_FOLDER}/SFSafelyLimitSpeed.st --unreachable-branches 130_ff 155_ff

	echo "--- SFSafetyRequest.st"
	./bin/ahorn --cse --cycle-bound 25 --input-file ${BENCHMARK_FOLDER}/SFSafetyRequest.st --unreachable-branches 17_ff 35_ff 98_ff 211_ff

	echo "--- SFTestableSafetySensor.st"
	./bin/ahorn --cse --cycle-bound 25 --input-file ${BENCHMARK_FOLDER}/SFTestableSafetySensor.st --unreachable-branches 23_ff 51_ff 183_ff 184_tt 184_ff 203_ff 204_tt 204_ff 207_tt 207_ff 210_tt 210_ff 203_tt 183_tt --unreachable-labels 184 185 204 205 207 208 210 211

	echo "--- SFTwoHandControlTypeII.st"
	./bin/ahorn --cse --cycle-bound 25 --input-file ${BENCHMARK_FOLDER}/SFTwoHandControlTypeII.st --unreachable-branches 5_ff 94_ff 119_ff

	echo "--- SFTwoHandControlTypeIII.st"
	./bin/ahorn --cse --cycle-bound 25 --input-file ${BENCHMARK_FOLDER}/SFTwoHandControlTypeIII.st --unreachable-branches 178_ff 134_ff 24_ff 47_ff

	echo "--- examples/4.1.DiagnosticsConcept.st"
	./bin/ahorn --cse --cycle-bound 25 --time-out 600000 --input-file ${BENCHMARK_FOLDER}/examples/4.1.DiagnosticsConcept.st

	echo "--- examples/4.2.SafeMotion.st"
	./bin/ahorn --cse --cycle-bound 25 --time-out 600000 --input-file ${BENCHMARK_FOLDER}/examples/4.2.SafeMotion.st

	echo "--- examples/4.3.Muting.st"
	./bin/ahorn --cse --cycle-bound 25 --time-out 600000 --input-file ${BENCHMARK_FOLDER}/examples/4.3.Muting.st

	echo "--- examples/4.4.SafeMotionIO.st"
	./bin/ahorn --cse --cycle-bound 25 --time-out 600000 --input-file ${BENCHMARK_FOLDER}/examples/4.4.SafeMotionIO.st

	echo "--- examples/4.5.TwoHandControl.st"
	./bin/ahorn --cse --cycle-bound 25 --time-out 600000 --input-file ${BENCHMARK_FOLDER}/examples/4.5.TwoHandControl.st
    elif [[ "$1" == "TSA" ]]; then
        printf "# Table 3. TSA\n"
        BENCHMARK_FOLDER="./benchmark/PPU"
        
        echo "--- Scenario_0 -> Scenario_1"
        ./bin/ahorn --verbose info --cycle-bound 25 --sse --test-suite ${BENCHMARK_FOLDER}/ST/test_suite_scenario_0 --input-file ${BENCHMARK_FOLDER}/reconfiguration/Scenario0_1.st
        
        echo "--- Scenario_0 -> Scenario_2"
        ./bin/ahorn --verbose info --cycle-bound 25 --sse --test-suite ${BENCHMARK_FOLDER}/ST/test_suite_scenario_0 --input-file ${BENCHMARK_FOLDER}/reconfiguration/Scenario0_2.st
        
        echo "--- Scenario_2 -> Scenario_3"
        ./bin/ahorn --verbose info --cycle-bound 25 --sse --test-suite ${BENCHMARK_FOLDER}/ST/test_suite_scenario_2 --input-file ${BENCHMARK_FOLDER}/reconfiguration/Scenario2_3.st
    else
    	printf "Missing argument TSG or TSA.\n"
    fi
}

main "$@"
