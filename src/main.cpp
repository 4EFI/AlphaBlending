
#include <SFML/Graphics.hpp>
#include <immintrin.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define SSE

//-----------------------------------------------------------------------------

const int Window_Width  = 800;
const int Window_Height = 600;

//-----------------------------------------------------------------------------

inline void AlphaBlend( sf::Image* out_img, 
                        sf::Image* back_img, 
                        sf::Image* front_img, sf::Vector2u front_pos );

inline void AlphaBlendSSE( sf::Image* out_img, 
                           sf::Image* back_img, 
                           sf::Image* front_img, sf::Vector2u front_pos );

//-----------------------------------------------------------------------------

int main()
{
    sf::RenderWindow window( sf::VideoMode( Window_Width, Window_Height ), "C-a-a-a-t" );

    sf::Image cat_img;
    cat_img.loadFromFile( "img/cat.bmp" );

    sf::Image table_img;
    table_img.loadFromFile( "img/table.bmp" );

    sf::Image out_img;
    out_img.create( table_img.getSize().x, table_img.getSize().y, sf::Color::Green ); 

    sf::Texture out_texture;
    out_texture.loadFromImage( out_img );

    sf::Sprite out_sprite;
    out_sprite.setTexture( out_texture );

    sf::Clock clock;

    while( window.isOpen() )
    {
        sf::Time elapsed = clock.restart();

        sf::Event event;
        while( window.pollEvent(event) )
        {
            if( event.type == sf::Event::Closed ) window.close();
        }

        for( int i = 0; i < 100; i++ )
        {
            #ifndef SSE
                AlphaBlend(    &out_img, &table_img, &cat_img, {100, 100} );
            #else
                AlphaBlendSSE( &out_img, &table_img, &cat_img, {100, 100} );
            #endif
            out_texture.update( out_img );
        }
        
        printf ( "FPS %.3f\n", 1 / elapsed.asSeconds() );

        window.clear( sf::Color::Red );
        window.draw ( out_sprite );
        window.display();
    }

    return 0;
}

//-----------------------------------------------------------------------------

