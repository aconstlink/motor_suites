#include <motor/msl/database.hpp>
#include <motor/msl/dependency_resolver.hpp>
#include <motor/msl/generators/generator.h>
#include <motor/msl/generators/hlsl5_generator.h>
#include <motor/msl/generators/glsl4_generator.h>
#include <motor/msl/parser.h>
#include <motor/log/global.h>
#include <motor/memory/global.h>

#include <cctype>

#if defined( _WIN32 )
#include <d3dcompiler.h>
#endif

namespace
{
    using namespace motor::core::types ;
    using stage_t = motor::msl::shader_type ;

    size_t occurrences( motor::string_cref_t code, motor::string_cref_t token )
    {
        size_t count = 0 ;
        for( size_t pos = code.find( token ) ; pos != motor::string_t::npos ;
            pos = code.find( token, pos + token.size() ) ) ++count ;
        return count ;
    }

    motor::string_t compact( motor::string_cref_t code )
    {
        motor::string_t result ; result.reserve( code.size() ) ;
        for( char const c : code ) if( !std::isspace( static_cast<unsigned char>( c ) ) ) result += c ;
        return result ;
    }

#if defined( _WIN32 )
    bool_t compile_hlsl( motor::string_cref_t code, stage_t const stage )
    {
        char const * entry = stage == stage_t::vertex_shader ? "VS" : stage == stage_t::geometry_shader ? "GS" : "PS" ;
        char const * target = stage == stage_t::vertex_shader ? "vs_5_0" : stage == stage_t::geometry_shader ? "gs_5_0" : "ps_5_0" ;
        ID3DBlob * bytecode = nullptr, * errors = nullptr ;
        auto const hr = D3DCompile( code.data(), code.size(), "msl_clip_depth", nullptr,
            nullptr, entry, target, D3DCOMPILE_ENABLE_STRICTNESS, 0, &bytecode, &errors ) ;
        if( FAILED( hr ) && errors != nullptr )
        {
            motor::log::global_t::error( motor::string_t(
                static_cast<char const *>( errors->GetBufferPointer() ), errors->GetBufferSize() ) ) ;
        }
        if( errors != nullptr ) errors->Release() ;
        if( bytecode != nullptr ) bytecode->Release() ;
        return SUCCEEDED( hr ) ;
    }
#endif

