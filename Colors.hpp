// *********************************************************
    // start include guard
    #ifndef COLORS_HPP
    #define COLORS_HPP
    
    // include standard C/C++ headers
    #include <vector>       // [ C++ STL ] Vectors
    #include <cstdint>      // [ ANSI C ] Standard integers
// *********************************************************


// ========================================================
//      COMMON TYPES USED WITH COLORS AND PALETTES
// ========================================================


// type for a RGB color (no alpha)
struct RGBColor
{
    uint8_t red;
    uint8_t green;
    uint8_t blue;
};

// types for 1D and 2D RGBColor vectors
typedef std::vector< RGBColor > RGBColorVector;
typedef std::vector< RGBColorVector > RGBColorMatrix;

// the same but expressed as indices from a palette
typedef std::vector< uint8_t > PaletteIndexVector;
typedef std::vector< PaletteIndexVector > PaletteIndexMatrix;


// ========================================================
//      FUNCTIONS TO HANDLE COLORS
// ========================================================


// distance between 2 RGB colors, calculated in perceptual space
double PerceptualColorDistance( RGBColor& c1, RGBColor& c2 );


// *********************************************************
    // end include guard
    #endif
// *********************************************************
