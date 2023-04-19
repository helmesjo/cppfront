
//  Copyright (c) Herb Sutter
//  SPDX-License-Identifier: CC-BY-NC-ND-4.0

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.


//===========================================================================
//  Reflection and meta
//===========================================================================

#ifndef __CPP2_REFLECT
#define __CPP2_REFLECT

#include "parse.h"

namespace cpp2 {

namespace meta {


//-----------------------------------------------------------------------
//
//  Compiler services
//
//-----------------------------------------------------------------------
//

class compiler_services
{
    std::vector<error_entry>* errors;
    int                       errors_original_size;
    std::deque<token>*        generated_tokens;
    cpp2::parser              parser;

public:
    compiler_services(
        std::vector<error_entry>* errors_,
        std::deque<token>*        generated_tokens_
    )
        : errors{errors_}
        , errors_original_size{static_cast<int>(std::ssize(*errors))}
        , generated_tokens{generated_tokens_}
        , parser{*errors}
    {
    }

protected:
    auto parse_statement(
        std::string_view source
    )
        -> std::unique_ptr<statement_node>
    {
        generated_lines.push_back({});
        auto &lines = generated_lines.back();

        auto add_line = [&] (std::string_view s) {
            lines.emplace_back( s, source_line::category::cpp2 );
        };

        //  First split this string into source_lines
        //
        if (auto newline_pos = source.find('\n');
            source.length() > 1
            && newline_pos != source.npos
            )
        {
            while (newline_pos != std::string_view::npos)
            {
                add_line( source.substr(0, newline_pos) );
                source.remove_prefix( newline_pos+1 );
                newline_pos = source.find('\n');
            }
        }
        if (!source.empty()) {
            add_line( source );
        }

        //  Now lex this source fragment to generate
        //  a single grammar_map entry, whose .second
        //  is the vector of tokens
        generated_lexers.emplace_back( *errors );
        auto& tokens = generated_lexers.back();
        tokens.lex( lines );
        assert(std::ssize(tokens.get_map()) == 1);

        //  Now parse this single declaration from
        //  the lexed tokens
        return parser.parse_one_declaration(
            tokens.get_map().begin()->second,
            *generated_tokens
        );
    }

public:
    auto require(
        bool             b,
        std::string_view msg,
        source_position  pos = source_position{}
    )
        -> void
    {
        if (!b) {
            errors->emplace_back( pos, msg );
        }
    }

};


//-----------------------------------------------------------------------
//
//  Declarations
//
//-----------------------------------------------------------------------
//

//  All declarations are noncopyable wrappers around a pointer to node
//
class declaration_base : public compiler_services
{
protected:
    //  Note: const member => implicitly =delete's copy/move
    declaration_node* const n;

