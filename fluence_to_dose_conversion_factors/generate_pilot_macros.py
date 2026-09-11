#!/usr/bin/env python3
"""
FASE 1 do estudo de numero de primarios.

Gera um .mac por particula (macros_pilot/<particula>.mac), cada um a
percorrer as 13 energias com um N pequeno (N_PILOT), escrevendo para
pilot_results.csv (separado do CSV de producao).

Uso:
    python3 generate_pilot_macros.py
    ./convFactors macros_pilot/gamma.mac
    ./convFactors macros_pilot/proton.mac
    ... (as 18)

Depois de correres todas, corre estimate_required_n.py para gerar as
macros de producao com o N certo para cada (particula, energia).
"""

import os
from beam_config_list import ENERGIES_MEV, PARTICLES

N_PILOT = 20000       # primarios por ponto no piloto (so para estimar sigma)
N_THREADS = 16
OUTPUT_DIR = "macros_pilot"
PILOT_CSV = "pilot_results.csv"


def main():
    os.makedirs(OUTPUT_DIR, exist_ok=True)

    for fname, gun_cmd, is_ion, _g4name, A in PARTICLES:
        lines = [
            f"# Macro PILOTO para {fname} (N={N_PILOT} por energia, so para estimar incerteza)",
            f"/run/numberOfThreads {N_THREADS}",
            "/run/initialize",
            "",
            f"/output/csvFile {PILOT_CSV}",
            f"/output/primaryOnly true",
            "",
        ]

        if not is_ion:
            lines.append(gun_cmd)

        for e in ENERGIES_MEV:
            if is_ion:
                lines.append(f"{gun_cmd} {e}")
            else:
                lines.append(f"/source/energy {e} MeV")
            lines.append(f"/run/beamOn {N_PILOT}")
            lines.append("")

        with open(os.path.join(OUTPUT_DIR, f"{fname}.mac"), "w") as f:
            f.write("\n".join(lines))
        print(f"Escrito: {OUTPUT_DIR}/{fname}.mac")

    print(f"\n{len(PARTICLES)} macros piloto geradas em '{OUTPUT_DIR}/'.")
    print(f"Corre cada uma (ex: ./convFactors {OUTPUT_DIR}/gamma.mac).")
    print(f"Os resultados vao-se acumulando em '{PILOT_CSV}'.")
    print("Quando tiveres as 18 correram, usa estimate_required_n.py")


if __name__ == "__main__":
    main()
