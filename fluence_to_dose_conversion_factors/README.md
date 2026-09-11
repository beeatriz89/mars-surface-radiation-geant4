# Conversion Factors (Geant4) — dose absorvida e dose equivalente em tecido

Feixe plano, mono-direcional, perpendicular (geometria AP), incidente
numa placa de tecido de 18 mm. Dose equivalente calculada via Q(L)
(ICRP 60 / ICRP 123), não w_R fixo — captura o LET real de cada
partícula (incluindo secundários) ao longo do percurso na placa.

## Compilar

```bash
mkdir build && cd build
cmake ..
make -j
```

(Precisas de ter o Geant4 instalado/configurado no ambiente, com a
physics list `FTFP_INCLXX_HP` disponível — está incluída no build
standard do Geant4 11.x.)

## Fluxo de trabalho: piloto -> estimar N -> produção

1. **Gerar macros piloto** (N pequeno, só para estimar a incerteza):
   ```bash
   python3 generate_pilot_macros.py
   ```
   Cria `macros_pilot/<particula>.mac`, um por cada uma das 18 partículas
   em `beam_config_list.py`, cada um a percorrer as 13 energias.

2. **Correr o piloto** (sequencial, em background, sobrevive a fechares o ssh):
   ```bash
   nohup ./run_pilot.sh > run_pilot_master.log 2>&1 &
   disown
   tail -f run_pilot_master.log        # acompanhar progresso
   ```
   Resultados em `pilot_results.csv`.

3. **Calcular o N necessário** para uma incerteza-alvo (default 1%,
   editável em `estimate_required_n.py`):
   ```bash
   python3 estimate_required_n.py
   ```
   Cria `macros_production/<particula>.mac` com o N calibrado por
   (partícula, energia), a partir da lei `N ~ 1/erro²`.

4. **Correr a produção**:
   ```bash
   nohup ./run_production.sh > run_production_master.log 2>&1 &
   disown
   ```
   Resultados finais em `conversion_factors.csv`, com colunas:
   `Particle, Energy_MeV, N_events, Area_cm2, Fluence_per_cm2,
   LET_doseAvg_keV_per_um, Q_doseAvg, Dose_Gy, Dose_relUncert_pct,
   DosePerFluence_Gy_cm2, H_Sv, H_relUncert_pct, HPerFluence_Sv_cm2`

## Notas

- **18 partículas** (`beam_config_list.py`): ²H, ³H, ¹H, ³He, ⁴He, π±, µ±,
  e±, nêutron, fotão, e 5 iões representativos de grupos de Z (Z5=B-11,
  Z6=C-12, Z12=Mg-24, Z14=Si-28, Z26=Fe-56). Edita esse ficheiro se
  precisares de outra lista.
- **13 energias**: 10, 20, 50, 100, 200, 500 MeV, 1, 2, 5, 10, 20, 50,
  100 GeV — também em `beam_config_list.py` (`ENERGIES_MEV`).
- **MT**: 8 threads por omissão (`/run/numberOfThreads 8` nas macros
  geradas, e no `main()`).
- Podes correr particulas/energias em paralelo em nós diferentes do
  cluster — basta que cada uma escreva para um `/output/csvFile`
  diferente e depois concatenares os CSVs (cuidado com cabeçalhos
  repetidos).
