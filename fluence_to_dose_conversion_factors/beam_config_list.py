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

ENERGIES_MEV = [10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000, 50000, 100000]

# (nome_do_ficheiro, comando_de_particula_sem_energia, e_iao, nome_reportado_pelo_G4)
#
# "nome_reportado_pelo_G4" e o que aparece na coluna "Particle" do CSV
# (BeamConfig::GetParticleName()). Para ioes e o nome devolvido por
# G4IonTable::GetIon(Z,A,0)->GetParticleName(). Os ioes leves (Z<=2) tem
# nomes especiais no Geant4: "deuteron", "triton", "He3", "alpha".
PARTICLES = [
    ("H2_deuteron", "/source/ion 1 2",         True,  "deuteron"),
    ("H3_triton",   "/source/ion 1 3",         True,  "triton"),
    ("H1_proton",   "/source/particle proton", False, "proton"),
    ("He3",         "/source/ion 2 3",         True,  "He3"),
    ("He4_alpha",   "/source/ion 2 4",         True,  "alpha"),
    ("pi+",         "/source/particle pi+",    False, "pi+"),
    ("pi-",         "/source/particle pi-",    False, "pi-"),
    ("mu+",         "/source/particle mu+",    False, "mu+"),
    ("mu-",         "/source/particle mu-",    False, "mu-"),
    ("e+",          "/source/particle e+",     False, "e+"),
    ("e-",          "/source/particle e-",     False, "e-"),
    ("neutron",     "/source/particle neutron",False, "neutron"),
    ("gamma",       "/source/particle gamma",  False, "gamma"),
    ("Z5_B11",      "/source/ion 5 11",        True,  "B11"),
    ("Z6_C12",      "/source/ion 6 12",        True,  "C12"),
    ("Z12_Mg24",    "/source/ion 12 24",       True,  "Mg24"),
    ("Z14_Si28",    "/source/ion 14 28",       True,  "Si28"),
    ("Z26_Fe56",    "/source/ion 26 56",       True,  "Fe56"),
]