inline void AlphaBlend( sf::Image* out_img, 
                        sf::Image* back_img, 
                        sf::Image* front_img, sf::Vector2u front_pos )
{
    assert( out_img   != NULL );
    assert( back_img  != NULL );
    assert( front_img != NULL );

    sf::Vector2u front_img_size = front_img->getSize();

    sf::Uint8*   out_img_pixels_ptr =   out_img->getPixelsPtr();
    sf::Uint8*  back_img_pixels_ptr =  back_img->getPixelsPtr();
    sf::Uint8* front_img_pixels_ptr = front_img->getPixelsPtr();

    memcpy( ( void* )out_img_pixels_ptr, 
            ( void* )back_img->getPixelsPtr(), 
            4 * back_img->getSize().y * back_img->getSize().x );

    for( unsigned int cur_y = 0; cur_y < front_img_size.y; cur_y++ )
    {
        for( unsigned int cur_x = 0; cur_x < front_img_size.x; cur_x++ )
        {
            sf::Color back_clr  = back_img ->getPixel( cur_x + front_pos.x, 
                                                       cur_y + front_pos.y );
            sf::Color front_clr = front_img->getPixel( cur_x,   
                                                       cur_y );   

            sf::Color new_clr( sf::Uint8( ( front_clr.r * front_clr.a + back_clr.r * ( 255 - front_clr.a ) ) >> 8 ),
                               sf::Uint8( ( front_clr.g * front_clr.a + back_clr.g * ( 255 - front_clr.a ) ) >> 8 ),
                               sf::Uint8( ( front_clr.b * front_clr.a + back_clr.b * ( 255 - front_clr.a ) ) >> 8 ),
                               sf::Uint8( ( front_clr.a * 255         + back_clr.a * ( 255 - front_clr.a ) ) >> 8 ) );

            out_img->setPixel( cur_x + front_pos.x, 
                               cur_y + front_pos.y, 
                               new_clr );   

            __m128i front_color = _mm_load_si128( (const __m128i *) front_img->[ cur_x + cur_y * front_img_size.x ] );
            __m128i back_color  = _mm_load_si128( (const __m128i *) 
                                        &Background->pixelsPointer[x + Offset_x + 
                                                                (y + Offset_y) * Background->width]);

            __m128i upperForegroundColor = (__m128i) _mm_movehl_ps((__m128) zeroArray, 
                                                                    (__m128) foregroundColor);
            __m128i upperBackgroundColor = (__m128i) _mm_movehl_ps((__m128) zeroArray, 
                                                                    (__m128) backgroundColor);

            foregroundColor      = _mm_cvtepu8_epi16(foregroundColor);
            upperForegroundColor = _mm_cvtepu8_epi16(upperForegroundColor);

            backgroundColor      = _mm_cvtepu8_epi16(backgroundColor);
            upperBackgroundColor = _mm_cvtepu8_epi16(upperBackgroundColor);

            __m128i lowerForegroundAlpha = _mm_shuffle_epi8(foregroundColor,      
                                                            alphaShuffleMask);                           
            __m128i upperForegroundAlpha = _mm_shuffle_epi8(upperForegroundColor, 
                                                            alphaShuffleMask);

            foregroundColor      = _mm_mullo_epi16(foregroundColor, 
                                                    lowerForegroundAlpha);  
            upperForegroundColor = _mm_mullo_epi16(upperForegroundColor, 
                                                    upperForegroundAlpha);

            backgroundColor      = _mm_mullo_epi16(backgroundColor, 
                                                    _mm_sub_epi16 (ffArray, lowerForegroundAlpha));                     
            upperBackgroundColor = _mm_mullo_epi16(upperBackgroundColor, 
                                                    _mm_sub_epi16 (ffArray, upperForegroundAlpha));

            __m128i lowerSum = _mm_add_epi16(foregroundColor, 
                                                backgroundColor);                                  
            __m128i upperSum = _mm_add_epi16(upperForegroundColor,
                                                upperBackgroundColor);

            lowerSum = _mm_shuffle_epi8(lowerSum, sumShuffleMask);                                
            upperSum = _mm_shuffle_epi8(upperSum, sumShuffleMask);

            __m128i newColor = _mm_set1_epi8((char) 255);
            newColor = (__m128i) _mm_movelh_ps((__m128) lowerSum, 
                                                (__m128) upperSum);

            _mm_store_si128((__m128i *) &result[x + Offset_x + (y + Offset_y) * Background->width], newColor);
        }
    }
}

//-----------------------------------------------------------------------------

inline void AlphaBlendSSE( sf::Image* out_img, 
                           sf::Image* back_img, 
                           sf::Image* front_img, sf::Vector2u front_pos )
{
    assert( out_img   != NULL );
    assert( back_img  != NULL );
    assert( front_img != NULL );

    sf::Vector2u front_img_size = front_img->getSize();

    memcpy( ( void* )out_img ->getPixelsPtr(), 
            ( void* )back_img->getPixelsPtr(), 
            4 * back_img->getSize().y * back_img->getSize().x );

    for( unsigned int cur_y = 0; cur_y < front_img_size.y; cur_y++ )
    {
        for( unsigned int cur_x = 0; cur_x < front_img_size.x; cur_x++ )
        {
            sf::Color back_clr  = back_img ->getPixel( cur_x + front_pos.x, 
                                                       cur_y + front_pos.y );
            sf::Color front_clr = front_img->getPixel( cur_x,   
                                                       cur_y );   

            sf::Color new_clr( sf::Uint8( ( front_clr.r * front_clr.a + back_clr.r * ( 255 - front_clr.a ) ) >> 8 ),
                               sf::Uint8( ( front_clr.g * front_clr.a + back_clr.g * ( 255 - front_clr.a ) ) >> 8 ),
                               sf::Uint8( ( front_clr.b * front_clr.a + back_clr.b * ( 255 - front_clr.a ) ) >> 8 ),
                               sf::Uint8( ( front_clr.a * 255         + back_clr.a * ( 255 - front_clr.a ) ) >> 8 ) );

            out_img->setPixel( cur_x + front_pos.x, 
                               cur_y + front_pos.y, 
                               new_clr );   
        }
    }
}

//-----------------------------------------------------------------------------