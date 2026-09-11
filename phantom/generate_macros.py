import os
import re

# ============================================================
# CONFIGURAÇÃO
# ============================================================

SPECTRUM_DIR   = "geant4_spectra_prob"
MACRO_DIR      = "macros/generated"
N_THREADS      = 128

# Geometria da fonte
SOURCE_RADIUS        = 3    # m
DOWNWARD_SOURCE_Z    = 13     # m acima da superfície (z_superficie + 2)
UPWARD_SOURCE_Z      = 11    # m na z_superficie

os.makedirs(MACRO_DIR, exist_ok=True)

# ============================================================
# MASS NUMBER A POR Z
# ============================================================

ION_A = {
    1: 1,   2: 4,   3: 7,   4: 9,   5: 11,
    6: 12,  7: 14,  8: 16,  9: 19,  10: 20,
    11: 23, 12: 24, 13: 27, 14: 28, 15: 31,
    16: 32, 17: 35, 18: 40, 19: 39, 20: 40,
    21: 45, 22: 48, 23: 51, 24: 52, 25: 55,
    26: 56, 27: 59, 28: 58,
}

# ============================================================
# PARTICLE MAP  (nome_ficheiro → nome_geant4)
# ============================================================

PARTICLE_MAP = {
    "H"   : "proton",
    "2H"  : "deuteron",
    "3H"  : "triton",
    "3He" : "He3",
    "4He" : "alpha",
    "n"   : "neutron",
    "gamma": "gamma",
    "e-"  : "e-",
    "e+"  : "e+",
    "mu-" : "mu-",
    "mu+" : "mu+",
    "pi-" : "pi-",
    "pi+" : "pi+",
}

# ============================================================
# IDENTIFICAR PARTÍCULA E DIREÇÃO A PARTIR DO NOME DO FICHEIRO
# ============================================================

def identify_spectrum(filename):
    name = filename[:-4]   # remove .txt

    # Iões pesados: ion_Z<N>_<direction>
    match = re.fullmatch(r"ion_Z(\d+)_(downward|upward)", name)
    if match:
        Z = int(match.group(1))
        direction = match.group(2)
        if Z not in ION_A:
            raise ValueError(f"Z={Z} não existe no dicionário ION_A.")
        return {"type": "ion", "Z": Z, "A": ION_A[Z], "direction": direction}

    # Partículas normais: <nome>_<direction>
    for input_name, geant_name in PARTICLE_MAP.items():
        prefix = input_name + "_"
        if name.startswith(prefix):
            direction = name[len(prefix):]
            if direction not in ("downward", "upward"):
                raise ValueError(f"Direção inválida: {filename}")
            return {"type": "particle", "geant_name": geant_name,
                    "input_name": input_name, "direction": direction}

    raise ValueError(f"Não foi possível identificar: {filename}")

# ============================================================
# LIMPAR MACROS ANTIGOS
# ============================================================

for fn in os.listdir(MACRO_DIR):
    if fn.endswith(".mac"):
        os.remove(os.path.join(MACRO_DIR, fn))

# ============================================================
# ENCONTRAR ESPECTROS
# ============================================================

spectra = sorted(
    fn for fn in os.listdir(SPECTRUM_DIR) if fn.endswith(".txt")
)

print()
print("=" * 60)
print("GERAÇÃO DE MACROS")
print("=" * 60)
print(f"Espectros encontrados : {len(spectra)}")
print(f"Threads               : {N_THREADS}")
#print(f"Eventos por simulação : {N_EVENTS:,}")
print(f"Raio da fonte         : {SOURCE_RADIUS} m")
print(f"Z fonte downward      : {DOWNWARD_SOURCE_Z} m")
print(f"Z fonte upward        : {UPWARD_SOURCE_Z} m")
print()



# ============================================================
# NÚMERO DE PRIMÁRIOS POR PARTÍCULA E DIREÇÃO
# ============================================================

N_EVENTS_3M = 3_000_000
N_EVENTS_1M = 1_000_000
N_EVENTS_300k = 300_000
N_EVENTS_100K = 100_000

