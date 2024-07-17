

#include <motor/profiling/probe_guard.hpp>

#include <motor/platform/global.h>

#include <motor/wire/node/node.h>

#include <motor/log/global.h>
#include <motor/memory/global.h>
#include <motor/concurrent/global.h>

#include <future>

namespace this_file
{
    using namespace motor::core::types ;

    class my_app : public motor::application::app
    {
        motor_this_typedefs( my_app ) ;

        motor::concurrent::task::tier_builder_t::build_result_t _tier_builder_result ;
        motor::wire::node::tier_builder_t::build_result_t _tier_builder_result_nodes ;

        virtual void_t on_init( void_t ) noexcept
        {
            MOTOR_PROBE( "application", "on_init" ) ;

            {
                motor::application::window_info_t wi ;
                wi.x = 100 ;
                wi.y = 100 ;
                wi.w = 800 ;
                wi.h = 600 ;
                wi.gen = motor::application::graphics_generation::gen4_auto ;
                
                this_t::send_window_message( this_t::create_window( wi ), [&]( motor::application::app::window_view & wnd )
                {
                    wnd.send_message( motor::application::show_message( { true } ) ) ;
                    wnd.send_message( motor::application::cursor_message_t( {true} ) ) ;
                    wnd.send_message( motor::application::vsync_message_t( { true } ) ) ;
                } ) ;
            }

            // init node graph
            {
                auto start = motor::shared( motor::wire::node( "start", [=] ( motor::wire::node_ptr_t )
                {
                } ), "start node" ) ;

                auto a = motor::shared( motor::wire::node( "a", [=] ( motor::wire::node_ptr_t )
                {
                } ), "node" ) ;

                auto b = motor::shared( motor::wire::node( "b",  [=] ( motor::wire::node_ptr_t )
                {
                    //std::this_thread::sleep_for( std::chrono::milliseconds( 20 ) ) ;
                    
                } ), "node" ) ;

                auto c = motor::shared( motor::wire::node( "c", [=] ( motor::wire::node_ptr_t )
                {
                } ), "node" ) ;

                auto d = motor::shared( motor::wire::node( "d", [=] ( motor::wire::node_ptr_t )
                {
                } ), "node" ) ;

                auto e = motor::shared( motor::wire::node( "e", [=] ( motor::wire::node_ptr_t )
                {
                } ), "node" ) ;

                auto f = motor::shared( motor::wire::node( "f", [&] ( motor::wire::node_ptr_t )
                {
                } ), "node" ) ;

                // #graph
                //         .-(a)-.     .-(d)-.
                // (start)-|     |-(c)-'-(e)-|
                //         '-(b)-'-----------'-(f)

                {
                    start->then( motor::share( a ) )->then( motor::share( c ) )->then( motor::share( d ) )->then( motor::share( f ) ) ;
                    start->then( motor::share( b ) )->then( motor::share( c ) )->then( motor::share( e ) )->then( motor::share( f ) ) ;
                    
                    b->then( motor::share( f ) ) ;
                }

                // #tiers for this graph
                // tier #1 | tier #2 | tier #3 | tier #4  | tier #5 
                //  start  |  a, b   |   c     |    d,e     |   f
                {
                    motor::concurrent::task::tier_builder_t tb ;

                    tb.build( start->get_task(), _tier_builder_result ) ;
                    

                    if ( _tier_builder_result.has_cylce ) motor::log::global_t::status( "graph has a cycle." ) ;
                    assert( _tier_builder_result.has_cylce == false ) ;
                }

                {
                    motor::wire::node::tier_builder_t tb ;

                    tb.build( start, _tier_builder_result_nodes ) ;

                    if ( _tier_builder_result_nodes.has_cylce ) motor::log::global_t::status( "graph has a cycle." ) ;
                    assert( _tier_builder_result_nodes.has_cylce == false ) ;
                }
            }
        }

        virtual void_t on_event( window_id_t const wid, 
                motor::application::window_message_listener::state_vector_cref_t sv ) noexcept
        {
            MOTOR_PROBE( "application", "on_event" ) ;

            if( sv.create_changed )
            {
                motor::log::global_t::status("[my_app] : window created") ;
            }
            if( sv.close_changed )
            {
                motor::log::global_t::status("[my_app] : window closed") ;
                this->close() ;
            }
        }

