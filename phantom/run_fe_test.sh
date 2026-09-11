#!/bin/bash

# ============================================================
# TESTE DE TEMPO PARA IÃO PESADO (Fe, Z=26)
# Corre 100k eventos para estimar tempo por evento
# e extrapolar para 5M
# ============================================================

EXECUTABLE="./exampleB1"
SPECTRUM="geant4_spectra_prob/ion_Z26_downward.txt"
OUTPUT_DIR="results/convergence/fe_test"
PHANTOM="${1:-male}"

if [ "$PHANTOM" == "female" ]; then
    PHANTOM_FLAG="-f"
else
    PHANTOM_FLAG=""
fi

mkdir -p "$OUTPUT_DIR"

if [ ! -f "$SPECTRUM" ]; then
    echo "ERRO: espectro não encontrado: $SPECTRUM"
    exit 1
fi

MACRO="/tmp/fe_test.mac"

cat > "$MACRO" << MACEOF
/run/numberOfThreads 128
/run/initialize

/generator/setIon 26 56
/generator/setSpectrum ${SPECTRUM}
/generator/setDirection downward
/generator/sourceRadius 1.5 m
/generator/downwardSourceZ 16.135 m

/run/beamOn 100000
MACEOF

echo
echo "============================================================"
echo "TESTE Fe (Z=26, A=56) — 100k eventos"
echo "Phantom  : $PHANTOM"
echo "Espectro : $SPECTRUM"
echo "============================================================"
echo

START=$(date +%s)

nohup "$EXECUTABLE" $PHANTOM_FLAG \
    -m "$MACRO" \
    > "$OUTPUT_DIR/log.txt" 2>&1

STATUS=$?
END=$(date +%s)
ELAPSED=$(( END - START ))

if [ $STATUS -eq 0 ]; then
    echo "✓ FINISHED  tempo=${ELAPSED}s  (~$(( ELAPSED/60 )) min)"
    [ -f "MarsDosimetry_absorbed.out" ] && mv "MarsDosimetry_absorbed.out" "$OUTPUT_DIR/"
    [ -f "MarsDosimetry_equivalent.out" ] && mv "MarsDosimetry_equivalent.out" "$OUTPUT_DIR/"
    [ -f "MarsDosimetry_effective.out" ] && mv "MarsDosimetry_effective.out" "$OUTPUT_DIR/"

    echo
    echo "============================================================"
    echo "EXTRAPOLAÇÃO PARA 5M EVENTOS:"
    echo "  Fe 100k  → ${ELAPSED}s"
    TEMPO_5M=$(( ELAPSED * 50 ))
    TEMPO_5M_MIN=$(( TEMPO_5M / 60 ))
    TEMPO_5M_H=$(( TEMPO_5M / 3600 ))
    echo "  Fe 5M    → ~${TEMPO_5M_MIN} min (~${TEMPO_5M_H}h)"
    echo
    TOTAL_MIN=$(( TEMPO_5M_MIN * 114 ))
    TOTAL_H=$(( TOTAL_MIN / 60 ))
    TOTAL_D=$(( TOTAL_H / 24 ))
    echo "  114 sims × ${TEMPO_5M_MIN} min = ~${TOTAL_H}h = ~${TOTAL_D} dias"
    echo "  (estimativa conservadora — assume todas as sims = Fe)"
    echo "============================================================"
else
    echo "✗ ERRO  exit=$STATUS"
    echo "  Ver log: $OUTPUT_DIR/log.txt"
    exit $STATUS
fi
