
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

const char Zero_Val = ( char )128;

const __m128i FF_Arr = _mm_set_epi8( 0, ( char )255, 0, ( char )255,
                                     0, ( char )255, 0, ( char )255, 
                                     0, ( char )255, 0, ( char )255,
                                     0, ( char )255, 0, ( char )255 );

const __m128i Alpha_Shuffle_Mask = _mm_set_epi8( Zero_Val, 8, Zero_Val, 8, 
                                                 Zero_Val, 8, Zero_Val, 8,
                                                 Zero_Val, 0, Zero_Val, 0,
                                                 Zero_Val, 0, Zero_Val, 0 );

const __m128i Sum_Shuffle_Mask   = _mm_set_epi8( Zero_Val, Zero_Val, Zero_Val, Zero_Val, 
                                                 Zero_Val, Zero_Val, Zero_Val, Zero_Val,
                                                 15,       13,       11,       9,   
                                                 7,        5,        3,        1 );

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
    cat_img.loadFromFile( "img/cat_resize.bmp" );

    sf::Image table_img;
    table_img.loadFromFile( "img/table_resize.bmp" );

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

inline void AlphaBlendSSE( sf::Image* out_img, 
                           sf::Image* back_img, 
                           sf::Image* front_img, sf::Vector2u front_pos )
{
    assert( out_img   != NULL );
    assert( back_img  != NULL );
    assert( front_img != NULL );

    sf::Vector2u front_img_size = front_img->getSize();
    sf::Vector2u  back_img_size =  back_img->getSize();

    sf::Uint8*   out_img_pixels_ptr = ( sf::Uint8* )(   out_img->getPixelsPtr() );
    sf::Uint8*  back_img_pixels_ptr = ( sf::Uint8* )(  back_img->getPixelsPtr() );
    sf::Uint8* front_img_pixels_ptr = ( sf::Uint8* )( front_img->getPixelsPtr() );

    out_img->setPixel( 0,0, sf::Color( 50, 100, 200, 0 ) );

    printf( "%d %d %d %d\n", out_img_pixels_ptr[0], out_img_pixels_ptr[1], 
                            out_img_pixels_ptr[2], out_img_pixels_ptr[3] );

    memcpy( ( void* )out_img_pixels_ptr, 
            ( void* )back_img_pixels_ptr, 
            4 * back_img->getSize().y * back_img->getSize().x );

    for( unsigned int cur_y = 0; cur_y < front_img_size.y; cur_y++ )
    {
        for( unsigned int cur_x = 0; cur_x < front_img_size.x; cur_x += 4 )
        {
            // load 
            // | r3 g3 b3 a3 | r2 g2 b2 a2 | r1 g1 b1 a1 | r0 g0 b0 a0 |
            // 
            __m128i front_clr = _mm_load_si128( ( const __m128i* )
                                                ( &front_img_pixels_ptr[ 4*cur_x + 4*cur_y * front_img_size.x ] ) );
            //
            __m128i back_clr  = _mm_load_si128( ( const __m128i* )
                                                (  &back_img_pixels_ptr[ 4*( cur_x + front_pos.x) + 
                                                                         4*( cur_y + front_pos.y ) * back_img_size.x ] ) );
            //
            // | 0 0 0 0 | 0 0 0 0 | r3 g3 b3 a3 | r2 g2 b2 a2 | 
            //
            __m128i high_front_clr = ( __m128i )_mm_movehl_ps( ( __m128 )_mm_set1_epi8( 0 ), ( __m128 )front_clr );
            __m128i high_back_clr  = ( __m128i )_mm_movehl_ps( ( __m128 )_mm_set1_epi8( 0 ), ( __m128 )back_clr );
            // 
            // | 0 r1 0 g1 | 0 b1 0 a1 | 0 r0 0 g0 | 0 b0 0 a0 |
            //
            front_clr = _mm_cvtepu8_epi16( front_clr );
            back_clr  = _mm_cvtepu8_epi16( back_clr  );
            // 
            // | 0 r3 0 g3 | 0 b3 0 a3 | 0 r2 0 g2 | 0 b2 0 a2 |
            //
            high_front_clr = _mm_cvtepu8_epi16( high_front_clr );
            high_back_clr  = _mm_cvtepu8_epi16( high_back_clr  );
            //
            // | 0 a1 0 a1 | 0 a1 0 a1 | 0 a0 0 a0 | 0 a0 0 a0 |
            //
            __m128i low_front_alpha  = _mm_shuffle_epi8( front_clr,      
                                                         Alpha_Shuffle_Mask );
            //
            // | 0 a3 0 a3 | 0 a3 0 a3 | 0 a2 0 a2 | 0 a2 0 a2 |                        
            //   
            __m128i high_front_alpha = _mm_shuffle_epi8( high_front_clr, 
                                                         Alpha_Shuffle_Mask );
            //
            // mul ( rgb * a )
            //
            front_clr      = _mm_mullo_epi16( front_clr, 
                                              low_front_alpha );  
            high_front_clr = _mm_mullo_epi16( high_front_clr, 
                                              high_front_alpha );

            back_clr       = _mm_mullo_epi16( back_clr, 
                                              _mm_sub_epi16( FF_Arr, low_front_alpha ) );                     
            high_back_clr  = _mm_mullo_epi16( high_back_clr, 
                                              _mm_sub_epi16( FF_Arr, high_front_alpha ) );
            //
            // sum ( front_clr + back_clr )
            //
            __m128i low_sum  = _mm_add_epi16( front_clr, 
                                              back_clr );                                  
            __m128i high_sum = _mm_add_epi16( high_front_clr,
                                              high_back_clr );
            //
            // shuffle
            // | 0 0 0 0 | 0 0 0 0 | r1 g1 b1 a1 | r0 g0 b0 a0 |
            //
            low_sum  = _mm_shuffle_epi8( low_sum,  Sum_Shuffle_Mask );
            //
            // | 0 0 0 0 | 0 0 0 0 | r3 g3 b3 a3 | r2 g2 b2 a2 |                                
            //
            high_sum = _mm_shuffle_epi8( high_sum, Sum_Shuffle_Mask );

            __m128i new_color = ( __m128i )_mm_movelh_ps( ( __m128 )low_sum, 
                                                          ( __m128 )high_sum );
            //
            // store
            //
            _mm_store_si128( ( __m128i* )
                             ( &out_img_pixels_ptr[ 4*( cur_x + front_pos.x ) + 
                                                    4*( cur_y + front_pos.y ) * back_img_size.x ] ), new_color );
        }
    }
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