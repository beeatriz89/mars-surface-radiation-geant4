#!/usr/bin/env bash
# Corre todas as macros de producao, uma a seguir a outra, e regista logs.
#
# Uso no cluster:
#   nohup ./run_production.sh > run_production_master.log 2>&1 &
#   disown
#
# Acompanhar progresso:
#   tail -f run_production_master.log
#   tail -f logs/production_<particula>.log
#   wc -l conversion_factors.csv   # cresce uma linha por energia terminada

set -uo pipefail

EXE=./convFactors
MACRO_DIR=macros_production
LOG_DIR=logs

mkdir -p "$LOG_DIR"

if [ ! -x "$EXE" ]; then
  echo "ERRO: nao encontrei o executavel $EXE (compilaste o projeto?)"
  exit 1
fi

if [ ! -d "$MACRO_DIR" ]; then
  echo "ERRO: nao encontrei $MACRO_DIR/ (corre primeiro: python3 estimate_required_n.py)"
  exit 1
fi

echo "=== Inicio: $(date) ==="

for macro in "$MACRO_DIR"/*.mac; do
  name=$(basename "$macro" .mac)
  echo "--- A correr: $name  ($(date)) ---"
  "$EXE" "$macro" > "$LOG_DIR/production_${name}.log" 2>&1
  status=$?
  if [ $status -ne 0 ]; then
    echo "    [AVISO] $name terminou com codigo $status - ver $LOG_DIR/production_${name}.log"
  else
    echo "    OK: $name"
  fi
done

echo "=== Fim: $(date) ==="
echo "Resultados em conversion_factors.csv"
