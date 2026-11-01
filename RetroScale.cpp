// *********************************************************
//  RetroScale - Copyright (C) 2026 Carra
//  --------------------------------------------------------
//  This program will take a larger input PNG image and
//  produce a "pixel art" version of it. For that it will
//  apply 2 transformations: 1) reduce it by the specified
//  integer factor (e.g: 3x) and 2) apply a color palette
//  to it. These 2 are applied at the same time, so that
//  each pixel in the output best approximates the source
//  pixel block with the given color palette.
// *********************************************************


// *********************************************************
    // include standard C/C++ headers
    #include <iostream>     // [ C++ STL ] I/O streams
    #include <limits>       // [ C++ STL ] Numeric limits
    #include <cstddef>      // [ ANSI C ] Standard definitions
    #include <cstdlib>      // [ ANSI C ] Standard library
    
    // include project headers
    #include "Colors.hpp"
    #include "FileFunctions.hpp"
    
    // define used namespaces
    using namespace std;
// *********************************************************


// ========================================================
//      MAIN FUNCTION
// ========================================================


int main( int NumberOfArguments, char* Arguments[] )
{
    // print usage if needed
    if( NumberOfArguments != 5 )
    {
        cerr << "Usage: retroscale <input.png> <output.png> <palette> <reduction>" << endl;
        cerr << "- Image files are read and saved in PNG format" << endl;
        cerr << "- Palette files are detected as JASC-PAL, RIFF PAL, ACT or HEX" << endl;
        cerr << "- Reduction factor is an integer (e.g: 3 will reduce the image by 3x)" << endl;
        cerr << "- Transparency or alpha channel are not supported" << endl;
        
        return 2;
    }
    
    try
    {
        // read program arguments
        const string InputPNGPath = Arguments[ 1 ];
        const string OututPNGPath = Arguments[ 2 ];
        const string PalettePath  = Arguments[ 3 ];
        
        // careful, reduction factor may be invalid
        const int ReductionFactor = atoi( Arguments[ 4 ] );
        
        if( ReductionFactor <= 0 )
          throw runtime_error( "Reduction factor must be a positive integer" );
        
        // load input image
        vector< vector<RGBColor> > OriginalImage;
        LoadPNGFile( InputPNGPath, OriginalImage );
        
        if( OriginalImage.empty() || OriginalImage[ 0 ].empty() )
          throw runtime_error( "Input image is empty or was not loaded correctly" );
        
        // load color palette file
        vector< RGBColor > PaletteColors;
        LoadPaletteFile( PalettePath, PaletteColors );
        
        if( PaletteColors.empty() )
          throw runtime_error( "Color palette is empty or was not loaded correctly" );
        
        if( PaletteColors.size() > 256 )
          throw runtime_error( "Only palettes up to 256 colors are supported" );
        
        // determine usable sized given the reduction
        const size_t OriginalHeight = OriginalImage.size();
        const size_t OriginalWidth  = OriginalImage[0].size();
        
        const size_t UsableWidth  = ( OriginalWidth  / ReductionFactor ) * ReductionFactor;
        const size_t UsableHeight = ( OriginalHeight / ReductionFactor ) * ReductionFactor;
        
        if( UsableWidth == 0 || UsableHeight == 0 )
          throw runtime_error( "Reduction factor is too large for this image" );
        
        // determine final size of the reduced image
        const size_t ReducedWidth  = UsableWidth  / ReductionFactor;
        const size_t ReducedHeight = UsableHeight / ReductionFactor;
        
        // the final image will be saved as indexed, so
        // store only the color indices in the palette
        vector< vector<uint8_t> > OutputImageIndices
        (
            ReducedHeight,
            vector< uint8_t >( ReducedWidth )
        );
        
        // process every reduced pixel in the output image
        for( size_t ReducedY = 0; ReducedY < ReducedHeight; ++ReducedY )
        {
            for( size_t ReducedX = 0; ReducedX < ReducedWidth; ++ReducedX )
            {
                double BestTotalDistance = numeric_limits< double >::max();
                size_t BestPaletteIndex = 0;
                
                // test every color in the palette
                for( size_t PaletteIndex = 0; PaletteIndex < PaletteColors.size(); ++PaletteIndex )
                {
                    double TotalDistance = 0.0;
                    const RGBColor& PaletteColor = PaletteColors[ PaletteIndex ];
                    
                    // iterate over the corresponding block of original pixels
                    for( int BlockY = 0; BlockY < ReductionFactor; ++BlockY )
                    {
                        for( int BlockX = 0; BlockX < ReductionFactor; ++BlockX )
                        {
                            const size_t OriginalX = ReducedX * ReductionFactor + BlockX;
                            const size_t OriginalY = ReducedY * ReductionFactor + BlockY;
                            const RGBColor& OriginalPixel = OriginalImage[ OriginalY ][ OriginalX ];
                            
                            const double PixelDistance = PerceptualColorDistance
                            (
                                const_cast< RGBColor& >( OriginalPixel ),
                                const_cast< RGBColor& >( PaletteColor )
                            );
                            
                            TotalDistance += PixelDistance * PixelDistance;
                        }
                    }
                    
                    // keep track of the best approximation
                    if( TotalDistance < BestTotalDistance )
                    {
                        BestTotalDistance = TotalDistance;
                        BestPaletteIndex = PaletteIndex;
                    }
                }
                
                // save index for the final color of this output pixel
                OutputImageIndices[ ReducedY ][ ReducedX ] = static_cast< uint8_t >( BestPaletteIndex );
            }
        }
        
        // save pixelated and palettized PNG
        SaveIndexedPNGFile( OututPNGPath, PaletteColors, OutputImageIndices );
        return 0;
    }
    catch( const exception& e )
    {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }
}
