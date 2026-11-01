// *********************************************************
    // start include guard
    #ifndef PALETTEFILEIO_HPP
    #define PALETTEFILEIO_HPP

    // include standard C/C++ headers
    #include <string>       // [ C++ STL ] Strings
    
    // include project headers
    #include "Colors.hpp"
// *********************************************************


// ========================================================
//      FUNCTIONS TO LOAD PALETTE FILES
// ========================================================


// this is the main load palette function, that autodetects
// the file format (JASC-PAL, RIFF PAL, ACT or HEX) and then
// loads the palette into a vector of RGB colors
void LoadPaletteFile
(
    const std::string& PaletteFilePath,
    RGBColorVector& PaletteColors
);


// *********************************************************
    // end include guard
    #endif
// *********************************************************
