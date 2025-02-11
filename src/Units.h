//
//  Units.h
//  tsflib
//
//  Created by the EPANET-RTX Development Team
//  See README.md and license.txt for more information
//  

#ifndef tsflib_units_h
#define tsflib_units_h

#include "tsfMacros.h"
#include <string>
#include <map>

// convenience defines ------------ unit= conversion,   dimension (m,l,t,current,temp,amount,intensity)
#define TSF_NO_UNITS                TSF::Units(0)
#define TSF_DIMENSIONLESS           TSF::Units(1)
#define TSF_HERTZ                   TSF::Units(1,            0,0,-1)
#define TSF_RPM                     TSF::Units(0.016666666666667,0,0,-1)
#define TSF_PERCENT                 TSF::Units(0.01)
// Pressure
#define TSF_PSI                     TSF::Units(6894.75728,   1,-1,-2)
#define TSF_PASCAL                  TSF::Units(1,            1,-1,-2)
#define TSF_KILOPASCAL              TSF::Units(1000,         1,-1,-2)
#define TSF_BAR                     TSF::Units(100000,       1,-1,-2)
// distance
#define TSF_FOOT                    TSF::Units(.3048,        0,1,0)
#define TSF_INCH                    TSF::Units(.0254,        0,1,0)
#define TSF_METER                   TSF::Units(1,            0,1,0)
#define TSF_CENTIMETER              TSF::Units(.01,          0,1,0)
// volume
#define TSF_CUBIC_METER             TSF::Units(1,            0,3,0)
#define TSF_GALLON                  TSF::Units(.00378541,    0,3,0)
#define TSF_MILLION_GALLON          TSF::Units(3785.41178,   0,3,0)
#define TSF_LITER                   TSF::Units(.001,         0,3,0)
#define TSF_CUBIC_FOOT              TSF::Units(.0283168466,  0,3,0)
//flow
#define TSF_CUBIC_METER_PER_SECOND  TSF::Units(1,            0,3,-1)
#define TSF_THOUSAND_CUBIC_METER_PER_DAY TSF::Units(.0115740741,0,3,-1)
#define TSF_CUBIC_FOOT_PER_SECOND   TSF::Units(.0283168466,  0,3,-1)
#define TSF_GALLON_PER_SECOND       TSF::Units(.00378541178, 0,3,-1)
#define TSF_GALLON_PER_MINUTE       TSF::Units(.00006309020, 0,3,-1)
#define TSF_GALLON_PER_DAY          TSF::Units(43.812638888E-9, 0,3,-1)
#define TSF_MILLION_GALLON_PER_DAY  TSF::Units(.0438126364,  0,3,-1)
#define TSF_LITER_PER_SECOND        TSF::Units(.001,         0,3,-1)
#define TSF_LITER_PER_MINUTE        TSF::Units(.00001666667, 0,3,-1)
#define TSF_MILLION_LITER_PER_DAY   TSF::Units(.0115740741,  0,3,-1)
#define TSF_CUBIC_METER_PER_HOUR    TSF::Units(.000277777778,0,3,-1)
#define TSF_CUBIC_METER_PER_DAY     TSF::Units(.000011574074,0,3,-1)
#define TSF_ACRE_FOOT_PER_DAY       TSF::Units(.0142764102,  0,3,-1)
#define TSF_IMPERIAL_MILLION_GALLON_PER_DAY TSF::Units(.0526168042,0,3,-1)
// time
#define TSF_SECOND                  TSF::Units(1,            0,0,1)
#define TSF_MINUTE                  TSF::Units(60,           0,0,1)
#define TSF_HOUR                    TSF::Units(3600,         0,0,1)
#define TSF_DAY                     TSF::Units(86400,        0,0,1)
// mass
#define TSF_MICROGRAM               TSF::Units(.000000001,   1,0,0)
#define TSF_MILLIGRAM               TSF::Units(.000001,      1,0,0)
#define TSF_GRAM                    TSF::Units(.001,         1,0,0)
#define TSF_KILOGRAM                TSF::Units(1,            1,0,0)
// concentration
#define TSF_MICROGRAMS_PER_LITER    TSF::Units(.000001,      1,-3,0)
#define TSF_MILLIGRAMS_PER_LITER    TSF::Units(.001,         1,-3,0)
// conductance
#define TSF_MICROSIEMENS_PER_CM     TSF::Units(.0001,        -1,-3,3,2)
// velocity
#define TSF_METER_PER_SECOND        TSF::Units(1,            0,1,-1)
#define TSF_FOOT_PER_SECOND         TSF::Units(.3048,        0,1,-1)
#define TSF_FOOT_PER_HOUR           TSF::Units(84.6666667E-6,0,1,-1)
// acceleration
#define TSF_METER_PER_SECOND_SECOND TSF::Units(1,            0,1,-2)
#define TSF_FOOT_PER_SECOND_SECOND  TSF::Units(.3048,        0,1,-2)
#define TSF_FOOT_PER_HOUR_HOUR      TSF::Units(25.5185185E-9,0,1,-2)

