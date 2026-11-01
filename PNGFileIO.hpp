// *********************************************************
    // start include guard
    #ifndef PNGFILEIO_HPP
    #define PNGFILEIO_HPP

    // include standard C/C++ headers
    #include <string>       // [ C++ STL ] Strings
    
    // include project headers
    #include "Colors.hpp"
// *********************************************************


// ========================================================
//      FUNCTIONS TO LOAD AND SAVE FROM PNG FILES
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


// *********************************************************
    // end include guard
    #endif
// *********************************************************
