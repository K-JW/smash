/*
 *
 *    Copyright (c) 2016
 *      SMASH Team
 *
 *    GNU General Public License (GPLv3 or later)
 *
 */

#include <cmath>
#include "pdgcode.h"
#include "particletype.h"
#include "kinematics.h"


// calculation method for the cross sections
enum class ComputationMethod {Analytic, Lookup, Parametrized};


// usage would be 
// PhotonCrossSection xs<Analytic>; xs.xs_pi_pi_rho(var1, var2...)
template <ComputationMethod method>
class PhotonCrossSection; 

template<>
class PhotonCrossSection <ComputationMethod::Analytic>
{
    
public: 

    // TODO: Clean up parameters. Some are unused. 
    

    static double xs_pi_pi_rho0(const double s);
        
    static double xs_pi0_pi_rho(const double s);
    static double xs_pi0_rho0_pi0(const double s);

    static double xs_diff_pi0_rho0_pi0(const double s, const double t);
    
    double xs_pi_rho0_pi(const double m1, const double m2, 
        const double m3, const double t1, const double t2, const double s,
        const double mpion, const double mrho);
    double xs_pi_rho_pi0(const double m1, const double m2, 
        const double m3, const double t1, const double t2, const double s,
        const double mpion, const double mrho);
    double xs_pi0_rho_pi(const double m1, const double m2, 
        const double m3, const double t1, const double t2, const double s,
        const double mpion, const double mrho);
    
    
        // and so on
    
private:

    constexpr static double to_mb = 0.3894;
    constexpr static double Const = 0.059;
    constexpr static double g_POR = 11.93;
    constexpr static double ma1 = 1.26;
    constexpr static double ghat = 6.4483;
    constexpr static double eta1 = 2.3920;
    constexpr static double eta2 = 1.9430;
    constexpr static double delta = -0.6426;
    constexpr static double C4 = -0.14095;
    constexpr static double Gammaa1 = 0.4;
    constexpr static double Pi = M_PI;
    constexpr static double m_omega = 0.783;

    constexpr static double m_pion_ = 0.139;
    constexpr static double m_rho_ = 0.775;
    

};

template<>
class PhotonCrossSection <ComputationMethod::Lookup>
{};

template<>
class PhotonCrossSection <ComputationMethod::Parametrized>
{};