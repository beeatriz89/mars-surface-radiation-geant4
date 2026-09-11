#!/usr/bin/env python3
"""
FASE 2 do estudo de numero de primarios.

Le pilot_results.csv (gerado ao correres as macros de generate_pilot_macros.py)
e calcula, para cada (particula, energia), o N necessario para atingir a
incerteza-alvo (TARGET_REL_UNCERT_PCT), usando a lei de escala do erro
padrao da media: erro ~ 1/sqrt(N)  =>  N_alvo = N_piloto * (erro_piloto/erro_alvo)^2

Usa o pior dos dois erros (dose ou H) para cada ponto, para garantir que
ambos ficam dentro do alvo. Aplica um teto (MAX_N) para nao disparares
sem querer para um run gigante nalgum ponto com erro piloto muito alto.

Uso:
    python3 estimate_required_n.py
    -> escreve macros_production/<particula>.mac
"""

import os
import csv
import math
from beam_config_list import ENERGIES_MEV, PARTICLES

PILOT_CSV = "pilot_results.csv"
OUTPUT_DIR = "macros_production"
PRODUCTION_CSV = "conversion_factors.csv"
N_THREADS = 8

TARGET_REL_UNCERT_PCT = 1.0
MIN_N = 20000
MAX_N = 50_000_000


def load_pilot_results():
    """ dict[(g4name, energy_mev)] -> (N_pilot, relUnc_dose_pct, relUnc_H_pct) """
    data = {}
    if not os.path.exists(PILOT_CSV):
        raise SystemExit(f"Nao encontrei '{PILOT_CSV}'. Corre primeiro as macros piloto.")

    with open(PILOT_CSV, newline="") as f:
        for row in csv.DictReader(f):
            key = (row["Particle"], round(float(row["Energy_MeV"]), 6))
            data[key] = (
                int(row["N_events"]),
                float(row["Dose_relUncert_pct"]),
                float(row["H_relUncert_pct"]),
            )
    return data


def required_n(n_pilot, relunc_pilot_pct, target_pct):
    if relunc_pilot_pct <= 0 or relunc_pilot_pct <= target_pct:
        return n_pilot
    return int(math.ceil(n_pilot * (relunc_pilot_pct / target_pct) ** 2))


def main():
    pilot = load_pilot_results()
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    warnings = []

    for fname, gun_cmd, is_ion, g4name in PARTICLES:
        lines = [
            f"# Macro de PRODUCAO para {fname} (N calibrado a partir do piloto,",
            f"# alvo de incerteza = {TARGET_REL_UNCERT_PCT}%)",
            f"/run/numberOfThreads {N_THREADS}",
            "/run/initialize",
            "",
            f"/output/csvFile {PRODUCTION_CSV}",
            "",
        ]

        if not is_ion:
            lines.append(gun_cmd)

        for e in ENERGIES_MEV:
            key = (g4name, round(float(e), 6))
            entry = pilot.get(key)

            if entry is None:
                n_needed = MIN_N
                warnings.append(f"  [AVISO] '{g4name}' @ {e} MeV: sem dados piloto "
                                 f"(chave {key} nao encontrada) - a usar N_MIN={MIN_N}.")
            else:
                n_pilot, relunc_d, relunc_h = entry
                n_needed = max(required_n(n_pilot, relunc_d, TARGET_REL_UNCERT_PCT),
                                required_n(n_pilot, relunc_h, TARGET_REL_UNCERT_PCT),
                                MIN_N)
                if n_needed > MAX_N:
                    warnings.append(f"  [AVISO] '{g4name}' @ {e} MeV: N={n_needed} excede "
                                     f"MAX_N={MAX_N} - a usar o teto (alvo pode nao ser atingido).")
                    n_needed = MAX_N

            if is_ion:
                lines.append(f"{gun_cmd} {e}")
            else:
                lines.append(f"/source/energy {e} MeV")
            lines.append(f"/run/beamOn {n_needed}")
            lines.append("")

        with open(os.path.join(OUTPUT_DIR, f"{fname}.mac"), "w") as f:
            f.write("\n".join(lines))
        print(f"Escrito: {OUTPUT_DIR}/{fname}.mac")

    if warnings:
        print("\nAvisos:")
        print("\n".join(warnings))

    print(f"\nMacros de producao em '{OUTPUT_DIR}/'. Resultados finais em '{PRODUCTION_CSV}'.")


if __name__ == "__main__":
    main()