// temperature
#define TSF_DEGREE_KELVIN           TSF::Units(1,            0,0,0,0,1,0,0)
#define TSF_DEGREE_RANKINE          TSF::Units(5./9.,        0,0,0,0,1,0,0) //  K = R * 5/9
#define TSF_DEGREE_CELSIUS          TSF::Units(1,            0,0,0,0,1,0,0,273.15)
#define TSF_DEGREE_FAHRENHEIT       TSF::Units(5./9.,        0,0,0,0,1,0,0,459.67)

// power and energy
#define TSF_KILOWATT_HOUR          TSF::Units(3600000,       1,2,-2)
#define TSF_JOULE                  TSF::Units(1,             1,2,-2)
#define TSF_MEGAJOULE              TSF::Units(1000000,       1,2,-2)
#define TSF_WATT                   TSF::Units(1,             1,2,-3)
#define TSF_KILOWATT               TSF::Units(1000,          1,2,-3)

#define TSF_VOLT                   TSF::Units(1,             1,2,-3,-1) // (kg * m2) / ( A * s3)
#define TSF_AMP                    TSF::Units(1,             0,0,0,1)

// energy density
#define TSF_ENERGY_DENSITY        TSF::Units(3600000,        1,-1,-2)
#define TSF_ENERGY_DENSITY_MG     TSF::Units(951.01983668876,1,-1,-2)

// area
#define TSF_SQ_FOOT               TSF::Units(0.092903,       0,2)
#define TSF_SQ_METER              TSF::Units(1,              0,2)
#define TSF_SQ_INCH               TSF::Units(0.00064516,     0,2)
#define TSF_SQ_CENTIMETER         TSF::Units(0.0001,         0,2)

namespace TSF {
  /*!
   \class Units
   \brief Keep track of dimensions and units of measure.
   
   In general, any unit can be expressed as a set of mutually independent exponent factors of the
   quantities listed below. We shall support all of the seven (7) exponents, even though we expect to
   never use the candela, but why limit the future, right?
   
   - length = meter (m)
   - mass = kilogram (kg)
   - time = second (s)
   - electric current = ampere (A)
   - thermodynamic temperature = kelvin (K)
   - amount of substance = mole (mol)
   - luminous intensity = candela (cd)
   
  */
  /*!
   \fn Units::Units(double conversion, int mass, int length, int time, int current, int temperature, int amount, int intensity)
   \brief Create a new Units object with the specified dimensions and conversion factor.
   
   The default dimensional exponents are all zero (0), which means dimensionless. See Units.h for some predefined units of measure.
   
   */
  
  class Units : public TSF_object {
  public:
    TSF_BASE_PROPS(Units);
    const static std::map<std::string, Units> unitStrings;
    
    Units(double conversion = 1., int kilogram = 0, int meter = 0, int second = 0, int ampere = 0, int kelvin = 0, int mole = 0, int candela = 0, double offset = 0);
    Units(const std::string& type);
    Units operator*(const Units& unit) const;
    Units operator*(const double factor) const;
    Units operator/(const Units& unit) const;
    Units operator^(const double power) const;
    bool operator==(const Units& unit) const;
    bool operator!=(const Units& unit) const;
    
    const bool isInvalid() const;
    const bool isSameDimensionAs(const Units& unit) const;
    const bool isDimensionless() const;
    const double conversion() const;
    const double offset() const;
    static double convertValue(double value, const Units& fromUnits, const Units& toUnits);
    static Units unitOfType(const std::string& unitString);
    const std::string to_string() const;
    const std::string rawUnitString(bool ignoreZeroDimensions = true) const;
    
    virtual std::ostream& toStream(std::ostream &stream) const;
    
  private:
    int _m;
    int _kg;
    int _s;
    int _A;
    int _K;
    int _mol;
    int _cd;
    double _conversion;
    double _offset;
  };
  
  std::ostream& operator<< (std::ostream &out, Units &unit);

} // namespace



#endif