Z_1M_DOWN = {3, 4, 5, 6, 7, 8}

Z_300k_DOWN = {9, 10, 11, 12, 13, 14, 16, 26}

Z_100K_DOWN = {
    15, 17, 18, 19, 20, 21, 22, 23, 24, 25, 27, 28
}

Z_100K_UP = {3, 4, 5, 6, 7}

PARTICLES_3M_BOTH = {
    "2H", "3H", "3He", "H", "4He",
    "e-", "e+", "pi-", "pi+",
    "mu-", "mu+", "n", "gamma"
}


def get_number_of_events(info):
    direction = info["direction"]

    # --------------------------------------------------------
    # Iões
    # --------------------------------------------------------
    if info["type"] == "ion":
        Z = info["Z"]

        if direction == "downward":
            if Z in Z_1M_DOWN:
                return N_EVENTS_1M
            elif Z in Z_300k_DOWN:
                return N_EVENTS_300k
            elif Z in Z_100K_DOWN:
                return N_EVENTS_100K

        elif direction == "upward":
            if Z in Z_100K_UP:
                return N_EVENTS_100K

    # --------------------------------------------------------
    # Partículas normais
    # --------------------------------------------------------
    else:
        input_name = info["input_name"]

        if input_name in PARTICLES_3M_BOTH:
            return N_EVENTS_3M

    raise ValueError(
        f"Não foi definida uma categoria de eventos para "
        f"{info}."
    )



# ============================================================
# GERAR MACROS
# ============================================================


for index, filename in enumerate(spectra, start=1):

    info      = identify_spectrum(filename)
    direction = info["direction"]

    N_EVENTS = get_number_of_events(info)

    # Nome descritivo para o ficheiro de macro e pasta de resultados
    if info["type"] == "ion":
        label = f"ion_Z{info['Z']}"
    else:
        label = info["input_name"]

    macro_name = f"{index:03d}_{label}_{direction}.mac"
    macro_path = os.path.join(MACRO_DIR, macro_name)
    spectrum_path = os.path.abspath(os.path.join(SPECTRUM_DIR, filename))

    with open(macro_path, "w") as f:

        # ---- Cabeçalho ----
        f.write("# " + "=" * 58 + "\n")
        f.write(f"# Simulation {index:03d}\n")
        f.write(f"# Spectrum  : {filename}\n")
        f.write(f"# Direction : {direction}\n")

        if info["type"] == "ion":
            f.write(f"# Ion       : Z={info['Z']}  A={info['A']}\n")
        else:
            f.write(f"# Particle  : {info['geant_name']}\n")

        f.write(f"# Primaries : {N_EVENTS:,}\n")
        f.write("# " + "=" * 58 + "\n\n")

        # ---- Threading + initialize ----
        f.write(f"/run/numberOfThreads {N_THREADS}\n")
        f.write("/run/initialize\n\n")

        # ---- Partícula ----
        if info["type"] == "ion":
            f.write(f"/generator/setIon {info['Z']} {info['A']}\n")
        else:
            f.write(f"/generator/setParticle {info['geant_name']}\n")

        # ---- Espectro ----
        f.write(f"/generator/setSpectrum {spectrum_path}\n")

        # ---- Direcção ----
        f.write(f"/generator/setDirection {direction}\n")

        # ---- Geometria da fonte ----
        f.write(f"/generator/sourceRadius {SOURCE_RADIUS} m\n")
        f.write(f"/generator/downwardSourceZ {DOWNWARD_SOURCE_Z} m\n")
        f.write(f"/generator/upwardSourceZ {UPWARD_SOURCE_Z} m\n\n")

        # ---- Run ----
        f.write(f"/run/beamOn {N_EVENTS}\n")

    print(
        f"  {index:03d}  {filename:<35} "
        f"→ {macro_name:<35} "
        f"({N_EVENTS:,} primários)"
    )


# ============================================================
# SUMMARY
# ============================================================

print()
print("=" * 60)
print(f"Macros criados : {len(spectra)}")
print(f"Diretório      : {MACRO_DIR}")
print("=" * 60)
