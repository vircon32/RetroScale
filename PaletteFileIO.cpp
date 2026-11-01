// *********************************************************
    // include standard C/C++ headers
    #include <fstream>      // [ C++ STL ] File streams
    #include <sstream>      // [ C++ STL ] String streams
    #include <iostream>     // [ C++ STL ] I/O streams
    #include <stdexcept>    // [ C++ STL ] Standard exceptions
    #include <cstdio>       // [ ANSI C ] Standard I/O
    #include <cmath>        // [ ANSI C ] Mathematics
    
    // include project headers
    #include "PaletteFileIO.hpp"
    
    // declare used namespaces
    using namespace std;
// *********************************************************


// =========================================================
//      AUXILIARY FUNCTIONS TO LOAD FROM PALETTE FILES
// =========================================================


// this function reads the next line of text from a palette file,
// and returns true if a line was read or false if file ended
static bool ReadPaletteTextLine( istream& InputFile, string& TextLine )
{
    if( !getline( InputFile, TextLine ) )
      return false;
    
    // also accept Windows line endings when running on other platforms
    if( !TextLine.empty() && TextLine.back() == '\r' )
      TextLine.pop_back();
    
    return true;
}

// ---------------------------------------------------------

// detects whether a palette text line is a valid HEX color line
static bool IsHEXColorLine( const string& TextLine )
{
    if( TextLine.size() != 6 )
      return false;
    
    for( char Character : TextLine )
    {
        if( !(Character >= '0' && Character <= '9') &&
            !(Character >= 'a' && Character <= 'f') &&
            !(Character >= 'A' && Character <= 'F') )
          return false;
    }
    
    return true;
}

// ---------------------------------------------------------

// function to read a 32-bit value at a certain position from
// RIFF palette files, taking into account little endian format
static uint32_t ReadPaletteLittleEndian( const string& FileData, size_t Offset, size_t NumberOfBytes )
{
    uint32_t Value = 0;
    
    // callers validate the field's bounds before reading it
    for( size_t i = 0; i < NumberOfBytes; i++ )
      Value |= static_cast< uint32_t >( static_cast< uint8_t >( FileData[ Offset + i ] ) ) << ( i * 8 );
    
    return Value;
}


// =========================================================
//      LOAD FUNCTIONS FOR SPECIFIC PALETTE FILE FORMATS
// =========================================================


static void LoadJASCPaletteFile( const string& PaletteFilePath, RGBColorVector& PaletteColors )
{
    // open input palette file in text mode
    ifstream InputFile( PaletteFilePath );
    
    if( !InputFile.is_open() )
      throw runtime_error( "Could not open palette file: " + PaletteFilePath );
    
    string TextLine;
    
    // validate format signature in the first line
    if( !ReadPaletteTextLine( InputFile, TextLine ) || TextLine != "JASC-PAL" )
      throw runtime_error( "The palette file does not have a valid JASC-PAL format" );
    
    // read version number in second line, typically "0100"
    if( !ReadPaletteTextLine( InputFile, TextLine ) )
      throw runtime_error( "Unexpected end of file reading palette file version" );
    
    // read number of colors in the third line
    if( !ReadPaletteTextLine( InputFile, TextLine ) )
      throw runtime_error( "Unexpected end of file reading palette file color number" );
    
    stringstream CountStream( TextLine );
    int NumberOfColors = 0;
    
    if( !(CountStream >> NumberOfColors) || NumberOfColors < 1 || NumberOfColors > 256 )
      throw runtime_error( "Stated number of colors in palette file is not valid: " + TextLine );
    
    // prepare the output vector to store all colors in order
    PaletteColors.clear();
    PaletteColors.reserve( NumberOfColors );
    
    // read every color in order (an RGB triple per line)
    for( int i = 0; i < NumberOfColors; i++ )
    {
        if( !ReadPaletteTextLine( InputFile, TextLine ) )
          throw runtime_error( "Palette file contains less colors than it states. Contains: " + to_string( i ) + ", expected: " + to_string( NumberOfColors ) );
        
        // ignore empty lines within the data block
        if( TextLine.empty() )
        {
            --i;
            continue;
        }
        
        // read the 3 color components as integers
        stringstream ColorStream( TextLine );
        int r, g, b;
        
        if( !(ColorStream >> r >> g >> b) )
          throw runtime_error( "RGB format error in palette file, in line number " + to_string( i + 4 ) + ": " + TextLine );
        
        // check range for each of the colors
        if( r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255 )
          throw runtime_error( "Color components out of range (0-255) in palette file, in line number " + to_string( i + 4 ) );
        
        // build the output RGB color for this index
        RGBColor Color;
        Color.red   = static_cast< uint8_t >( r );
        Color.green = static_cast< uint8_t >( g );
        Color.blue  = static_cast< uint8_t >( b );
        
        PaletteColors.push_back( Color );
    }
}

