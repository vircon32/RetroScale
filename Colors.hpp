// *********************************************************
    // start include guard
    #ifndef COLORS_HPP
    #define COLORS_HPP
    
    // include standard C/C++ headers
    #include <cstdint>      // [ ANSI C ] Standard integers
// *********************************************************


// ========================================================
//      FUNCTIONS TO LOAD AND SAVE FROM FILE
// ========================================================


// type for a RGB color (no alpha)
struct RGBColor
{
    uint8_t red;
    uint8_t green;
    uint8_t blue;
};

// ---------------------------------------------------------

// distance between 2 RGB colors, calculated in perceptual space
double PerceptualColorDistance( RGBColor& c1, RGBColor& c2 );


// *********************************************************
    // end include guard
    #endif
// *********************************************************
