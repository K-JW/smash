#!/bin/bash

fail() {
  echo "$*" >&2
  exit 1
}

type perf >/dev/null 2>&1 || fail "Failed to find perf (This script only works on Linux)."

BASEDIR=$(dirname $BASH_SOURCE)
smash_root_dir="${BASEDIR}/../../.."
configs_dir="${smash_root_dir}/src/benchmarks/configs"
n_cores=`grep -c processor /proc/cpuinfo`

echo
echo "Building SMASH ..."
echo

mkdir -p build || fail "Failed to create $PWD/build directory."
cd build || fail "Failed to change directory to $PWD/build."

# cmake -DCMAKE_BUILD_TYPE=Release $smash_root_dir || fail "Failed to configure the build system."
# make -j${n_cores} || fail "Failed to build SMASH."

echo
echo "Collecting System Information ..."

SMASH_VER_NUM=$(./smash -v | grep -E 'SMASH-')
SMASH_VER_OUT=$(./smash -v)
HW_INFO=$(lshw -short 2> /dev/null)
UNAME_INFO=$(uname -a)

echo
echo "Running Benchmarks ..."
echo

benchmark_run() {
  CONFIG_DIR=$1
  DECAYM_DIR=$2
  PART_DIR=$3
  if [ "$#" -eq 4 ]
  then
    CONFIG_OPT=$4
    PERF_OUT=$(perf stat -B \
               ./smash \
               -i ${configs_dir}/${CONFIG_DIR}/config.yaml \
               -d ${configs_dir}/${DECAYM_DIR}/decaymodes.txt \
               -p ${configs_dir}/${PART_DIR}/particles.txt \
               -c "$CONFIG_OPT" \
               2>&1 >/dev/null)
  else
    PERF_OUT=$(perf stat -B \
               ./smash \
               -i ${configs_dir}/${CONFIG_DIR}/config.yaml \
               -d ${configs_dir}/${DECAYM_DIR}/decaymodes.txt \
               -p ${configs_dir}/${PART_DIR}/particles.txt \
               2>&1 >/dev/null)
  fi
  echo "$PERF_OUT"
}

# Defaults
PART_DEF="."
DECAYM_DEF="."

echo "Started benchmark for collider ..."
coll_perf=$(benchmark_run collider $DECAYM_DEF $PART_DEF)
echo "$coll_perf" | grep -E "time elapsed"

# echo "Started benchmark for timestepless ..."
# nots_perf=$(benchmark_run collider $DECAYM_DEF $PART_DEF 'General: { Time_Step_Mode: None }')
# echo "$nots_perf" | grep -E "time elapsed"

# echo "Started benchmark for adaptive ..."
# adts_perf=$(benchmark_run collider $DECAYM_DEF $PART_DEF 'General: { Time_Step_Mode: Adaptive }')
# echo "$adts_perf" | grep -E "time elapsed"

# echo "Started benchmark for box ..."
# box_perf=$(benchmark_run box box box)
# echo "$box_perf" | grep -E "time elapsed"

# echo "Started benchmark for sphere ..."
# sphere_perf=$(benchmark_run sphere $DECAYM_DEF $PART_DEF)
# echo "$sphere_perf" | grep -E "time elapsed"

echo "Started benchmark for dileptons ..."
dilepton_perf=$(benchmark_run collider dileptons $PART_DEF 'Output: { Dileptons: {Format: ["Binary"], Extended: True} }')
echo "$dilepton_perf" | grep -E "time elapsed"

# echo "Started benchmark for testparticles ..."
# testp_perf=$(benchmark_run testparticles $DECAYM_DEF $PART_DEF)
# echo "$testp_perf" | grep -E "time elapsed"
#
# echo "Started benchmark for potentials ..."
# potentials_perf=$(benchmark_run potentials $DECAYM_DEF $PART_DEF)
# echo "$potentials_perf" | grep -E "time elapsed"


OUTPUT_FILE_N=bm-results-${SMASH_VER_NUM}.md

# \` are espaced backticks
cat > ../${OUTPUT_FILE_N}<<EOF
# Benchmark Results for $SMASH_VER_NUM

### \`smash -v\`
\`\`\`
$SMASH_VER_OUT}
\`\`\`

## System information

### \`uname -a\`
\`\`\`
$UNAME_INFO
\`\`\`

### \`lshw -short\`

\`\`\`
$HW_INFO
\`\`\`

## Results

Performed with \`perf\` and the \`stat -B\` options.

### Collider Run (AuAu@1.23)
\`\`\`
$coll_perf
\`\`\`

### Collider Run without Timesteps
Same setup as collider run, but without timesteps.
\`\`\`
$nots_perf
\`\`\`

### Collider Run with Adaptive Timesteps
Same setup as collider run, but without timesteps.
\`\`\`
$adts_perf
\`\`\`

### Box Run
\`\`\`
$box_perf
\`\`\`

### Sphere Run
\`\`\`
$sphere_perf
\`\`\`

### Dilepton Run
Same setup as collider run, but with dileptons.
\`\`\`
$dilepton_perf
\`\`\`

### Collider Run with Testparticles (CuCu@1.23)
Baseline for Potentials Run.
\`\`\`
$testp_perf
\`\`\`

### Potentials Run
Same setup as with Testparticles, but also with potentials.
\`\`\`
$potentials_perf
\`\`\`
EOF
echo
echo "Benchmark results are written to $OUTPUT_FILE_N"
