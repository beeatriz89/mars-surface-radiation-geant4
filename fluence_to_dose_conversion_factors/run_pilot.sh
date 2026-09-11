#!/usr/bin/env bash
# Corre todas as macros piloto, uma a seguir a outra, e regista logs.
#
# Uso no cluster (fica a correr mesmo depois de fechares a sessao ssh):
#   nohup ./run_pilot.sh > run_pilot_master.log 2>&1 &
#   disown
#
# Acompanhar progresso:
#   tail -f run_pilot_master.log
#   tail -f logs/pilot_<particula>.log

set -uo pipefail

EXE=./convFactors
MACRO_DIR=macros_pilot
LOG_DIR=logs

mkdir -p "$LOG_DIR"

if [ ! -x "$EXE" ]; then
  echo "ERRO: nao encontrei o executavel $EXE (compilaste o projeto?)"
  exit 1
fi

if [ ! -d "$MACRO_DIR" ]; then
  echo "ERRO: nao encontrei $MACRO_DIR/ (corre primeiro: python3 generate_pilot_macros.py)"
  exit 1
fi

echo "=== Inicio: $(date) ==="

for macro in "$MACRO_DIR"/*.mac; do
  name=$(basename "$macro" .mac)
  echo "--- A correr: $name  ($(date)) ---"
  "$EXE" "$macro" > "$LOG_DIR/pilot_${name}.log" 2>&1
  status=$?
  if [ $status -ne 0 ]; then
    echo "    [AVISO] $name terminou com codigo $status - ver $LOG_DIR/pilot_${name}.log"
  else
    echo "    OK: $name"
  fi
done

echo "=== Fim: $(date) ==="
echo "Resultados em pilot_results.csv"
