# Mars Radiation and Dosimetry

This repository contains the source code developed for the MSc thesis:

**Effective dose estimation on Mars and the Moon using simulations and radiobiological modeling**

[![DOI](https://zenodo.org/badge/1365135365.svg)](https://doi.org/10.5281/zenodo.22700705)

The project focuses on the simulation and dosimetric assessment of radiation exposure at the Martian surface using Monte Carlo methods and the Geant4 toolkit.

## Contents

* **Geant4** — Monte Carlo simulation code for radiation transport through the Martian environment and computational phantoms.
* **Python** — Scripts developed to automate the Monte Carlo simulations.

## References and acknowledgements

The implementation of the Martian radiation environment and particle flux calculations was based on the methods described by Matthiä and Berger (2017) [1].

The computational phantom implementation was based on the `advanced/ICRP145HumanPhantom` example provided with the Geant4 distribution. The reference computational phantoms are described in ICRP Publication 145 [2]. The `POLY2TET` software used in the preparation of the tetrahedral phantom geometry was developed by Han et al. (2020) [3].

The Geant4 toolkit is described by Agostinelli et al. (2003) [4].

The code in this repository contains adaptations and additional implementations developed for the purposes of the present thesis.

### References

[1] D. Matthiä and T. Berger, *The radiation environment on the surface of Mars – Numerical calculations of the galactic component with GEANT4 / PLANETOCOSMICS*, Life Sciences in Space Research, 14, 57–63 (2017).
https://doi.org/10.1016/j.lssr.2017.03.005

[2] ICRP, *Adult Mesh-Type Reference Computational Phantoms*, ICRP Publication 145, 2020.
https://doi.org/10.1177/0146645320913787

[3] H. Han, Y. S. Yeom, C. Choi, S. Moon, B. Shin, S. Ha, and C. H. Kim, *POLY2TET: A computer program for conversion of computational human phantoms from polygonal mesh to tetrahedral mesh*, Journal of Radiological Protection, 40(4), 962–979 (2020).
https://doi.org/10.1088/1361-6498/abb360

[4] S. Agostinelli et al., *Geant4—a simulation toolkit*, Nuclear Instruments and Methods in Physics Research Section A, 506(3), 250–303 (2003).
https://doi.org/10.1016/S0168-9002(03)01368-8

The corresponding BibTeX entries are provided in [`references.bib`](references.bib).

## Software

* Geant4
* Python

## Author

**Maria Beatriz Costa**

MSc in Medical Physics
Faculty of Sciences, University of Porto
2026

## License

This project is licensed under the Apache License 2.0. See the `LICENSE` file for details.


### Use of Generative AI

Generative AI tools, namely Claude (Claude Sonnet 4 and Claude Sonnet 5) and ChatGPT, were used between November 2025 and August 2026 to support the development, revision, debugging, and problem-solving of the simulation codes in this repository. The author retains full responsibility for the design, implementation, validation, execution, and interpretation of the simulations, as well as for the final content of the work.
