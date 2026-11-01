// *********************************************************
    // include standard C/C++ headers
    #include <cmath>        // [ ANSI C ] Mathematics

    // include project headers
    #include "Colors.hpp"
    
    // declare used namespaces
    using namespace std;
// *********************************************************


// =========================================================
//      HANDLING COLORS IN CIELAB SPACE
// =========================================================


// type for a CIELAB color
struct CielabColor
{
    double L;
    double a;
    double b;
};

// -----------------------------------------------------------------------------

// converting an sRGB color component to linear RGB
double SRGBtoLinearRGB( double c )
{
    c /= 255.0;
    
    if( c <= 0.04045 )
      return c / 12.92;
    
    return pow( (c + 0.055) / 1.055, 2.4 );
}

// -----------------------------------------------------------------------------

// converting sRGB to CIELAB
CielabColor RgbToCielab( const RGBColor& c )
{
    // sRGB -> linear RGB
    double r = SRGBtoLinearRGB( c.red );
    double g = SRGBtoLinearRGB( c.green );
    double b = SRGBtoLinearRGB( c.blue );
    
    // linear RGB -> XYZ (D65 illuminant model)
    double X = r * 0.4124 + g * 0.3576 + b * 0.1805;
    double Y = r * 0.2126 + g * 0.7152 + b * 0.0722;
    double Z = r * 0.0193 + g * 0.1192 + b * 0.9505;
    
    // D65 reference white
    constexpr double Xn = 0.95047;
    constexpr double Yn = 1.00000;
    constexpr double Zn = 1.08883;
    
    auto f = []( double t )
    {
        constexpr double Epsilon = 216.0 / 24389.0;
        constexpr double Kappa   = 24389.0 / 27.0;
        
        if( t > Epsilon )
          return cbrt( t );
        
        return (Kappa * t + 16.0) / 116.0;
    };
    
    double fx = f( X / Xn );
    double fy = f( Y / Yn );
    double fz = f( Z / Zn );
    
    // convert to CIELAB components
    CielabColor Cielab;
    
    Cielab.L = 116.0 * fy - 16.0;
    Cielab.a = 500.0 * (fx - fy);
    Cielab.b = 200.0 * (fy - fz);
    
    return Cielab;
}


// =========================================================
//      PERCEPTUAL COLOR DISTANCE BETWEEN RGB COLORS
// =========================================================


double PerceptualColorDistance( RGBColor& c1, RGBColor& c2 )
{
    CielabColor Cielab1 = RgbToCielab( c1 );
    CielabColor Cielab2 = RgbToCielab( c2 );

    double dL = Cielab1.L - Cielab2.L;
    double da = Cielab1.a - Cielab2.a;
    double db = Cielab1.b - Cielab2.b;

    // distance between CIELAB colors (Delta E 1976 formula)
    return sqrt( dL * dL + da * da + db * db );
}
