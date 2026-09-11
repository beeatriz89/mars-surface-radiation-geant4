"""
Configuracao partilhada pelos scripts generate_pilot_macros.py e
estimate_required_n.py.

18 particulas: isotopos leves de H/He, leptoes, pi+/-, mu+/-, neutrao,
fotao, e 5 ioes representativos de grupos de Z (o mais abundante nos
espectros de GCR em cada grupo):
  Z3-5   -> Z5  (B-11, ~80% abundancia isotopica do boro)
  Z6-8   -> Z6  (C-12)
  Z9-13  -> Z12 (Mg-24, ~79%)
  Z14-24 -> Z14 (Si-28, ~92%)
  Z>24   -> Z26 (Fe-56, ~92%)
"""

ENERGIES_MEV = [0.1, 0.2, 0.5, 0.8, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 20, 30, 50, 80, 100, 200, 300, 500, 800,

1000, 2000, 3000, 5000, 8000, 10000, 20000, 30000, 50000, 80000, 100000, 200000, 300000, 500000, 800000, 1000000]

# (nome_do_ficheiro, comando_de_particula_sem_energia, e_iao, nome_reportado_pelo_G4)
#
# "nome_reportado_pelo_G4" e o que aparece na coluna "Particle" do CSV
# (BeamConfig::GetParticleName()). Para ioes e o nome devolvido por
# G4IonTable::GetIon(Z,A,0)->GetParticleName(). Os ioes leves (Z<=2) tem
# nomes especiais no Geant4: "deuteron", "triton", "He3", "alpha".
PARTICLES = [
    ("H2_deuteron", "/source/ion 1 2",         True,  "deuteron", 2),
    ("H3_triton",   "/source/ion 1 3",         True,  "triton",   3),
    ("H1_proton",   "/source/particle proton", False, "proton",   1),
    ("He3",         "/source/ion 2 3",         True,  "He3",      3),
    ("He4_alpha",   "/source/ion 2 4",         True,  "alpha",    4),
    ("pi+",         "/source/particle pi+",    False, "pi+",      1),
    ("pi-",         "/source/particle pi-",    False, "pi-",      1),
    ("mu+",         "/source/particle mu+",    False, "mu+",      1),
    ("mu-",         "/source/particle mu-",    False, "mu-",      1),
    ("e+",          "/source/particle e+",     False, "e+",       1),
    ("e-",          "/source/particle e-",     False, "e-",       1),
    ("neutron",     "/source/particle neutron",False, "neutron",  1),
    ("gamma",       "/source/particle gamma",  False, "gamma",    1),
    ("Z4_Be9",      "/source/ion 4 9",         True,  "Be9",      9),
    ("Z7_N14",      "/source/ion 7 14",        True,  "N14",      14),
    ("Z11_Na23",    "/source/ion 11 23",       True,  "Na23",     23),
    ("Z19_K39",     "/source/ion 19 39",       True,  "K39",      39),
    ("Z26_Fe56",    "/source/ion 26 56",       True,  "Fe56",     56),

]
