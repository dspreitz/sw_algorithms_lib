#!/bin/bash
# Before/after comparison of a library change on a recorded flight
# (larus-breeze/sw_tools#49): builds the SIL (sw_tools/sensor_data_analyzer)
# once with the library before the change and once with the library after it,
# runs both on the flight logs and compares their key figures.
#
#   tests/sil_flight_regression.sh BEFORE_LIB AFTER_LIB SW_TOOLS FLIGHT_DATA
#
# Fails only if the SIL with the changed library cannot be built, crashes or
# writes non-finite values. Deviations of the key figures are reported (job
# summary in GitHub Actions), because library changes often alter results on
# purpose.
set -euo pipefail

before_lib=$(realpath "$1")
after_lib=$(realpath "$2")
sw_tools=$(realpath "$3")
data=$(realpath "$4")
work=$(mktemp -d)
script=sensor_data_analyzer/tests/flight_regression.py
summary=${GITHUB_STEP_SUMMARY:-/dev/null}

build() { # name, library
  rm -rf "$work/$1"
  cp -r "$sw_tools" "$work/$1"
  rm -rf "$work/$1/sensor_data_analyzer/lib"
  cp -r "$2" "$work/$1/sensor_data_analyzer/lib"
  cmake -S "$work/$1/sensor_data_analyzer" -B "$work/$1/build" -DCMAKE_BUILD_TYPE=Release >/dev/null
  cmake --build "$work/$1/build" --parallel >/dev/null
}

flights=$(python3 -c "import json,sys; print(' '.join(json.load(open(sys.argv[1]))['flights']))" \
          "$sw_tools/sensor_data_analyzer/tests/flight_reference.json")

build after "$after_lib"
before_ok=true
if ! build before "$before_lib"; then
  echo "::warning::The SIL does not build with the library before the change; only the changed library is checked."
  before_ok=false
fi

status=0
for flight in $flights; do
  if [ "$before_ok" = true ]; then
    python3 "$sw_tools/$script" --emulator "$work/before/build/larus_emulator" --data "$data" \
      --figures-out "$work/$flight.before.json" "$flight" || before_ok=false
  fi
  if [ "$before_ok" = true ]; then
    python3 "$sw_tools/$script" --emulator "$work/after/build/larus_emulator" --data "$data" \
      --against "$work/$flight.before.json" --report-only --label "(library before vs. after the change)" \
      "$flight" || status=1
  else
    python3 "$sw_tools/$script" --emulator "$work/after/build/larus_emulator" --data "$data" \
      --report-only --label "(changed library vs. sw_tools reference)" "$flight" || status=1
  fi
done
rm -rf "$work"
exit $status
