// *********************************************************
    // start include guard
    #ifndef FILEFUNCTIONS_HPP
    #define FILEFUNCTIONS_HPP

    // include standard C/C++ headers
    #include <vector>       // [ C++ STL ] Vectors
    #include <string>       // [ C++ STL ] Strings
    
    // include project headers
    #include "Colors.hpp"
// *********************************************************


// ========================================================
//      COMMON TYPES USED WITH COLORS AND PALETTES
// ========================================================


// types for 1D and 2D RGBColor vectors
typedef std::vector< RGBColor > RGBColorVector;
typedef std::vector< RGBColorVector > RGBColorMatrix;

// the same but expressed as indices from a palette
typedef std::vector< uint8_t > PaletteIndexVector;
typedef std::vector< PaletteIndexVector > PaletteIndexMatrix;


// ========================================================
//      FUNCTIONS TO LOAD AND SAVE FROM FILE
// ========================================================


// loads a PNG file, converting it to 24-bit RGB if needed;
// then it stores its pixels into a 2D matrix of RGB colors
void LoadPNGFile
(
    const std::string& PNGFilePath,
    RGBColorMatrix& ImageColors
);

// takes a color palette and a 2D matrix of color indices from
// that palette, and saves that into a palettized PNG image file
void SaveIndexedPNGFile
(
    const std::string& PNGFilePath,
    const RGBColorVector& Palette,
    const PaletteIndexMatrix& ImageColorIndices
);

// loads a palette file in PAL format and
// stores it into a vector of RGB colors
void LoadPaletteFile
(
    const std::string& PaletteFilePath,
    RGBColorVector& PaletteColors
);


// *********************************************************
    // end include guard
    #endif
// *********************************************************