        virtual bool_t on_tool( this_t::window_id_t const wid, motor::application::app::tool_data_ref_t ) noexcept 
        { 
            MOTOR_PROBE( "application", "on_tool" ) ;

            // #1 : task graph window
            if( ImGui::Begin( "task graph visualization" ) )
            {
                ImNodes::BeginNodeEditor();

                ImVec2 cur_pos(0.0f,0.0f) ;//= ImGui::GetCursorPos() ; 
                
                // visualize tasks
                {
                    motor::hash_map< motor::concurrent::task_mtr_t, int_t > tasks_to_ids ;

                    int_t const num_nodes = int_t( _tier_builder_result.num_tasks ) ;

                    // visualize tasks and collect info
                    {
                        int_t nid = 0 ;
                        int_t tier_id = 0 ;
                        
                        for ( auto & tier : _tier_builder_result.tiers )
                        {
                            for ( auto * t : tier.tasks )
                            {
                                ImNodes::BeginNode( nid );

                                ImNodes::BeginNodeTitleBar();
                                ImGui::TextUnformatted( "node" );
                                ImNodes::EndNodeTitleBar();

                                #if 0
                                ImGui::Dummy( ImVec2() ) ;
                                #else
                                {
                                    int_t const base_attr_id = num_nodes + nid * 2 ;
                                    ImNodes::BeginInputAttribute( base_attr_id + 0 );
                                    ImGui::Text( "input" );
                                    ImNodes::EndInputAttribute();


                                    ImNodes::BeginOutputAttribute( base_attr_id + 1 );
                                    ImGui::Indent( 40 );
                                    ImGui::Text( "output" );
                                    ImNodes::EndOutputAttribute();
                                }
                                #endif

                                ImNodes::EndNode();
                                ImNodes::SetNodeGridSpacePos( nid, cur_pos ) ;

                                tasks_to_ids[ t ] = nid ;

                                ImVec2 const dims = ImNodes::GetNodeDimensions( nid ) ;

                                cur_pos.y += dims.y * 2.0f ;

                                ++nid ;
                            }

                            ImVec2 const dims( 50.0f, 1.0f ) ;
                            cur_pos.x += dims.x * 3.0f ;
                            cur_pos.y = 0.0f ;
                        }
                    }

                    // link all tasks
                    {
                        int_t link_id = 0 ;
                        motor::concurrent::task::tier_builder_t::output_slot_walk( _tier_builder_result,
                            [&] ( motor::concurrent::task_mtr_t t_in, motor::concurrent::task_t::tasks_in_t outputs )
                        {
                            int_t const tid = tasks_to_ids[ t_in ] ;

                            for ( auto * t : outputs )
                            {
                                int_t const oid = tasks_to_ids[ t ] ;

                                int_t const out_id = num_nodes + tid * 2 + 1 ;
                                int_t const in_id = num_nodes + oid * 2 + 0 ;
                                
                                ImNodes::Link( link_id++, in_id, out_id ) ;
                            }
                        } ) ;
                    }

                    ImNodes::MiniMap();
                    ImNodes::EndNodeEditor();

                    ImGui::End();
                }
            }


            // #2 : nodes graph window
            if ( ImGui::Begin( "node graph visualization" ) )
            {
                ImNodes::BeginNodeEditor();

                ImVec2 cur_pos( 0.0f, 0.0f ) ;//= ImGui::GetCursorPos() ; 

                // visualize tasks
                {
                    motor::hash_map< motor::wire::inode_mtr_t, int_t > nodes_to_ids ;

                    int_t const num_nodes = int_t( _tier_builder_result_nodes.num_nodes ) ;

                    // visualize tasks and collect info
                    {
                        int_t nid = 0 ;
                        int_t tier_id = 0 ;

                        for ( auto & tier : _tier_builder_result_nodes.tiers )
                        {
                            for ( auto * n : tier.nodes )
                            {
                                ImNodes::BeginNode( nid );

                                ImNodes::BeginNodeTitleBar();
                                ImGui::TextUnformatted( n->name().c_str() );
                                ImNodes::EndNodeTitleBar();

                                #if 0
                                ImGui::Dummy( ImVec2() ) ;
                                #else
                                {
                                    int_t const base_attr_id = num_nodes + nid * 2 ;
                                    ImNodes::BeginInputAttribute( base_attr_id + 0 );
                                    ImGui::Text( "input" );
                                    ImNodes::EndInputAttribute();


                                    ImNodes::BeginOutputAttribute( base_attr_id + 1 );
                                    ImGui::Indent( 40 );
                                    ImGui::Text( "output" );
                                    ImNodes::EndOutputAttribute();
                                }
                                #endif

                                ImNodes::EndNode();
                                ImNodes::SetNodeGridSpacePos( nid, cur_pos ) ;

                                nodes_to_ids[ n ] = nid ;

                                ImVec2 const dims = ImNodes::GetNodeDimensions( nid ) ;

                                cur_pos.y += dims.y * 2.0f ;

                                ++nid ;
                            }

                            ImVec2 const dims( 50.0f, 1.0f ) ;
                            cur_pos.x += dims.x * 3.0f ;
                            cur_pos.y = 0.0f ;
                        }
                    }

                    // link all tasks
                    {
                        int_t link_id = 0 ;
                        motor::wire::node::tier_builder_t::output_slot_walk( _tier_builder_result_nodes,
                            [&] ( motor::wire::inode_mtr_t n_in, motor::wire::inode::nodes_in_t outputs )
                        {
                            int_t const tid = nodes_to_ids[ n_in ] ;

                            for ( auto * n : outputs )
                            {
                                int_t const oid = nodes_to_ids[ n ] ;

                                int_t const out_id = num_nodes + tid * 2 + 1 ;
                                int_t const in_id = num_nodes + oid * 2 + 0 ;

                                ImNodes::Link( link_id++, in_id, out_id ) ;
                            }
                        } ) ;
                    }

                    ImNodes::MiniMap();
                    ImNodes::EndNodeEditor();

                    ImGui::End();
                }


            }

            return true ; 
        }

        virtual void_t on_shutdown( void_t ) noexcept {}
    };
}

int main( int argc, char ** argv )
{
    return motor::platform::global_t::create_and_exec< this_file::my_app >() ;
}