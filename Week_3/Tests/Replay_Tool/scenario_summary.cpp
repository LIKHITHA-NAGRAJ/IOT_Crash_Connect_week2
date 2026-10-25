// Host tool: runs every SIMULATED accident scenario through the staged detector and writes a CSV summary.
// Build (from this folder):
//   g++ -std=c++17 -I ../../Firmware -I ../../Firmware/Accident_Detection scenario_summary.cpp ../../Firmware/Accident_Detection/accident_detector.cpp ../../Firmware/Accident_Detection/sim_scenarios.cpp ../../Firmware/config.cpp -o scenario_summary
// Run:
//   ./scenario_summary > ../Scenario_Results.csv
// ALL DATA IS SIMULATED. Thresholds are placeholders. Not validated accident detection.
#include <stdio.h>
#include <string.h>
#include "accident_detector.h"
#include "sim_scenarios.h"
#include "config.h"

struct Run { AccidentDetector* det; AccidentResult first; bool haveFirst; };

static void onSample(const SimSample& s, void* p) {
  Run* r = (Run*)p;
  if (s.speedState == 1) r->det->feedSpeed(true, s.speedKmh);
  else if (s.speedState == 2) r->det->feedSpeed(false, 0.0f);
  ImuInput in = { s.tMs, s.ax, s.ay, s.az, s.gx, s.gy, s.gz };
  AccidentResult res;
  if (r->det->feedSample(in, res) && !r->haveFirst) { r->first = res; r->haveFirst = true; }
}

static const char* verdict(const ScenarioInfo& si, const AccidentStats& st) {
  if (si.ideal) return st.confirmed > 0 ? "DETECTED" : "MISSED";
  if (st.confirmed > 0) return "FALSE_POSITIVE";
  return st.candidates > 0 ? "CANDIDATE_REJECTED" : "QUIET";
}

int main() {
  printf("scenario,data_source,description,ideal_accident,"
         "default_candidates,default_confirmed,default_rejected,default_verdict,decision_reason,"
         "peak_g,gyro_peak_dps,orientation_deg,stillness,speed_drop,low_confidence,"
         "jerk_candidates,jerk_confirmed,jerk_rejected,jerk_verdict,"
         "expected_candidates,expected_confirmed,expected_jerk_candidates,matches_documented_behaviour\n");
  for (int i = 0; i < scenarioCount(); i++) {
    const ScenarioInfo& si = scenarioInfo(i);
    AccidentConfig cfgA = CONFIG.accident; cfgA.jerkEnabled = false;
    AccidentConfig cfgB = CONFIG.accident; cfgB.jerkEnabled = true;
    AccidentDetector a, b;
    a.begin(cfgA); b.begin(cfgB);
    Run ra = { &a, {}, false }, rb = { &b, {}, false };
    generateScenario(i, onSample, &ra);
    generateScenario(i, onSample, &rb);
    AccidentStats sa = a.stats(), sb = b.stats();
    const AccidentResult& r = ra.first;
    bool match = sa.candidates == (uint16_t)si.expCandidates && sa.confirmed == (uint16_t)si.expConfirmed &&
                 sb.candidates == (uint16_t)si.expCandidatesJerk;
    printf("%s,SIMULATED,\"%s\",%s,%u,%u,%u,%s,", si.name, si.description, si.ideal ? "yes" : "no",
           sa.candidates, sa.confirmed, sa.rejected, verdict(si, sa));
    if (ra.haveFirst)
      printf("\"%s\",%.2f,%.0f,%.0f,%s,%s,%s,", r.reason, r.peakG, r.peakGyroDps, r.orientationDeg,
             r.stillness ? "yes" : "no", r.speedDrop ? "yes" : "no", r.lowConfidence ? "yes" : "no");
    else
      printf("\"no candidate\",,,,,,,");
    printf("%u,%u,%u,%s,%d,%d,%d,%s\n", sb.candidates, sb.confirmed, sb.rejected, verdict(si, sb),
           si.expCandidates, si.expConfirmed, si.expCandidatesJerk, match ? "yes" : "NO");
  }
  return 0;
}