    motor::string_t source( char const * name, char const * geometry, bool_t const pixel )
    {
        motor::string_t code = motor::string_t( "config " ) + name + R"(
        {
            vertex_shader
            {
                in vec4_t vertex : position ;
                out vec4_t clip_coord : position ;
                out vec4_t raw_coord : texcoord0 ;
                float_t keep( float_t v ) { return v ; }
                void main()
                {
                    out.clip_coord = in.vertex ;
                    out.raw_coord = in.vertex ;
                    if( in.vertex.x < 0.0 ) { return ; }
                    out.clip_coord.x = keep( in.vertex.x ) ;
                    out.clip_coord.y = noise_1d( in.vertex.y ) ;
                }
            }
        )" ;
        if( geometry != nullptr )
        {
            code += motor::string_t( "geometry_shader { in points ; out " ) + geometry + R"([ max_verts = 3 ] ;
                inout vec4_t clip_coord : position ;
                inout vec4_t raw_coord : texcoord0 ;
                void main()
                {
                    int_t i = 0 ;
                    if( in[i].clip_coord.x < -100.0 ) { return ; }
                    out.clip_coord = in[i].clip_coord ;
                    out.raw_coord = in[i].raw_coord ;
                    emit_vertex() ;
                    emit_vertex() ;
                    out.clip_coord.x = out.clip_coord.x + 1.0 ;
                    emit_vertex() ;
                    end_primitive() ;
                }
            }
            )" ;
        }
        if( pixel )
        {
            code += R"(
            pixel_shader
            {
                in vec4_t raw_coord : texcoord0 ;
                out vec4_t color : color ;
                out float_t fragment_depth : depth ;
                void main()
                {
                    out.color = in.raw_coord ;
                    out.fragment_depth = 0.375 ;
                }
            }
            )" ;
        }
        return code + "}" ;
    }

    bool_t run_case( char const * name, char const * geometry, bool_t const pixel )
    {
        bool_t passed = true ;
        auto const check = [&]( char const * description, bool_t const condition )
        {
            motor::log::global_t::test( motor::string_t( "[msl clip depth] " ) + name + ": " + description, condition ) ;
            passed = condition && passed ;
        } ;

        motor::msl::database_t database ;
        motor::vector<motor::msl::symbol_t> symbols ;
        database.insert( motor::msl::parser_t( name ).process( source( name, geometry, pixel ) ), symbols ) ;
        check( "one configuration parsed", symbols.size() == 1 ) ;
        if( symbols.size() != 1 ) return false ;

        auto resolved = motor::msl::dependency_resolver_t().resolve( &database, symbols[0] ) ;
        check( "all dependencies resolved", resolved.missing.empty() ) ;
        if( !resolved.missing.empty() ) return false ;
        motor::msl::generator_t generator( std::move( resolved ) ) ;
        auto const hlsl = generator.generate<motor::msl::hlsl::hlsl5_generator_t>() ;
        auto const glsl = generator.generate<motor::msl::glsl::glsl4_generator_t>() ;
        check( "stage count", hlsl.shaders.size() == 1 + size_t( geometry != nullptr ) + size_t( pixel ) ) ;
        check( "stream-out contract", ( hlsl.streamout != motor::msl::streamout_type::none ) == !pixel ) ;

        for( auto const & shader : hlsl.shaders )
        {
            check( "one HLSL source per stage", shader.codes.size() == 1 ) ;
            if( shader.codes.size() != 1 ) continue ;
            auto const & raw = shader.codes[0].shader ;
            auto const code = compact( raw ) ;
            bool_t const correct = pixel && shader.type ==
                ( geometry == nullptr ? stage_t::vertex_shader : stage_t::geometry_shader ) ;
            check( "conversion only in the final raster stage",
                ( code.find( "__msl_raster_output__(" ) != motor::string_t::npos ) == correct ) ;

            if( correct )
            {
                motor::string_t position ;
                for( auto const & v : shader.variables )
                {
                    if( v.fq == motor::msl::flow_qualifier::out && v.binding == motor::msl::binding::position ) position = v.name ;
                }
                check( "renamed position binding resolved", !position.empty() ) ;
                motor::string_t const assignment = "value." + position + ".z=0.5f*(value." + position + ".z+value." + position + ".w);" ;
                check( "one depth conversion in a by-value output helper",
                    occurrences( code, assignment ) == 1 && code.find( "inout" +
                        motor::string_t( geometry == nullptr ? "vs_to_ps" : "gs_to_ps" ) + "value" ) == motor::string_t::npos &&
                    code.find( "__output__." + position + ".z=" ) == motor::string_t::npos ) ;
            }

            if( shader.type == stage_t::vertex_shader )
            {
                motor::string_t const output = correct ? "__msl_raster_output__(__output__)" : "__output__" ;
                check( "both early and final vertex returns", occurrences( code, "return" + output + ";" ) == 2 ) ;
            }
            else if( shader.type == stage_t::geometry_shader )
            {
                motor::string_t const output = correct ? "__msl_raster_output__(__output__)" : "__output__" ;
                check( "every emit, including repeated output, uses the correct copy",
                    occurrences( code, "prim_stream.Append(" + output + ")" ) == 3 &&
                    occurrences( code, "prim_stream.Append(" ) == 3 ) ;
            }
            else
            {
                motor::string_t depth ;
                for( auto const & v : shader.variables )
                    if( v.binding == motor::msl::binding::depth ) depth = v.name ;
                check( "explicit pixel depth is untouched", !depth.empty() &&
                    code.find( "__output__." + depth + "=0.375;" ) != motor::string_t::npos ) ;
            }
#if defined( _WIN32 )
            check( "generated HLSL compiles", compile_hlsl( raw, shader.type ) ) ;
#endif
        }
        for( auto const & shader : glsl.shaders )
            for( auto const & code : shader.codes )
                check( "GLSL has no depth-conversion helper", code.shader.find( "__msl_raster_output__" ) == motor::string_t::npos ) ;
        return passed ;
    }
}

int main( void_t )
{
    bool_t passed = true ;
    {
        passed = run_case( "vertex_raster", nullptr, true ) && passed ;
        passed = run_case( "geometry_points", "points", true ) && passed ;
        passed = run_case( "geometry_lines", "lines", true ) && passed ;
        passed = run_case( "geometry_triangles", "triangles", true ) && passed ;
        passed = run_case( "vertex_streamout", nullptr, false ) && passed ;
        passed = run_case( "geometry_streamout", "points", false ) && passed ;
    }
    motor::log::global_t::deinit() ;
    motor::memory::global_t::dump_to_std() ;
    return passed ? 0 : 1 ;
}