// ---------------------------------------------------------

static void LoadHEXPaletteFile( const string& FileData, RGBColorVector& PaletteColors )
{
    istringstream InputFile( FileData );
    string TextLine;
    int LineNumber = 0;
    
    // each nonempty line contains one color as six hexadecimal digits
    while( ReadPaletteTextLine( InputFile, TextLine ) )
    {
        LineNumber++;
        
        if( TextLine.empty() )
          continue;
        
        if( !IsHEXColorLine( TextLine ) )
          throw runtime_error( "HEX format error in palette file, in line number " + to_string( LineNumber ) );
        
        if( PaletteColors.size() == 256 )
          throw runtime_error( "Only palettes up to 256 colors are supported" );
        
        unsigned int ColorValue = 0;
        stringstream ColorStream( TextLine );
        ColorStream >> hex >> ColorValue;
        
        RGBColor Color;
        Color.red   = static_cast< uint8_t >( ColorValue >> 16 );
        Color.green = static_cast< uint8_t >( ColorValue >> 8 );
        Color.blue  = static_cast< uint8_t >( ColorValue );
        PaletteColors.push_back( Color );
    }
}

// ---------------------------------------------------------

static void LoadACTPaletteFile( const string& FileData, RGBColorVector& PaletteColors )
{
    // ACT format has 256 RGB triples, optionally followed by two big-endian words
    if( FileData.size() != 768 && FileData.size() != 772 )
      throw runtime_error( "The ACT palette file must contain 768 or 772 bytes" );
    
    size_t NumberOfColors = 256;
    
    if( FileData.size() == 772 )
    {
        NumberOfColors = static_cast< uint8_t >( FileData[ 768 ] ) * 256u
                       + static_cast< uint8_t >( FileData[ 769 ] );
        
        // a zero count denotes the full 256-color table
        if( NumberOfColors == 0 )
          NumberOfColors = 256;
        
        else if( NumberOfColors > 256 )
          throw runtime_error( "Stated number of colors in ACT palette file is not valid" );
        
        // the last word is a transparency index;
        // we ignore it since this program only uses RGB
    }
    
    PaletteColors.reserve( NumberOfColors );
    
    for( size_t i = 0; i < NumberOfColors; i++ )
    {
        RGBColor Color;
        Color.red   = static_cast< uint8_t >( FileData[ i * 3 + 0 ] );
        Color.green = static_cast< uint8_t >( FileData[ i * 3 + 1 ] );
        Color.blue  = static_cast< uint8_t >( FileData[ i * 3 + 2 ] );
        PaletteColors.push_back( Color );
    }
}

// ---------------------------------------------------------