    declaration_base(
        declaration_node*        n_,
        compiler_services const& s
    )
        : compiler_services{s}
        , n{n_}
    {
        assert(n && "a meta::declaration must point to a valid declaration_node, not null");
    }
};

class function_declaration;
class type_declaration;
class object_declaration;


//-----------------------------------------------------------------------
//  All declarations
//
class declaration
    : public declaration_base
{
public:
    declaration(
        declaration_node*        n_,
        compiler_services const& s
    )
        : declaration_base{n_, s}
    { }

    auto position() const -> source_position { return n->position(); }

    auto is_public        () const -> bool { return n->is_public(); }
    auto is_protected     () const -> bool { return n->is_protected(); }
    auto is_private       () const -> bool { return n->is_private(); }
    auto is_default_access() const -> bool { return n->is_default_access(); }

    auto make_public      () const -> bool { return n->make_public(); }
    auto make_protected   () const -> bool { return n->make_protected(); }
    auto make_private     () const -> bool { return n->make_private(); }

    auto has_name()                   const -> bool        { return n->has_name(); }
    auto has_name(std::string_view s) const -> bool        { return n->has_name(s); }

    auto name() const -> std::string_view {
        if (has_name()) { return n->name()->as_string_view(); }
        else            { return ""; }
    }

    auto has_initializer() const -> bool { return n->has_initializer(); }

    auto is_global   () const -> bool { return n->is_global(); }

    auto is_function () const -> bool { return n->is_function(); }
    auto is_object   () const -> bool { return n->is_object(); }
    auto is_type     () const -> bool { return n->is_type(); }
    auto is_namespace() const -> bool { return n->is_namespace(); }
    auto is_alias    () const -> bool { return n->is_alias(); }

    auto as_function() const -> function_declaration;
    auto as_object  () const -> object_declaration;
    auto as_type    () const -> type_declaration;

    auto parent_is_function   () const -> bool { return n->parent_is_function(); }
    auto parent_is_object     () const -> bool { return n->parent_is_object(); }
    auto parent_is_type       () const -> bool { return n->parent_is_type(); }
    auto parent_is_namespace  () const -> bool { return n->parent_is_namespace(); }
    auto parent_is_alias      () const -> bool { return n->parent_is_alias(); }
    auto parent_is_polymorphic() const -> bool { return n->parent_is_polymorphic(); }
};


//-----------------------------------------------------------------------
//  Function declarations
//
class function_declaration
    : public declaration
{
public:
    function_declaration(
        declaration_node*        n_,
        compiler_services const& s
    )
        : declaration{n_, s}
    {
        assert(n->is_function());
    }

    auto index_of_parameter_named(std::string_view s) const -> int  { return n->index_of_parameter_named(s); }
    auto has_parameter_named     (std::string_view s) const -> bool { return n->has_parameter_named(s); }
    auto has_in_parameter_named  (std::string_view s) const -> bool { return n->has_in_parameter_named(s); }
    auto has_out_parameter_named (std::string_view s) const -> bool { return n->has_out_parameter_named(s); }
    auto has_move_parameter_named(std::string_view s) const -> bool { return n->has_move_parameter_named(s); }

    auto is_function_with_this        () const -> bool { return n->is_function_with_this(); }
    auto is_virtual_function          () const -> bool { return n->is_virtual_function(); }
    auto is_constructor               () const -> bool { return n->is_constructor(); }
    auto is_constructor_with_that     () const -> bool { return n->is_constructor_with_that(); }
    auto is_constructor_with_in_that  () const -> bool { return n->is_constructor_with_in_that(); }
    auto is_constructor_with_move_that() const -> bool { return n->is_constructor_with_move_that(); }
    auto is_assignment                () const -> bool { return n->is_assignment(); }
    auto is_assignment_with_that      () const -> bool { return n->is_assignment_with_that(); }
    auto is_assignment_with_in_that   () const -> bool { return n->is_assignment_with_in_that(); }
    auto is_assignment_with_move_that () const -> bool { return n->is_assignment_with_move_that(); }
    auto is_destructor                () const -> bool { return n->is_destructor(); }

    auto is_copy_or_move() const -> bool { return is_constructor_with_that() || is_assignment_with_that(); }

    auto make_function_virtual() -> bool { return n->make_function_virtual(); }

    struct declared_that_funcs {
        bool out_this_in_that     = false;
        bool out_this_move_that   = false;
        bool inout_this_in_that   = false;;
        bool inout_this_move_that = false;;
    };

    auto query_declared_that_functions() const
        -> declared_that_funcs
    {
        auto declared = n->find_declared_that_functions();
        return {
            declared.out_this_in_that     != nullptr,
            declared.out_this_move_that   != nullptr,
            declared.inout_this_in_that   != nullptr,
            declared.inout_this_move_that != nullptr
        };
    }
};

auto declaration::as_function() const
    -> function_declaration
{
    return function_declaration(n, *this);
}


//-----------------------------------------------------------------------
//  Object declarations
//
class object_declaration
    : public declaration
{
public:
    object_declaration(
        declaration_node*        n_,
        compiler_services const& s
    )
        : declaration{n_, s}
    {
        assert(n->is_object());
    }

    auto is_const() const -> bool { return n->is_const(); }

    auto has_wildcard_type() const -> bool { return n->has_wildcard_type(); }

    // TODO: auto get_type() const -> 
        
};

auto declaration::as_object() const
    -> object_declaration
{
    return object_declaration(n, *this);
}


//-----------------------------------------------------------------------
//  Type declarations
//
class type_declaration
    : public declaration
{
public:
    type_declaration(
        declaration_node*        n_,
        compiler_services const& s
    )
        : declaration{n_, s}
    {
        assert(n->is_type());
    }

    auto is_polymorphic() const
        -> bool
    {
        return n->is_polymorphic();
    }

    auto get_member_functions() const
        -> std::vector<function_declaration>
    {
        auto ret = std::vector<function_declaration>{};
        for (auto d : n->get_type_scope_declarations(declaration_node::functions)) {
            ret.emplace_back( d, *this );
        }
        return ret;
    }

    auto get_member_objects() const
        -> std::vector<object_declaration>
    {
        auto ret = std::vector<object_declaration>{};
        for (auto& d : n->get_type_scope_declarations(declaration_node::objects)) {
            ret.emplace_back( d, *this );
        }
        return ret;
    }

    auto get_member_types() const
        -> std::vector<type_declaration>
    {
        auto ret = std::vector<type_declaration>{};
        for (auto& d : n->get_type_scope_declarations(declaration_node::types)) {
            ret.emplace_back( d, *this );
        }
        return ret;
    }

    auto get_members() const
        -> std::vector<declaration>
    {
        auto ret = std::vector<declaration>{};
        for (auto& d : n->get_type_scope_declarations(declaration_node::all)) {
            ret.emplace_back( d, *this );
        }
        return ret;
    }

    auto add_member( std::string_view source )
        -> bool
    {
        if (auto decl = parse_statement(source)) {
            return n->add_type_member(std::move(decl));
        }
        return false;
    }
};

auto declaration::as_type() const
    -> type_declaration
{
    return type_declaration(n, *this);
}


//  Close namespace subnamespace meta
}


//-----------------------------------------------------------------------
//
//  Meta functions - these are hardwired for now until we get to the
//  step of writing a Cpp2 interpreter to run inside the compiler
//
//-----------------------------------------------------------------------
//

//-----------------------------------------------------------------------
//  interface: an abstract base class having only pure virtual functions
//
auto interface( meta::type_declaration&  t )
    -> void
{
    bool has_dtor = false;
    for (auto m : t.get_members()) {
        m.require( !m.is_object(),
                   "interfaces may not contain data objects");
        if (m.is_function()) {
            auto mf = m.as_function();
            mf.require( !mf.is_copy_or_move(),
                        "interfaces may not copy or move; consider a virtual clone() instead");
            mf.require( !mf.has_initializer(),
                        "interface functions must not have a function body; remove the '=' initializer");
            mf.require( mf.make_public(),
                        "interface functions must be public");
            mf.make_function_virtual();
            has_dtor |= mf.is_destructor();
        }
    }
    if (!has_dtor) {
        t.require( t.add_member( "operator=: (virtual move this) = { }"),
                   "could not add pure virtual destructor");
    }
}


//  Now finish namespace cpp2, for the rest of the parser definition
//  and the currently-hardwired initial set of meta functions

auto parser::apply_type_meta_functions( declaration_node& n )
    -> bool
{
    assert(n.is_type());

    //  Get the reflection state ready to pass to the function
    auto cs = meta::compiler_services{ &errors, generated_tokens };
    auto rtype = meta::type_declaration{ &n, cs };

    //  For each meta function, apply it
    for (auto& meta : n.meta_functions) {
        if (meta->to_string() == "interface") {
            interface( rtype );
        }
        else {
            error( "(temporary alpha limitation) unrecognized meta function name - currently only unqualified 'interface' is supported" );
            return false;
        }
    }
    return true;
}


}

#endif