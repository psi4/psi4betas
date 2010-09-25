#include "dcft.h"
#include "defines.h"
#include "libpsio/psio.hpp"
#include "psifiles.h"

namespace psi{ namespace dcft{

/**
 * Dumps the MO Basis density matrix in a DPD form.
 */
void
DCFTSolver::dump_density()
{
    _psio->open(PSIF_DCFT_DENSITY, PSIO_OPEN_OLD);
    _psio->open(PSIF_LIBTRANS_DPD, PSIO_OPEN_OLD);

    dpdbuf4 Laa, Lab, Lbb, Gaa, Gab, Gbb, I, L;
    dpdfile2 T_OO, T_oo, T_VV, T_vv;

    dpd_buf4_init(&Laa, PSIF_DCFT_DPD, 0, ID("[O,O]"), ID("[V,V]"),
                  ID("[O,O]"), ID("[V,V]"), 0, "Lambda <OO|VV>");
    dpd_buf4_init(&Lab, PSIF_DCFT_DPD, 0, ID("[O,o]"), ID("[V,v]"),
                  ID("[O,o]"), ID("[V,v]"), 0, "Lambda <Oo|Vv>");
    dpd_buf4_init(&Lbb, PSIF_DCFT_DPD, 0, ID("[o,o]"), ID("[v,v]"),
                  ID("[o,o]"), ID("[v,v]"), 0, "Lambda <oo|vv>");
    dpd_file2_init(&T_OO, PSIF_DCFT_DPD, 0,
                  _ints->DPD_ID('O'), _ints->DPD_ID('O'), "Tau <O|O>");
    dpd_file2_init(&T_oo, PSIF_DCFT_DPD, 0,
                  _ints->DPD_ID('o'), _ints->DPD_ID('o'), "Tau <o|o>");
    dpd_file2_init(&T_VV, PSIF_DCFT_DPD, 0,
                  _ints->DPD_ID('V'), _ints->DPD_ID('V'), "Tau <V|V>");
    dpd_file2_init(&T_vv, PSIF_DCFT_DPD, 0,
                  _ints->DPD_ID('v'), _ints->DPD_ID('v'), "Tau <v|v>");

    dpd_file2_mat_init(&T_OO);
    dpd_file2_mat_init(&T_oo);
    dpd_file2_mat_init(&T_VV);
    dpd_file2_mat_init(&T_vv);

    dpd_file2_mat_rd(&T_OO);
    dpd_file2_mat_rd(&T_oo);
    dpd_file2_mat_rd(&T_VV);
    dpd_file2_mat_rd(&T_vv);

    Matrix aOccOPDM(_nIrreps, _nAOccPI, _nAOccPI);
    Matrix bOccOPDM(_nIrreps, _nBOccPI, _nBOccPI);
    Matrix aVirOPDM(_nIrreps, _nAVirPI, _nAVirPI);
    Matrix bVirOPDM(_nIrreps, _nBVirPI, _nBVirPI);
    for(int h = 0; h < _nIrreps; ++h){
        for(int i = 0; i < _nAOccPI[h]; ++i){
            for(int j = 0; j < _nAOccPI[h]; ++j){
                aOccOPDM.set(h, i, j, (i == j ? 1.0 : 0.0) + T_OO.matrix[h][i][j]);
            }
        }
        for(int a = 0; a < _nAVirPI[h]; ++a){
            for(int b = 0; b < _nAVirPI[h]; ++b){
                aVirOPDM.set(h, a, b, T_VV.matrix[h][a][b]);
            }
        }
        for(int i = 0; i < _nBOccPI[h]; ++i){
            for(int j = 0; j < _nBOccPI[h]; ++j){
                bOccOPDM.set(h, i, j, (i == j ? 1.0 : 0.0) + T_oo.matrix[h][i][j]);
            }
        }
        for(int a = 0; a < _nBVirPI[h]; ++a){
            for(int b = 0; b < _nBVirPI[h]; ++b){
                bVirOPDM.set(h, a, b, T_vv.matrix[h][a][b]);
            }
        }
    }

    dpd_file2_close(&T_OO);
    dpd_file2_close(&T_oo);
    dpd_file2_close(&T_VV);
    dpd_file2_close(&T_vv);

    /*
     * The VVVV block
     */
    dpd_buf4_init(&Gaa, PSIF_DCFT_DENSITY, 0, ID("[V,V]"), ID("[V,V]"),
              ID("[V,V]"), ID("[V,V]"), 0, "Gamma <VV|VV>");
    for(int h = 0; h < _nIrreps; ++h){
        dpd_buf4_mat_irrep_init(&Gaa, h);
        dpd_buf4_mat_irrep_init(&Laa, h);
        dpd_buf4_mat_irrep_rd(&Laa, h);
        for(size_t ab = 0; ab < Gaa.params->rowtot[h]; ++ab){
            size_t a = Gaa.params->roworb[h][ab][0];
            int Ga = Gaa.params->psym[a];
            a -= Gaa.params->poff[Ga];
            size_t b = Gaa.params->roworb[h][ab][1];
            int Gb = Gaa.params->qsym[b];
            b -= Gaa.params->qoff[Gb];
            for(size_t cd = 0; cd < Gaa.params->coltot[h]; ++cd){
                double tpdm = 0.0;
                for(size_t ij = 0; ij < Laa.params->rowtot[h]; ++ij){
                    tpdm += 0.5 * Laa.matrix[h][ij][ab] * Laa.matrix[h][ij][cd];
                }
                size_t c = Gaa.params->colorb[h][cd][0];
                int Gc = Gaa.params->rsym[c];
                c -= Gaa.params->roff[Gc];
                size_t d = Gaa.params->colorb[h][cd][1];
                int Gd = Gaa.params->ssym[d];
                d -= Gaa.params->soff[Gd];
                if(Ga == Gc && Gb == Gd) tpdm += aVirOPDM(Ga, a, c) * aVirOPDM(Gb, b, d);
                if(Ga == Gd && Gb == Gc) tpdm -= aVirOPDM(Ga, a, d) * aVirOPDM(Gb, b, c);
                Gaa.matrix[h][ab][cd] = tpdm;
            }
        }
        dpd_buf4_mat_irrep_wrt(&Gaa, h);
        dpd_buf4_mat_irrep_close(&Gaa, h);
        dpd_buf4_mat_irrep_close(&Laa, h);
    }
    dpd_buf4_close(&Gaa);

    dpd_buf4_init(&Gab, PSIF_DCFT_DENSITY, 0, ID("[V,v]"), ID("[V,v]"),
              ID("[V,v]"), ID("[V,v]"), 0, "Gamma <Vv|Vv>");
    for(int h = 0; h < _nIrreps; ++h){
        dpd_buf4_mat_irrep_init(&Gab, h);
        dpd_buf4_mat_irrep_init(&Lab, h);
        dpd_buf4_mat_irrep_rd(&Lab, h);
        for(size_t ab = 0; ab < Gab.params->rowtot[h]; ++ab){
            size_t a = Gab.params->roworb[h][ab][0];
            int Ga = Gab.params->psym[a];
            a -= Gab.params->poff[Ga];
            size_t b = Gab.params->roworb[h][ab][1];
            int Gb = Gab.params->qsym[b];
            b -= Gab.params->qoff[Gb];
            for(size_t cd = 0; cd < Gab.params->coltot[h]; ++cd){
                double tpdm = 0.0;
                for(size_t ij = 0; ij < Lab.params->rowtot[h]; ++ij){
                    tpdm += Lab.matrix[h][ij][ab] * Lab.matrix[h][ij][cd];
                }
                size_t c = Gab.params->colorb[h][cd][0];
                int Gc = Gab.params->rsym[c];
                c -= Gab.params->roff[Gc];
                size_t d = Gab.params->colorb[h][cd][1];
                int Gd = Gab.params->ssym[d];
                d -= Gab.params->soff[Gd];
                if(Ga == Gc && Gb == Gd) tpdm += aVirOPDM(Ga, a, c) * bVirOPDM(Gb, b, d);
                Gab.matrix[h][ab][cd] = tpdm;
            }
        }
        dpd_buf4_mat_irrep_wrt(&Gab, h);
        dpd_buf4_mat_irrep_close(&Gab, h);
        dpd_buf4_mat_irrep_close(&Lab, h);
    }
    dpd_buf4_close(&Gab);

    dpd_buf4_init(&Gbb, PSIF_DCFT_DENSITY, 0, ID("[v,v]"), ID("[v,v]"),
              ID("[v,v]"), ID("[v,v]"), 0, "Gamma <vv|vv>");
    for(int h = 0; h < _nIrreps; ++h){
        dpd_buf4_mat_irrep_init(&Gbb, h);
        dpd_buf4_mat_irrep_init(&Lbb, h);
        dpd_buf4_mat_irrep_rd(&Lbb, h);
        for(size_t ab = 0; ab < Gbb.params->rowtot[h]; ++ab){
            size_t a = Gbb.params->roworb[h][ab][0];
            int Ga = Gbb.params->psym[a];
            a -= Gbb.params->poff[Ga];
            size_t b = Gbb.params->roworb[h][ab][1];
            int Gb = Gbb.params->qsym[b];
            b -= Gbb.params->qoff[Gb];
            for(size_t cd = 0; cd < Gbb.params->coltot[h]; ++cd){
                double tpdm = 0.0;
                for(size_t ij = 0; ij < Lbb.params->rowtot[h]; ++ij){
                    tpdm += 0.5 * Lbb.matrix[h][ij][ab] * Lbb.matrix[h][ij][cd];
                }
                size_t c = Gbb.params->colorb[h][cd][0];
                int Gc = Gbb.params->rsym[c];
                c -= Gbb.params->roff[Gc];
                size_t d = Gbb.params->colorb[h][cd][1];
                int Gd = Gbb.params->ssym[d];
                d -= Gbb.params->soff[Gd];
                if(Ga == Gc && Gb == Gd) tpdm += bVirOPDM(Ga, a, c) * bVirOPDM(Gb, b, d);
                if(Ga == Gd && Gb == Gc) tpdm -= bVirOPDM(Ga, a, d) * bVirOPDM(Gb, b, c);
                Gbb.matrix[h][ab][cd] = tpdm;
            }
        }
        dpd_buf4_mat_irrep_wrt(&Gbb, h);
        dpd_buf4_mat_irrep_close(&Gbb, h);
        dpd_buf4_mat_irrep_close(&Lbb, h);
    }
    dpd_buf4_close(&Gbb);

    /*
     * The OOOO  block
     */
    dpd_buf4_init(&Gaa, PSIF_DCFT_DENSITY, 0, ID("[O,O]"), ID("[O,O]"),
              ID("[O,O]"), ID("[O,O]"), 0, "Gamma <OO|OO>");
    for(int h = 0; h < _nIrreps; ++h){
        dpd_buf4_mat_irrep_init(&Gaa, h);
        dpd_buf4_mat_irrep_init(&Laa, h);
        dpd_buf4_mat_irrep_rd(&Laa, h);
        for(size_t ij = 0; ij < Gaa.params->rowtot[h]; ++ij){
            size_t i = Gaa.params->roworb[h][ij][0];
            int Gi = Gaa.params->psym[i];
            i -= Gaa.params->poff[Gi];
            size_t j = Gaa.params->roworb[h][ij][1];
            int Gj = Gaa.params->qsym[j];
            j -= Gaa.params->qoff[Gj];
            for(size_t kl = 0; kl < Gaa.params->coltot[h]; ++kl){
                double tpdm = 0.0;
                for(size_t ab = 0; ab < Laa.params->coltot[h]; ++ab){
                    tpdm += 0.5 * Laa.matrix[h][ij][ab] * Laa.matrix[h][kl][ab];
                }
                size_t k = Gaa.params->colorb[h][kl][0];
                int Gk = Gaa.params->rsym[k];
                k -= Gaa.params->roff[Gk];
                size_t l = Gaa.params->colorb[h][kl][1];
                int Gl = Gaa.params->ssym[l];
                l -= Gaa.params->soff[Gl];
                if(Gi == Gk && Gj == Gl) tpdm += aOccOPDM(Gi, i, k) * aOccOPDM(Gj, j, l);
                if(Gi == Gl && Gj == Gk) tpdm -= aOccOPDM(Gi, i, l) * aOccOPDM(Gj, j, k);
                Gaa.matrix[h][ij][kl] = tpdm;
            }
        }
        dpd_buf4_mat_irrep_wrt(&Gaa, h);
        dpd_buf4_mat_irrep_close(&Gaa, h);
        dpd_buf4_mat_irrep_close(&Laa, h);
    }
    dpd_buf4_close(&Gaa);


    dpd_buf4_init(&Gab, PSIF_DCFT_DENSITY, 0, ID("[O,o]"), ID("[O,o]"),
              ID("[O,o]"), ID("[O,o]"), 0, "Gamma <Oo|Oo>");
    for(int h = 0; h < _nIrreps; ++h){
        dpd_buf4_mat_irrep_init(&Gab, h);
        dpd_buf4_mat_irrep_init(&Lab, h);
        dpd_buf4_mat_irrep_rd(&Lab, h);
        for(size_t ij = 0; ij < Gab.params->rowtot[h]; ++ij){
            size_t i = Gab.params->roworb[h][ij][0];
            int Gi = Gab.params->psym[i];
            i -= Gab.params->poff[Gi];
            size_t j = Gab.params->roworb[h][ij][1];
            int Gj = Gab.params->qsym[j];
            j -= Gab.params->qoff[Gj];
            for(size_t kl = 0; kl < Gab.params->coltot[h]; ++kl){
                double tpdm = 0.0;
                for(size_t ab = 0; ab < Lab.params->coltot[h]; ++ab){
                    tpdm += Lab.matrix[h][ij][ab] * Lab.matrix[h][kl][ab];
                }
                size_t k = Gab.params->colorb[h][kl][0];
                int Gk = Gab.params->rsym[k];
                k -= Gab.params->roff[Gk];
                size_t l = Gab.params->colorb[h][kl][1];
                int Gl = Gab.params->ssym[l];
                l -= Gab.params->soff[Gl];
                if(Gi == Gk && Gj == Gl) tpdm += aOccOPDM(Gi, i, k) * bOccOPDM(Gj, j, l);
                Gab.matrix[h][ij][kl] = tpdm;
            }
        }
        dpd_buf4_mat_irrep_wrt(&Gab, h);
        dpd_buf4_mat_irrep_close(&Gab, h);
        dpd_buf4_mat_irrep_close(&Lab, h);
    }
    dpd_buf4_close(&Gab);

    dpd_buf4_init(&Gbb, PSIF_DCFT_DENSITY, 0, ID("[o,o]"), ID("[o,o]"),
              ID("[o,o]"), ID("[o,o]"), 0, "Gamma <oo|oo>");
    for(int h = 0; h < _nIrreps; ++h){
        dpd_buf4_mat_irrep_init(&Gbb, h);
        dpd_buf4_mat_irrep_init(&Lbb, h);
        dpd_buf4_mat_irrep_rd(&Lbb, h);
        for(size_t ij = 0; ij < Gbb.params->rowtot[h]; ++ij){
            size_t i = Gbb.params->roworb[h][ij][0];
            int Gi = Gbb.params->psym[i];
            i -= Gbb.params->poff[Gi];
            size_t j = Gbb.params->roworb[h][ij][1];
            int Gj = Gbb.params->qsym[j];
            j -= Gbb.params->qoff[Gj];
            for(size_t kl = 0; kl < Gbb.params->coltot[h]; ++kl){
                double tpdm = 0.0;
                for(size_t ab = 0; ab < Lbb.params->coltot[h]; ++ab){
                    tpdm += 0.5 * Lbb.matrix[h][ij][ab] * Lbb.matrix[h][kl][ab];
                }
                size_t k = Gbb.params->colorb[h][kl][0];
                int Gk = Gbb.params->rsym[k];
                k -= Gbb.params->roff[Gk];
                size_t l = Gbb.params->colorb[h][kl][1];
                int Gl = Gbb.params->ssym[l];
                l -= Gbb.params->soff[Gl];
                if(Gi == Gk && Gj == Gl) tpdm += bOccOPDM(Gi, i, k) * bOccOPDM(Gj, j, l);
                if(Gi == Gl && Gj == Gk) tpdm -= bOccOPDM(Gi, i, l) * bOccOPDM(Gj, j, k);
                Gbb.matrix[h][ij][kl] = tpdm;
            }
        }
        dpd_buf4_mat_irrep_wrt(&Gbb, h);
        dpd_buf4_mat_irrep_close(&Gbb, h);
        dpd_buf4_mat_irrep_close(&Lbb, h);
    }
    dpd_buf4_close(&Gbb);

    /*
     * The OOVV and VVOO blocks
     */
    dpd_buf4_init(&Gaa, PSIF_DCFT_DPD, 0, ID("[O,O]"), ID("[V,V]"),
              ID("[O,O]"), ID("[V,V]"), 0, "Lambda <OO|VV>");
    dpd_buf4_copy(&Gaa, PSIF_DCFT_DENSITY, "Gamma <OO|VV>");
    dpd_buf4_sort(&Gaa, PSIF_DCFT_DENSITY, rspq, ID("[V,V]"), ID("[O,O]"), "Gamma <VV|OO>");
    dpd_buf4_close(&Gaa);

    dpd_buf4_init(&Gab, PSIF_DCFT_DPD, 0, ID("[O,o]"), ID("[V,v]"),
              ID("[O,o]"), ID("[V,v]"), 0, "Lambda <Oo|Vv>");
    dpd_buf4_copy(&Gab, PSIF_DCFT_DENSITY, "Gamma <Oo|Vv>");
    dpd_buf4_sort(&Gab, PSIF_DCFT_DENSITY, rspq, ID("[V,v]"), ID("[O,o]"), "Gamma <Vv|Oo>");
    dpd_buf4_close(&Gab);

    dpd_buf4_init(&Gbb, PSIF_DCFT_DPD, 0, ID("[o,o]"), ID("[v,v]"),
              ID("[o,o]"), ID("[v,v]"), 0, "Lambda <oo|vv>");
    dpd_buf4_copy(&Gbb, PSIF_DCFT_DENSITY, "Gamma <oo|vv>");
    dpd_buf4_sort(&Gbb, PSIF_DCFT_DENSITY, rspq, ID("[v,v]"), ID("[o,o]"), "Gamma <vv|oo>");
    dpd_buf4_close(&Gbb);

    dpd_buf4_init(&I, PSIF_LIBTRANS_DPD, 0, ID("[O,o]"), ID("[V,v]"),
                      ID("[O,o]"), ID("[V,v]"), 0, "MO Ints <Oo|Vv>");
    dpd_buf4_sort(&I, PSIF_LIBTRANS_DPD, rspq, ID("[V,v]"), ID("[O,o]"), "MO Ints <Vv|Oo>");
    dpd_buf4_close(&I);

    dpd_buf4_close(&Laa);
    dpd_buf4_close(&Lab);
    dpd_buf4_close(&Lbb);

    /*
     * The OVOV block
     */
    dpdbuf4 Laaaa, Laabb, Labba, Lbaab, Lbbbb, Taa, Tab, Tba, Tbb;

    dpd_buf4_init(&Gaa, PSIF_DCFT_DENSITY, 0, ID("[O,V]"), ID("[O,V]"),
                  ID("[O,V]"), ID("[O,V]"), 0, "Gamma <OV|OV>");
    dpd_buf4_init(&Laa, PSIF_DCFT_DPD, 0, ID("[O,V]"), ID("[O,V]"),
                  ID("[O,V]"), ID("[O,V]"), 0, "Lambda (OV|OV)");
    dpd_buf4_copy(&Laa, PSIF_DCFT_DPD, "Temp (OV|OV)");
    dpd_buf4_init(&Taa, PSIF_DCFT_DPD, 0, ID("[O,V]"), ID("[O,V]"),
                  ID("[O,V]"), ID("[O,V]"), 0, "Temp (OV|OV)");
    dpd_contract444(&Laa, &Taa, &Gaa, 0, 0, -1.0, 0.0);
    dpd_buf4_close(&Laa);
    dpd_buf4_close(&Taa);
    dpd_buf4_init(&Lab, PSIF_DCFT_DPD, 0, ID("[O,V]"), ID("[o,v]"),
                  ID("[O,V]"), ID("[o,v]"), 0, "Lambda (OV|ov)");
    dpd_buf4_copy(&Lab, PSIF_DCFT_DPD, "Temp (OV|ov)");
    dpd_buf4_init(&Tab, PSIF_DCFT_DPD, 0, ID("[O,V]"), ID("[o,v]"),
                  ID("[O,V]"), ID("[o,v]"), 0, "Temp (OV|ov)");
    dpd_contract444(&Lab, &Tab, &Gaa, 0, 0, -1.0, 1.0);
    dpd_buf4_close(&Lab);
    dpd_buf4_close(&Tab);
    dpd_buf4_close(&Gaa);

    dpd_buf4_init(&Tab, PSIF_DCFT_DPD, 0, ID("[O,V]"), ID("[o,v]"),
                  ID("[O,V]"), ID("[o,v]"), 0, "Temp (OV|ov)");
    dpd_buf4_init(&Tba, PSIF_DCFT_DPD, 0, ID("[o,v]"), ID("[O,V]"),
                  ID("[o,v]"), ID("[O,V]"), 0, "Temp (ov|OV)");
    dpd_buf4_init(&Lab, PSIF_DCFT_DPD, 0, ID("[O,V]"), ID("[o,v]"),
                  ID("[O,V]"), ID("[o,v]"), 0, "Lambda (OV|ov)");
    dpd_buf4_init(&Laa, PSIF_DCFT_DPD, 0, ID("[O,V]"), ID("[O,V]"),
                  ID("[O,V]"), ID("[O,V]"), 0, "Lambda (OV|OV)");
    dpd_buf4_init(&Lbb, PSIF_DCFT_DPD, 0, ID("[o,v]"), ID("[o,v]"),
                  ID("[o,v]"), ID("[o,v]"), 0, "Lambda (ov|ov)");
    dpd_contract444(&Laa, &Lab, &Tab, 0, 1, 1.0, 0.0);
    dpd_contract444(&Lab, &Lbb, &Tab, 0, 0, 1.0, 1.0);
    dpd_contract444(&Lab, &Laa, &Tba, 1, 0, 1.0, 0.0);
    dpd_contract444(&Lbb, &Laa, &Tba, 0, 0, 1.0, 1.0);
    dpd_buf4_close(&Laa);
    dpd_buf4_close(&Lab);
    dpd_buf4_close(&Lbb);
    dpd_buf4_sort(&Tab, PSIF_DCFT_DENSITY, psrq, ID("[O,v]"), ID("[o,V]"), "Gamma <Ov|oV>");
    dpd_buf4_sort(&Tba, PSIF_DCFT_DENSITY, psrq, ID("[o,V]"), ID("[O,v]"), "Gamma <oV|Ov>");
    dpd_buf4_close(&Tab);
    dpd_buf4_close(&Tba);

    dpd_buf4_init(&Gbb, PSIF_DCFT_DENSITY, 0, ID("[o,v]"), ID("[o,v]"),
                  ID("[o,v]"), ID("[o,v]"), 0, "Gamma <ov|ov>");
    dpd_buf4_init(&Lbb, PSIF_DCFT_DPD, 0, ID("[o,v]"), ID("[o,v]"),
                  ID("[o,v]"), ID("[o,v]"), 0, "Lambda (ov|ov)");
    dpd_buf4_copy(&Lbb, PSIF_DCFT_DPD, "Temp (ov|ov)");
    dpd_buf4_init(&Tbb, PSIF_DCFT_DPD, 0, ID("[o,v]"), ID("[o,v]"),
                  ID("[o,v]"), ID("[o,v]"), 0, "Temp (ov|ov)");
    dpd_contract444(&Lbb, &Tbb, &Gbb, 0, 0, -1.0, 0.0);
    dpd_buf4_close(&Lbb);
    dpd_buf4_close(&Tbb);
    dpd_buf4_init(&Lab, PSIF_DCFT_DPD, 0, ID("[O,V]"), ID("[O,V]"),
                  ID("[o,v]"), ID("[o,v]"), 0, "Lambda (OV|ov)");
    dpd_buf4_copy(&Lab, PSIF_DCFT_DPD, "Temp (OV|ov)");
    dpd_buf4_init(&Tab, PSIF_DCFT_DPD, 0, ID("[o,v]"), ID("[o,v]"),
                  ID("[o,v]"), ID("[o,v]"), 0, "Temp (OV|ov)");
    dpd_contract444(&Lab, &Tab, &Gbb, 1, 1, -1.0, 1.0);
    dpd_buf4_close(&Lab);
    dpd_buf4_close(&Tab);
    dpd_buf4_close(&Gbb);

    /*
     * As a sanity check, compute the energy from the density matrices.
     */
    Matrix aH(_soH);
    Matrix bH(_soH);
    aH.transform(_Ca);
    bH.transform(_Cb);
    double hCoreEnergy = 0.0;
    for(int h = 0; h < _nIrreps; ++h){
        for(int p = 0; p < aOccOPDM.rowspi()[h]; ++p){
            for(int q = 0; q < aOccOPDM.colspi()[h]; ++q){
                hCoreEnergy += aH.get(h, p, q) * aOccOPDM.get(h, p, q);
            }
        }
        for(int p = 0; p < bOccOPDM.rowspi()[h]; ++p){
            for(int q = 0; q < bOccOPDM.colspi()[h]; ++q){
                hCoreEnergy += bH.get(h, p, q) * bOccOPDM.get(h, p, q);
            }
        }
        for(int p = 0; p < aVirOPDM.rowspi()[h]; ++p){
            for(int q = 0; q < aVirOPDM.colspi()[h]; ++q){
                hCoreEnergy += aH.get(h, p + _nAOccPI[h], q + _nAOccPI[h]) * aVirOPDM.get(h, p, q);
            }
        }
        for(int p = 0; p < bVirOPDM.rowspi()[h]; ++p){
            for(int q = 0; q < bVirOPDM.colspi()[h]; ++q){
                hCoreEnergy += bH.get(h, p + _nBOccPI[h], q + _nBOccPI[h]) * bVirOPDM.get(h, p, q);
            }
        }
    }

    fprintf(outfile, "\tOne electron energy  = %16.10f\n", hCoreEnergy);

    // OOVV
    double OOVVEnergy = 0.0;
    dpd_buf4_init(&L, PSIF_DCFT_DENSITY, 0, ID("[O,O]"), ID("[V,V]"),
                      ID("[O,O]"), ID("[V,V]"), 0, "Gamma <OO|VV>");
    dpd_buf4_init(&I, PSIF_LIBTRANS_DPD, 0, ID("[O,O]"), ID("[V,V]"),
                      ID("[O,O]"), ID("[V,V]"), 1, "MO Ints <OO|VV>");
    OOVVEnergy += 0.25* dpd_buf4_dot(&I, &L);
    dpd_buf4_close(&I);
    dpd_buf4_close(&L);

    dpd_buf4_init(&L, PSIF_DCFT_DENSITY, 0, ID("[O,o]"), ID("[V,v]"),
                      ID("[O,o]"), ID("[V,v]"), 0, "Gamma <Oo|Vv>");
    dpd_buf4_init(&I, PSIF_LIBTRANS_DPD, 0, ID("[O,o]"), ID("[V,v]"),
                      ID("[O,o]"), ID("[V,v]"), 0, "MO Ints <Oo|Vv>");
    OOVVEnergy += dpd_buf4_dot(&I, &L);
    dpd_buf4_close(&I);
    dpd_buf4_close(&L);

    dpd_buf4_init(&L, PSIF_DCFT_DENSITY, 0, ID("[o,o]"), ID("[v,v]"),
                      ID("[o,o]"), ID("[v,v]"), 0, "Gamma <oo|vv>");
    dpd_buf4_init(&I, PSIF_LIBTRANS_DPD, 0, ID("[o,o]"), ID("[v,v]"),
                      ID("[o,o]"), ID("[v,v]"), 1, "MO Ints <oo|vv>");
    OOVVEnergy += 0.25 * dpd_buf4_dot(&I, &L);
    dpd_buf4_close(&I);
    dpd_buf4_close(&L);

    fprintf(outfile, "\tOOVV Energy          = %16.10f\n", OOVVEnergy);

    // VVOO
    double VVOOEnergy = 0.0;
    dpd_buf4_init(&L, PSIF_DCFT_DENSITY, 0, ID("[V,V]"), ID("[O,O]"),
                      ID("[V,V]"), ID("[O,O]"), 0, "Gamma <VV|OO>");
    dpd_buf4_init(&I, PSIF_LIBTRANS_DPD, 0, ID("[V,V]"), ID("[O,O]"),
                      ID("[V,V]"), ID("[O,O]"), 1, "MO Ints <VV|OO>");
    VVOOEnergy += 0.25* dpd_buf4_dot(&I, &L);
    dpd_buf4_close(&I);
    dpd_buf4_close(&L);

    dpd_buf4_init(&L, PSIF_DCFT_DENSITY, 0, ID("[V,v]"), ID("[O,o]"),
                      ID("[V,v]"), ID("[O,o]"), 0, "Gamma <Vv|Oo>");
    dpd_buf4_init(&I, PSIF_LIBTRANS_DPD, 0, ID("[V,v]"), ID("[O,o]"),
                      ID("[V,v]"), ID("[O,o]"), 0, "MO Ints <Vv|Oo>");
    VVOOEnergy += dpd_buf4_dot(&I, &L);
    dpd_buf4_close(&I);
    dpd_buf4_close(&L);

    dpd_buf4_init(&L, PSIF_DCFT_DENSITY, 0, ID("[v,v]"), ID("[o,o]"),
                      ID("[v,v]"), ID("[o,o]"), 0, "Gamma <vv|oo>");
    dpd_buf4_init(&I, PSIF_LIBTRANS_DPD, 0, ID("[v,v]"), ID("[o,o]"),
                      ID("[v,v]"), ID("[o,o]"), 1, "MO Ints <vv|oo>");
    VVOOEnergy += 0.25 * dpd_buf4_dot(&I, &L);
    dpd_buf4_close(&I);
    dpd_buf4_close(&L);

    fprintf(outfile, "\tVVOO Energy          = %16.10f\n", VVOOEnergy);

    // VVVV
    double VVVVEnergy = 0.0;
    dpd_buf4_init(&L, PSIF_DCFT_DENSITY, 0, ID("[V,V]"), ID("[V,V]"),
                      ID("[V,V]"), ID("[V,V]"), 0, "Gamma <VV|VV>");
    dpd_buf4_init(&I, PSIF_LIBTRANS_DPD, 0, ID("[V,V]"), ID("[V,V]"),
                      ID("[V,V]"), ID("[V,V]"), 1, "MO Ints <VV|VV>");
    VVVVEnergy += 0.25* dpd_buf4_dot(&I, &L);
fprintf(outfile, "testaa = %16.10f\n", 0.25*dpd_buf4_dot(&I, &L));
    dpd_buf4_close(&I);
    dpd_buf4_close(&L);

    dpd_buf4_init(&L, PSIF_DCFT_DENSITY, 0, ID("[V,v]"), ID("[V,v]"),
                      ID("[V,v]"), ID("[V,v]"), 0, "Gamma <Vv|Vv>");
    dpd_buf4_init(&I, PSIF_LIBTRANS_DPD, 0, ID("[V,v]"), ID("[V,v]"),
                      ID("[V,v]"), ID("[V,v]"), 0, "MO Ints <Vv|Vv>");
    VVVVEnergy += dpd_buf4_dot(&I, &L);
fprintf(outfile, "testab = %16.10f\n", dpd_buf4_dot(&I, &L));
    dpd_buf4_close(&I);
    dpd_buf4_close(&L);

    dpd_buf4_init(&L, PSIF_DCFT_DENSITY, 0, ID("[v,v]"), ID("[v,v]"),
                      ID("[v,v]"), ID("[v,v]"), 0, "Gamma <vv|vv>");
    dpd_buf4_init(&I, PSIF_LIBTRANS_DPD, 0, ID("[v,v]"), ID("[v,v]"),
                      ID("[v,v]"), ID("[v,v]"), 1, "MO Ints <vv|vv>");
    VVVVEnergy += 0.25 * dpd_buf4_dot(&I, &L);
fprintf(outfile, "testbb = %16.10f\n", 0.25*dpd_buf4_dot(&I, &L));
    dpd_buf4_close(&I);
    dpd_buf4_close(&L);

    fprintf(outfile, "\tVVVV Energy          = %16.10f\n", VVVVEnergy);

    // OOOO
    double OOOOEnergy = 0.0;
    dpd_buf4_init(&L, PSIF_DCFT_DENSITY, 0, ID("[O,O]"), ID("[O,O]"),
                      ID("[O,O]"), ID("[O,O]"), 0, "Gamma <OO|OO>");
    dpd_buf4_init(&I, PSIF_LIBTRANS_DPD, 0, ID("[O,O]"), ID("[O,O]"),
                      ID("[O,O]"), ID("[O,O]"), 1, "MO Ints <OO|OO>");
    OOOOEnergy += 0.25* dpd_buf4_dot(&I, &L);
fprintf(outfile, "testaa = %16.10f\n", 0.25*dpd_buf4_dot(&I, &L));
    dpd_buf4_close(&I);
    dpd_buf4_close(&L);

    dpd_buf4_init(&L, PSIF_DCFT_DENSITY, 0, ID("[O,o]"), ID("[O,o]"),
                      ID("[O,o]"), ID("[O,o]"), 0, "Gamma <Oo|Oo>");
    dpd_buf4_init(&I, PSIF_LIBTRANS_DPD, 0, ID("[O,o]"), ID("[O,o]"),
                      ID("[O,o]"), ID("[O,o]"), 0, "MO Ints <Oo|Oo>");
    OOOOEnergy += dpd_buf4_dot(&I, &L);

fprintf(outfile, "testab = %16.10f\n", dpd_buf4_dot(&I, &L));
    dpd_buf4_close(&I);
    dpd_buf4_close(&L);

    dpd_buf4_init(&L, PSIF_DCFT_DENSITY, 0, ID("[o,o]"), ID("[o,o]"),
                      ID("[o,o]"), ID("[o,o]"), 0, "Gamma <oo|oo>");
    dpd_buf4_init(&I, PSIF_LIBTRANS_DPD, 0, ID("[o,o]"), ID("[o,o]"),
                      ID("[o,o]"), ID("[o,o]"), 1, "MO Ints <oo|oo>");
    OOOOEnergy += 0.25 * dpd_buf4_dot(&I, &L);
fprintf(outfile, "testbb = %16.10f\n", 0.25*dpd_buf4_dot(&I, &L));
    dpd_buf4_close(&I);
    dpd_buf4_close(&L);

    fprintf(outfile, "\tOOOO Energy          = %16.10f\n", OOOOEnergy);

    // OVOV
    double OVOVEnergy = 0.0;
    dpd_buf4_init(&L, PSIF_DCFT_DENSITY, 0, ID("[O,V]"), ID("[O,V]"),
                      ID("[O,V]"), ID("[O,V]"), 0, "Gamma <OV|OV>");
    dpd_buf4_init(&I, PSIF_LIBTRANS_DPD, 0, ID("[O,V]"), ID("[O,V]"),
                  ID("[O,V]"), ID("[O,V]"), 0, "MO Ints <OV|OV> - (OV|OV)");
    OVOVEnergy += dpd_buf4_dot(&I, &L);
fprintf(outfile, "testaa = %16.10f\n", dpd_buf4_dot(&I, &L));
    dpd_buf4_close(&I);
    dpd_buf4_close(&L);

    dpd_buf4_init(&L, PSIF_DCFT_DENSITY, 0, ID("[O,v]"), ID("[O,v]"),
                  ID("[O,v]"), ID("[O,v]"), 0, "Gamma <Ov|Ov>");
    dpd_buf4_init(&I, PSIF_LIBTRANS_DPD, 0, ID("[O,v]"), ID("[O,v]"),
                  ID("[O,v]"), ID("[O,v]"), 0, "MO Ints <Ov|Ov>");


    dpd_buf4_init(&I, PSIF_LIBTRANS_DPD, 0, ID("[O,V]"), ID("[o,v]"),
                  ID("[O,V]"), ID("[o,v]"), 0, "MO Ints (OV|ov)");
    dpd_buf4_sort(&I, PSIF_LIBTRANS_DPD, psrq, ID("[O,v]"), ID("[o,V]"), "MO Ints <Ov|oV>");
    dpd_buf4_close(&I);
    dpd_buf4_init(&I, PSIF_LIBTRANS_DPD, 0, ID("[O,v]"), ID("[o,V]"),
                  ID("[O,v]"), ID("[o,V]"), 0, "MO Ints <Ov|oV>");
    dpd_buf4_init(&L, PSIF_DCFT_DENSITY, 0, ID("[O,v]"), ID("[o,V]"),
                  ID("[O,v]"), ID("[o,V]"), 0, "Gamma <Ov|oV>");
    OVOVEnergy -= dpd_buf4_dot(&L, &I);
fprintf(outfile, "testab = %16.10f\n", -dpd_buf4_dot(&I, &L));
    dpd_buf4_close(&I);
    dpd_buf4_close(&L);

    dpd_buf4_init(&I, PSIF_LIBTRANS_DPD, 0, ID("[O,V]"), ID("[o,v]"),
                  ID("[O,V]"), ID("[o,v]"), 0, "MO Ints (OV|ov)");
    dpd_buf4_sort(&I, PSIF_LIBTRANS_DPD, rqps, ID("[o,V]"), ID("[O,v]"), "MO Ints <oV|Ov>");
    dpd_buf4_close(&I);
    dpd_buf4_init(&I, PSIF_LIBTRANS_DPD, 0, ID("[o,V]"), ID("[O,v]"),
                  ID("[o,V]"), ID("[O,v]"), 0, "MO Ints <oV|Ov>");
    dpd_buf4_init(&L, PSIF_DCFT_DENSITY, 0, ID("[o,V]"), ID("[O,v]"),
                  ID("[o,V]"), ID("[O,v]"), 0, "Gamma <oV|Ov>");
    OVOVEnergy -= dpd_buf4_dot(&L, &I);
fprintf(outfile, "testba = %16.10f\n", -dpd_buf4_dot(&I, &L));
    dpd_buf4_close(&I);
    dpd_buf4_close(&L);

    dpd_buf4_init(&L, PSIF_DCFT_DENSITY, 0, ID("[o,v]"), ID("[o,v]"),
                      ID("[o,v]"), ID("[o,v]"), 0, "Gamma <ov|ov>");
    dpd_buf4_init(&I, PSIF_LIBTRANS_DPD, 0, ID("[o,v]"), ID("[o,v]"),
                  ID("[o,v]"), ID("[o,v]"), 0, "MO Ints <ov|ov> - (ov|ov)");
    OVOVEnergy += dpd_buf4_dot(&I, &L);
fprintf(outfile, "testbb = %16.10f\n", dpd_buf4_dot(&I, &L));
    dpd_buf4_close(&I);
    dpd_buf4_close(&L);
    fprintf(outfile, "\tOVOV Energy          = %16.10f\n", OVOVEnergy);

    fprintf(outfile, "\tTotal Energy         = %16.10f\n", _eNuc + hCoreEnergy
            + OOOOEnergy + VVVVEnergy + OOVVEnergy + VVOOEnergy + OVOVEnergy);

    _psio->close(PSIF_DCFT_DENSITY, 1);
    _psio->close(PSIF_LIBTRANS_DPD, 1);
}


/**
 * Checks the n-representability of the density matrix by checking the positive
 * semidefiniteness of various matrices, used as constraints by Mazziotti
 */
void
DCFTSolver::check_n_representability()
{
    // This shouldn't be used!  Just some experimentation...
    return;
    dpdbuf4 Laa, Lab, Lbb, D, Q, G;
    dpdfile2 T_OO, T_oo, T_VV, T_vv;
    dpd_buf4_init(&Laa, PSIF_DCFT_DPD, 0, ID("[O,O]"), ID("[V,V]"),
                  ID("[O,O]"), ID("[V,V]"), 0, "Lambda <OO|VV>");
    dpd_buf4_init(&Lab, PSIF_DCFT_DPD, 0, ID("[O,o]"), ID("[V,v]"),
                  ID("[O,o]"), ID("[V,v]"), 0, "Lambda <Oo|Vv>");
    dpd_file2_init(&T_OO, PSIF_DCFT_DPD, 0,
                  _ints->DPD_ID('O'), _ints->DPD_ID('O'), "Tau <O|O>");
    dpd_file2_init(&T_oo, PSIF_DCFT_DPD, 0,
                  _ints->DPD_ID('o'), _ints->DPD_ID('o'), "Tau <o|o>");
    dpd_file2_init(&T_VV, PSIF_DCFT_DPD, 0,
                  _ints->DPD_ID('V'), _ints->DPD_ID('V'), "Tau <V|V>");
    dpd_file2_init(&T_vv, PSIF_DCFT_DPD, 0,
                  _ints->DPD_ID('v'), _ints->DPD_ID('v'), "Tau <v|v>");

    dpd_file2_mat_init(&T_OO);
    dpd_file2_mat_init(&T_oo);
    dpd_file2_mat_init(&T_VV);
    dpd_file2_mat_init(&T_vv);

    dpd_file2_mat_rd(&T_OO);
    dpd_file2_mat_rd(&T_oo);
    dpd_file2_mat_rd(&T_VV);
    dpd_file2_mat_rd(&T_vv);

    for(int h = 0; h < _nIrreps; ++h){
        int nOcc = _nAOccPI[h];
        int nVir = _nAVirPI[h];
        unsigned long int dim = _moPI[h];

        dpd_buf4_mat_irrep_init(&Laa, h);
        dpd_buf4_mat_irrep_init(&Lab, h);
        dpd_buf4_mat_irrep_rd(&Laa, h);
        dpd_buf4_mat_irrep_rd(&Lab, h);

        /*
         * Alpha - Alpha and Beta - Beta are in the same matrix
         */
        double **OPDM = block_matrix(dim, dim);
        for(int i = 0; i < nOcc; ++i){
            for(int j = 0; j < nOcc; ++j){
                OPDM[i][j] = ( i == j ? 1.0 : 0.0) + T_OO.matrix[h][i][j];
            }
        }
        for(int a = nOcc; a < dim; ++a){
            for(int b = nOcc; b < dim; ++b){
                OPDM[a][b] =  T_VV.matrix[h][a-nOcc][b-nOcc];
            }
        }
        double **TPDMaa = block_matrix(dim*dim, dim*dim);
        double **TPDMab = block_matrix(dim*dim, dim*dim);
        // The OOVV and VVOO elements of the TPDM
        for(int ij = 0; ij < Laa.params->rowtot[h]; ++ij){
            int i = Laa.params->roworb[h][ij][0];
            int j = Laa.params->roworb[h][ij][1];
            unsigned long int IJ = i * dim + j;
            for(int ab = 0; ab < Laa.params->coltot[h]; ++ab){
                int a = Laa.params->colorb[h][ab][0] + nOcc;
                int b = Laa.params->colorb[h][ab][1] + nOcc;
                unsigned long int AB = a * dim + b;
                TPDMaa[AB][IJ] = TPDMaa[IJ][AB] = Laa.matrix[h][ij][ab];
            }
        }
        // The OOOO elements of the TPDM
        for(int ij = 0; ij < Laa.params->rowtot[h]; ++ij){
            int i = Laa.params->roworb[h][ij][0];
            int j = Laa.params->roworb[h][ij][1];
            unsigned long int IJ = i * dim + j;
            for(int kl = 0; kl < Laa.params->rowtot[h]; ++kl){
                int k = Laa.params->roworb[h][kl][0];
                int l = Laa.params->roworb[h][kl][1];
                unsigned long int KL = k * dim + l;
                double AAval = 0.0;
                double ABval = 0.0;
                for(int ab = 0; ab < Laa.params->coltot[h]; ++ab){
                    AAval += Laa.matrix[h][ij][ab] * Laa.matrix[h][kl][ab];
                    ABval += Lab.matrix[h][ij][ab] * Lab.matrix[h][kl][ab];
                }
                TPDMaa[IJ][KL] = 0.5 * AAval;
                TPDMab[IJ][KL] = 0.5 * ABval;
            }
        }
        // The VVVV elements of the TPDM
        for(int ab = 0; ab < Laa.params->coltot[h]; ++ab){
            int a = Laa.params->colorb[h][ab][0] + nOcc;
            int b = Laa.params->colorb[h][ab][1] + nOcc;
            unsigned long int AB = a * dim + b;
            for(int cd = 0; cd < Laa.params->coltot[h]; ++cd){
                int c = Laa.params->colorb[h][cd][0] + nOcc;
                int d = Laa.params->colorb[h][cd][1] + nOcc;
                unsigned long int CD = c * dim + d;
                double AAval = 0.0;
                double ABval = 0.0;
                for(int ij = 0; ij < Laa.params->rowtot[h]; ++ij){
                    AAval += Laa.matrix[h][ij][ab] * Laa.matrix[h][ij][cd];
                    ABval += Lab.matrix[h][ij][ab] * Lab.matrix[h][ij][cd];
                }
                TPDMaa[AB][CD] = 0.5 * AAval;
                TPDMab[AB][CD] = 0.5 * ABval;
            }
        }
        // The OVOV elements
        for(int i = 0; i < nOcc; ++i){
            int I = i;
            for(int a = 0; a < nVir; ++a){
                int A = a + nOcc;
                unsigned long int IA = I * dim + A;
                unsigned long int AI = A * dim + I;
                for(int j = 0; j < nOcc; ++j){
                    int J = j;
                    for(int b = 0; b < nVir; ++b){
                        int B = b + nOcc;
                        unsigned long int JB = J * dim + B;
                        unsigned long int BJ = B * dim + J;
                        double AAval = 0.0;
                        double ABval = 0.0;
                        for(int k = 0; k < nOcc; ++k){
                            unsigned long int ik = i * nOcc + k;
                            unsigned long int jk = j * nOcc + k;
                            for(int c = 0; c < nVir; ++c){
                                unsigned long int ac = a * nVir + c;
                                unsigned long int bc = b * nVir + c;
                                AAval += Laa.matrix[h][ik][bc] * Laa.matrix[h][jk][ac];
                                AAval += Lab.matrix[h][ik][bc] * Lab.matrix[h][jk][ac];
                                ABval += Laa.matrix[h][ik][bc] * Laa.matrix[h][jk][ac];
                                ABval += Lab.matrix[h][ik][bc] * Lab.matrix[h][jk][ac];
                            }
                        }
                        TPDMaa[IA][JB] = TPDMaa[AI][BJ] = -AAval;
                        TPDMaa[IA][BJ] = TPDMaa[AI][JB] = AAval;
                        TPDMab[IA][JB] = TPDMab[AI][BJ] = -ABval;
                        TPDMab[IA][BJ] = TPDMab[AI][JB] = ABval;
                    }
                }
            }
        }

        // Contract the integra

        print_mat(TPDMab, dim*dim, dim*dim, outfile);
        free_block(OPDM);
        free_block(TPDMaa);
        free_block(TPDMab);
    }
}

}} // Namespaces