static void LoadRIFFPaletteFile( const string& FileData, RGBColorVector& PaletteColors )
{
    // RIFF PAL contains a little-endian LOGPALETTE structure in its data chunk
    if( FileData.size() < 12 || FileData.compare( 0, 4, "RIFF" ) != 0 ||
        FileData.compare( 8, 4, "PAL " ) != 0 )
      throw runtime_error( "The palette file does not have a valid RIFF PAL format" );
    
    if( ReadPaletteLittleEndian( FileData, 4, 4 ) != FileData.size() - 8 )
      throw runtime_error( "The RIFF palette file size does not match its header" );
    
    size_t Offset = 12;
    bool FoundColors = false;
    
    while( Offset < FileData.size() )
    {
        if( FileData.size() - Offset < 8 )
          throw runtime_error( "Incomplete chunk header in RIFF palette file" );
        
        bool IsDataChunk = FileData.compare( Offset, 4, "data" ) == 0;
        size_t ChunkSize = ReadPaletteLittleEndian( FileData, Offset + 4, 4 );
        Offset += 8;
        
        if( ChunkSize > FileData.size() - Offset )
          throw runtime_error( "Incomplete chunk data in RIFF palette file" );
        
        if( IsDataChunk )
        {
            if( FoundColors || ChunkSize < 4 )
              throw runtime_error( "Invalid data chunk in RIFF palette file" );
            
            if( ReadPaletteLittleEndian( FileData, Offset, 2 ) != 0x0300 )
              throw runtime_error( "Unsupported version in RIFF palette file" );
            
            size_t NumberOfColors = ReadPaletteLittleEndian( FileData, Offset + 2, 2 );
            
            if( NumberOfColors < 1 || NumberOfColors > 256 || ChunkSize != 4 + NumberOfColors * 4 )
              throw runtime_error( "Invalid color count or data size in RIFF palette file" );
            
            PaletteColors.reserve( NumberOfColors );
            
            for( size_t i = 0; i < NumberOfColors; i++ )
            {
                // careful, the 4th byte of each entry contains flags, not alpha
                RGBColor Color;
                Color.red   = static_cast< uint8_t >( FileData[ Offset + 4 + i * 4 + 0 ] );
                Color.green = static_cast< uint8_t >( FileData[ Offset + 4 + i * 4 + 1 ] );
                Color.blue  = static_cast< uint8_t >( FileData[ Offset + 4 + i * 4 + 2 ] );
                PaletteColors.push_back( Color );
            }
            
            FoundColors = true;
        }
        
        // skip unknown chunks and the padding byte following odd-sized chunks
        Offset += ChunkSize;
        
        if( ChunkSize % 2 != 0 )
        {
            if( Offset == FileData.size() )
              throw runtime_error( "Missing chunk padding in RIFF palette file" );
            
            Offset++;
        }
    }
    
    if( !FoundColors )
      throw runtime_error( "The RIFF palette file does not contain a data chunk" );
}


// =========================================================
//      GENERAL PALETTE FILE LOADING FUNCTION
// =========================================================


void LoadPaletteFile( const string& PaletteFilePath, RGBColorVector& PaletteColors )
{
    // open file in binary format
    ifstream InputFile( PaletteFilePath, ios::binary | ios::ate );
    
    if( !InputFile.is_open() )
      throw runtime_error( "Could not open palette file: " + PaletteFilePath );
    
    streamsize FileSize = InputFile.tellg();
    
    if( FileSize < 0 )
      throw runtime_error( "Could not determine palette file size: " + PaletteFilePath );
    
    string FileData( static_cast< size_t >( FileSize ), '\0' );
    InputFile.seekg( 0 );
    
    if( FileSize > 0 && !InputFile.read( &FileData[ 0 ], FileSize ) )
      throw runtime_error( "Could not read palette file: " + PaletteFilePath );
    
    istringstream TextStream( FileData );
    string FirstLine;
    
    // HEX files may contain blank lines before the first color
    while( ReadPaletteTextLine( TextStream, FirstLine ) && FirstLine.empty() )
    {
        // (do nothing)
    }
    
    // load into a temporary vector so failed loads leave the output unchanged
    RGBColorVector LoadedColors;
    
    // ---------------------------------------------------------
    // auto-detect file format from the palette file
    // ---------------------------------------------------------
    
    // case 1: PAL format (usually *.pal extension)
    if( FileData.compare( 0, 8, "JASC-PAL" ) == 0 )
      LoadJASCPaletteFile( PaletteFilePath, LoadedColors );
    
    // case 2: RIFF format (usually *.pal extension)
    else if( FileData.compare( 0, 4, "RIFF" ) == 0 )
      LoadRIFFPaletteFile( FileData, LoadedColors );
    
    // case 3: HEX format (usually *.hex extension)
    else if( IsHEXColorLine( FirstLine ) )
      LoadHEXPaletteFile( FileData, LoadedColors );
    
    // case 4: ACT format (usually *.act extension)
    // since it has no signature we can only detect
    // this format from its standard file sizes
    else if( FileSize == 768 || FileSize == 772 )
      LoadACTPaletteFile( FileData, LoadedColors );
    
    // raise an error if we can't detect a known format
    else throw runtime_error( "Unrecognized palette file format: " + PaletteFilePath );
    
    PaletteColors.swap( LoadedColors );
}
