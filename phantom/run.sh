#!/bin/bash

# ============================================================
# CONFIGURAÇÃO
# ============================================================

PHANTOM="$1"

# Caminho absoluto da pasta onde o script foi lançado
BASE_DIR="$(pwd)"
EXECUTABLE="$BASE_DIR/exampleB1"
MACRO_DIR="$BASE_DIR/macros/generated"
RESULTS_DIR="$BASE_DIR/results"

# ============================================================
# AUMENTAR STACK SIZE — previne segfault (stack overflow)
# com 128 threads e cascatas hadrónicas profundas
# ============================================================

ulimit -s unlimited

# ============================================================
# VALIDAR PHANTOM
# ============================================================

if [ "$PHANTOM" != "male" ] && [ "$PHANTOM" != "female" ]; then
    echo "Uso:"
    echo "  ./run.sh male"
    echo "  ./run.sh female"
    exit 1
fi

# ============================================================
# PHANTOM FLAG
# ============================================================

if [ "$PHANTOM" == "female" ]; then
    PHANTOM_FLAG="-f"
else
    PHANTOM_FLAG=""
fi

# ============================================================
# RESULTADOS
# ============================================================

mkdir -p "$RESULTS_DIR/$PHANTOM"

# ============================================================
# MACROS
# ============================================================

mapfile -t MACROS < <(
    find "$MACRO_DIR" \
        -maxdepth 1 \
        -type f \
        -name "*.mac" \
        | sort
)

echo
echo "============================================================"
echo "PHANTOM: $PHANTOM"
echo "MACROS:  ${#MACROS[@]}"
echo "Stack:   $(ulimit -s)"
echo "============================================================"
echo

# ============================================================
# EXECUTAR UMA A UMA
# ============================================================

for MACRO in "${MACROS[@]}"; do

    BASENAME=$(basename "$MACRO" .mac)

    # Remove o prefixo 001_, 002_, etc.
    SPECTRUM_NAME=$(echo "$BASENAME" | sed 's/^[0-9]\{3\}_//')

    OUTPUT_DIR="$RESULTS_DIR/$PHANTOM/$SPECTRUM_NAME"
    mkdir -p "$OUTPUT_DIR"

    echo
    echo "============================================================"
    echo "START: $BASENAME"
    echo "PHANTOM: $PHANTOM"
    echo "OUTPUT: $OUTPUT_DIR"
    echo "============================================================"

    # --------------------------------------------------------
    # SALTAR SE JÁ EXISTE OUTPUT
    # --------------------------------------------------------

    if [ -f "$OUTPUT_DIR/MarsDosimetry_absorbed.out" ]; then
        echo "SKIP (já existe): $BASENAME"
        continue
    fi

    # --------------------------------------------------------
    # LIMPAR OUTPUTS ANTIGOS
    # --------------------------------------------------------

    rm -f "$BASE_DIR/MarsDosimetry_absorbed.out"
    rm -f "$BASE_DIR/MarsDosimetry_equivalent.out"
    rm -f "$BASE_DIR/MarsDosimetry_effective.out"

    # --------------------------------------------------------
    # EXECUTAR
    # --------------------------------------------------------

    START=$(date +%s)

    "$EXECUTABLE" $PHANTOM_FLAG \
        -m "$MACRO" \
        > "$OUTPUT_DIR/log.txt" 2>&1

    STATUS=$?
    END=$(date +%s)
    ELAPSED=$(( END - START ))

    # --------------------------------------------------------
    # GUARDAR OUTPUTS
    # --------------------------------------------------------

    if [ $STATUS -eq 0 ]; then
        for f in MarsDosimetry_absorbed.out \
                 MarsDosimetry_equivalent.out \
                 MarsDosimetry_effective.out; do
            [ -f "$BASE_DIR/$f" ] && mv "$BASE_DIR/$f" "$OUTPUT_DIR/$f"
        done
    fi

    # --------------------------------------------------------
    # VERIFICAR
    # --------------------------------------------------------

    if [ $STATUS -eq 0 ]; then
        echo "✓ FINISHED: $BASENAME  (${ELAPSED}s)"
    else
        echo "✗ ERROR: $BASENAME"
        echo "  Exit code: $STATUS"
        echo "  Tempo:     ${ELAPSED}s"
        echo "  Log:       $OUTPUT_DIR/log.txt"
        exit $STATUS
    fi

done

echo
echo "============================================================"
echo "TODAS AS SIMULAÇÕES TERMINARAM"
echo "PHANTOM: $PHANTOM"
echo "============================================================"
