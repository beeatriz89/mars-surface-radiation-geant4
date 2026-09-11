#!/bin/bash

# ============================================================
# ESTUDO DE CONVERGÊNCIA ESTATÍSTICA
# Corre protões downward com N crescente e guarda os outputs
# ============================================================

EXECUTABLE="./exampleB1"
SPECTRUM="geant4_spectra_prob/H_downward.txt"
RESULTS_DIR="results/convergence"
PHANTOM="${1:-female}"   # default: female. Usa: ./run_convergence.sh male

# ============================================================
# VALIDAR PHANTOM
# ============================================================

if [ "$PHANTOM" != "male" ] && [ "$PHANTOM" != "female" ]; then
    echo "Uso: ./run_convergence.sh [male|female]"
    exit 1
fi

if [ "$PHANTOM" == "female" ]; then
    PHANTOM_FLAG="-f"
else
    PHANTOM_FLAG=""
fi

# ============================================================
# VERIFICAR ESPECTRO
# ============================================================

if [ ! -f "$SPECTRUM" ]; then
    echo "ERRO: espectro não encontrado: $SPECTRUM"
    exit 1
fi

# ============================================================
# LISTA DE N
# ============================================================

N_LIST=(100000)

echo
echo "============================================================"
echo "ESTUDO DE CONVERGÊNCIA — protões downward"
echo "Phantom  : $PHANTOM"
echo "Espectro : $SPECTRUM"
echo "Runs     : ${#N_LIST[@]}"
echo "============================================================"
echo

# ============================================================
# LOOP
# ============================================================

for N in "${N_LIST[@]}"; do

    OUTPUT_DIR="$RESULTS_DIR/${PHANTOM}/H_downward_${N}"
    mkdir -p "$OUTPUT_DIR"

    MACRO="/tmp/conv_H_${N}.mac"

    # ---- Gerar macro ----
    cat > "$MACRO" << MACEOF
/run/numberOfThreads 128
/run/initialize

/generator/setParticle proton
/generator/setSpectrum ${SPECTRUM}
/generator/setDirection downward
/generator/sourceRadius 1.5 m
/generator/downwardSourceZ 16 m
/generator/upwardSourceZ 11 m

/run/beamOn ${N}
MACEOF

    echo "------------------------------------------------------------"
    echo "START : N = ${N}"
    echo "Output: ${OUTPUT_DIR}"
    echo "------------------------------------------------------------"

    START=$(date +%s)

    # ---- Correr (com nohup implícito via redirecionamento) ----
    "$EXECUTABLE" $PHANTOM_FLAG \
        -m "$MACRO" \
        > "$OUTPUT_DIR/log.txt" 2>&1

    STATUS=$?
    END=$(date +%s)
    ELAPSED=$(( END - START ))

    if [ $STATUS -eq 0 ]; then
        echo "✓ FINISHED  N=${N}  tempo=${ELAPSED}s"

        # Mover ficheiros de output para a pasta desta run
        for f in MarsDosimetry_absorbed.out \
                  MarsDosimetry_equivalent.out \
                  MarsDosimetry_effective.out; do
            [ -f "$f" ] && mv "$f" "$OUTPUT_DIR/$f"
        done

        echo "  Outputs guardados em: $OUTPUT_DIR"
    else
        echo "✗ ERRO  N=${N}  exit=$STATUS"
        echo "  Ver log: $OUTPUT_DIR/log.txt"
        exit $STATUS
    fi

    echo

done

# ============================================================
# SUMMARY
# ============================================================

echo "============================================================"
echo "CONVERGÊNCIA COMPLETA"
echo "Resultados em: $RESULTS_DIR/$PHANTOM/"
echo
echo "Para comparar doses por órgão:"
echo "  grep 'lung' $RESULTS_DIR/$PHANTOM/H_downward_*/MarsDosimetry_absorbed.out"
echo "============================================================"
