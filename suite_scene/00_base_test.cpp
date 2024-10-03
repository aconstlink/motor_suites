

#include <motor/scene/node/logic_group.h>
#include <motor/scene/node/switch_group.h>
#include <motor/scene/node/logic_leaf.h>
#include <motor/scene/node/logic_decorator.h>

#include <motor/scene/visitor/log_visitor.h>
#include <motor/scene/visitor/code_exe_visitor.h>

#include <motor/scene/component/code_component.h>
#include <motor/scene/global.h>

#include <motor/log/global.h>

using namespace motor::core::types ;

int main( int argc, char ** argv )
{
    motor::scene::global::init() ;

    // #1 : basic tree
    {
        motor::scene::logic_group_t root ;

        {
            auto g = motor::shared( motor::scene::logic_group() ) ;

            {
                auto g2 = motor::shared( motor::scene::logic_group() ) ;
                g2->add_child( motor::shared( motor::scene::logic_leaf_t() ) ) ;
                g2->add_child( motor::shared( motor::scene::logic_leaf_t() ) ) ;
                g->add_child( motor::move( g2 ) ) ;
            }

            g->add_child( motor::shared( motor::scene::logic_leaf_t() ) ) ;
            g->add_child( motor::shared( motor::scene::logic_leaf_t() ) ) ;
            g->add_child( motor::shared( motor::scene::logic_leaf_t() ) ) ;
            g->add_child( motor::shared( motor::scene::logic_leaf_t() ) ) ;

            {
                motor::scene::logic_group_t d ;
                d.add_child( motor::shared( motor::scene::logic_leaf_t() ) ) ;
                g->add_child( motor::shared( std::move( d ) ) ) ;
            }

            {
                g->add_child( motor::shared( motor::scene::logic_decorator( 
                    motor::shared( motor::scene::logic_leaf_t() ) ) ) ) ;
            }

            root.add_child( motor::move( g ) ) ;
        }

        // try log visitor and print the tree
        {
            motor::scene::log_visitor_t lv ;
            motor::scene::node_t::traverser( &root ).apply( &lv ) ;
        }
    }

    // #2 : try component
    {
        motor::scene::logic_group_t root ;

        {
            auto g = motor::shared( motor::scene::logic_group() ) ;

            // add component
            {
                g->add_component( motor::shared( motor::scene::code_component_t( [=] ( motor::scene::node_ptr_t )
                {
                    motor::log::global::status( "code in group g" ) ;
                } ) ) );

                // can not add a second component of the same type.
                {
                    auto res = g->add_component( motor::shared( motor::scene::code_component_t( [=] ( motor::scene::node_ptr_t )
                    {
                        motor::log::global::status( "code in group g" ) ;
                    } ) ) );

                    assert( res == false ) ;
                }
            }

            // add component
            {
                auto l = motor::shared( motor::scene::logic_leaf() ) ;
                l->add_component( motor::shared( motor::scene::code_component_t( [=] ( motor::scene::node_ptr_t )
                {
                    motor::log::global::status( "code in leaf l" ) ;
                } ) ) );
                g->add_child( motor::move( l ) ) ;
            }
            root.add_child( motor::move( g ) ) ;
        }

        #if 0
        {
            motor::scene::code_exe_visitor_t v ;
            motor::scene::node_t::traverser( &root ).apply( &v ) ;
        }
        #endif
    }

    motor::scene::global::deinit() ;
    motor::log::global::deinit() ;
    motor::memory::global::dump_to_std() ;

    return 0 ;
}