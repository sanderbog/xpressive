///////////////////////////////////////////////////////////////////////////////
// examples2.hpp
//
//  Copyright 2006 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <boost/config.hpp>
#include <boost/mpl/min_max.hpp>
#include <boost/xpressive/proto3/proto.hpp>
#include <boost/xpressive/proto3/transform.hpp>
#include <boost/xpressive/proto3/transform/arg.hpp>
#include <boost/xpressive/proto3/transform/fold.hpp>
#include <boost/xpressive/proto3/transform/fold_tree.hpp>
#include <boost/utility/result_of.hpp>
#include <boost/fusion/include/cons.hpp>
#include <boost/fusion/include/pop_front.hpp>
#include <boost/test/unit_test.hpp>

using namespace boost::proto;
using namespace transform;
namespace mpl = boost::mpl;
namespace fusion = boost::fusion;

struct placeholder1 {};
struct placeholder2 {};

namespace test1
{
//[ CalculatorGrammar
    using namespace boost::proto;

    /*<< A Calculator expression is ... >>*/
    struct CalculatorGrammar
      : or_<
            /*<< placeholder1, or ... >>*/
            terminal< placeholder1 >
          /*<< placeholder2, or ... >>*/
          , terminal< placeholder2 >
          /*<< some other terminal, or ... >>*/
          , terminal< _ >
          /*<< a unary expression where the operand is a calculator expression, or ... >>*/
          , unary_expr< _, CalculatorGrammar >
          /*<< a binary expression where the operands are calculator expressions, or ... >>*/
          , binary_expr< _, CalculatorGrammar, CalculatorGrammar >
        >
    {};
//]
}

//[ binary_arity
/*<< The `CalculatorArity` is a transform for calculating
the arity of a calculator expression. It will be define in
terms of `binary_arity`, which is defined in terms of 
`CalculatorArity`; hence, the definition is recursive.>>*/
struct CalculatorArity;

// A custom transform that returns the arity of a unary
// calculator expression by finding the arity of the
// child expression.
struct unary_arity
  /*<< All custom transforms should inherit from
  transform_base. In some cases, (e.g., when the transform
  is a template), it is also necessary to specialize 
  the proto::is_transform<> trait. >>*/
  : raw_transform
{
    template<typename Expr, typename State, typename Visitor>
    /*<< Transforms have a nested `apply<>` for calculating their return type. >>*/
    struct apply
    {
        /*<< Get the child. >>*/
        typedef typename result_of::arg<Expr>::type child_expr;

        /*<< Apply `CalculatorArity` to find the arity of the child. >>*/
        typedef typename mpl::apply_wrap3<CalculatorArity, child_expr, State, Visitor>::type type;
    };

    template<typename Expr, typename State, typename Visitor>
    static typename apply<Expr, State, Visitor>::type
    /*<< Transforms have a nested `call()` member function. >>*/
    call(Expr const &, State const &, Visitor &)
    {
        /*<< The `unary_arity` transform doesn't have an interesting
        runtime counterpart, so just return a default-constructed object
        of the correct type. >>*/
        return typename apply<Expr, State, Visitor>::type();
    }
};

// A custom transform that returns the arity of a binary
// calculator expression by finding the maximum of the
// arities of the two children expressions.
struct binary_arity
  /*<< All custom transforms should inherit from
  transform_base. In some cases, (e.g., when the transform
  is a template), it is also necessary to specialize 
  the proto::is_transform<> trait. >>*/
  : raw_transform
{
    template<typename Expr, typename State, typename Visitor>
    /*<< Transforms have a nested `apply<>` for calculating their return type. >>*/
    struct apply
    {
        /*<< Get the left and right children. >>*/
        typedef typename result_of::left<Expr>::type left_expr;
        typedef typename result_of::right<Expr>::type right_expr;

        /*<< Apply `CalculatorArity` to find the arity of the left and right children. >>*/
        typedef typename mpl::apply_wrap3<CalculatorArity, left_expr, State, Visitor>::type left_arity;
        typedef typename mpl::apply_wrap3<CalculatorArity, right_expr, State, Visitor>::type right_arity;

        /*<< The return type is the maximum of the children's arities. >>*/
        typedef typename mpl::max<left_arity, right_arity>::type type;
    };

    template<typename Expr, typename State, typename Visitor>
    static typename apply<Expr, State, Visitor>::type
    /*<< Transforms have a nested `call()` member function. >>*/
    call(Expr const &, State const &, Visitor &)
    {
        /*<< The `binary_arity` transform doesn't have an interesting
        runtime counterpart, so just return a default-constructed object
        of the correct type. >>*/
        return typename apply<Expr, State, Visitor>::type();
    }
};
//]

struct zero : mpl::int_<0> {};
struct one  : mpl::int_<1> {};
struct two  : mpl::int_<2> {};

terminal< placeholder1 >::type const _1 = {{}};
terminal< placeholder2 >::type const _2 = {{}};

//[ CalculatorArityGrammar
struct CalculatorArity
  : or_<
        case_< terminal< placeholder1 >,    one() >
      , case_< terminal< placeholder2 >,    two() >
      , case_< terminal<_>,                 zero() >
      , case_< unary_expr<_, _>,            unary_arity >
      , case_< binary_expr<_, _, _>,        binary_arity >
    >
{};
//]

//[ CalculatorArityGrammar2
struct CalcArity2
  : or_<
        case_< terminal< placeholder1 >,                one()               >
      , case_< terminal< placeholder2 >,                two()               >
      , case_< terminal<_>,                             zero()              >
      , case_< unary_expr<_, CalcArity2>,               CalcArity2(_arg)    >
      , case_< binary_expr<_, CalcArity2, CalcArity2>,  mpl::max<CalcArity2(_left), CalcArity2(_right)>()   >
    >
{};
//]

//[ AsArgList
// This transform matches function invocations such as foo(1,'a',"b")
// and transforms them into Fusion cons lists of their arguments. In this
// case, the result would be cons(1, cons('a', cons("b", nil()))).
struct ArgsAsList
  : case_<
        function<terminal<_>, vararg<terminal<_> > >
      /*<< Use a `reverse_fold<>` transform to iterate over the children
      of this node in reverse order, building a fusion list from back to
      front. >>*/
      , reverse_fold<
            /*<< The first child expression of a `function<>` node is the
            function being invoked. We don't want that in our list, so use
            the `pop_front()` to remove it. >>*/
            _pop_front(_)    // make (_) optional
          /*<< `nil` is the initial state used by the `reverse_fold<>`
          transform. >>*/
          , fusion::nil()
          /*<< Put the rest of the function arguments in a fusion cons
          list. >>*/
          , fusion::cons<_arg, _state>(_arg, _state)
        >
    >
{};
//]

//[ FoldTreeToList
// This transform matches expressions of the form (_1=1,'a',"b")
// (note the use of the comma operator) and transforms it into a
// Fusion cons list of their arguments. In this case, the result
// would be cons(1, cons('a', cons("b", nil()))).
struct FoldTreeToList
  : or_<
        // This grammar describes what counts as the terminals in expressions
        // of the form (_1=1,'a',"b"), which will be flattened using
        // reverse_fold_tree<> below.
        case_<assign<_, terminal<_> >
             , _arg(_right)
        >
      , case_<terminal<_>
             , _arg
        >
      , case_<
            comma<FoldTreeToList, FoldTreeToList>
          /*<< Fold all terminals that are separated by commas into a Fusion cons list. >>*/
          , reverse_fold_tree<
                _
              , fusion::nil()
              , fusion::cons<FoldTreeToList, _state>(FoldTreeToList, _state)
            >
        >
    >
{};
//]

//[ Promote
// This transform finds all float terminals in an expression and promotes
// them to doubles.
struct Promote
  : or_<
        /*<< Match a `terminal<float>`, then construct a 
        `terminal<double>::type` with the `float`. >>*/
        case_<terminal<float>, terminal<double>::type(_arg) >
      , case_<terminal<_> >
      /*<< `nary_expr<>` has a pass-through transform which 
      will transform each child sub-expression using the 
      `Promote` transform. >>*/
      , case_<nary_expr<_, vararg<Promote> > >
    >
{};
//]

//[ LazyMakePair
struct make_pair_tag {};
terminal<make_pair_tag>::type const make_pair_ = {{}};

// This transform matches lazy function invocations like
// `make_pair_(1, 3.14)` and actually builds a `std::pair<>`
// from the arguments.
struct MakePair
  : case_<
        /*<< Match expressions like `make_pair_(1, 3.14)` >>*/
        function<terminal<make_pair_tag>, terminal<_>, terminal<_> >
      /*<< Return `std::pair<F,S>(f,s)` where `f` and `s` are the
      first and second arguments to the lazy `make_pair_()` function >>*/
      , std::pair<_arg(_arg1), _arg(_arg2)>(_arg(_arg1), _arg(_arg2))
    >
{};
//]

//[ NegateInt
struct NegateInt
  : case_<terminal<int>, negate<_>(_)>
{};
//]

#ifndef BOOST_MSVC
//[ SquareAndPromoteInt
struct SquareAndPromoteInt
  : case_<
        terminal<int>
      , multiplies<terminal<long>::type, terminal<long>::type>::type
            (terminal<long>::type(_arg), terminal<long>::type(_arg))
    >
{};
//]
#endif

void test_examples()
{
    //[ CalculatorArityTest
    int i = 0; // not used, dummy state and visitor parameter

    std::cout << CalculatorArity::call( lit(100) * 200, i, i) << '\n';
    std::cout << CalculatorArity::call( (_1 - _1) / _1 * 100, i, i) << '\n';
    std::cout << CalculatorArity::call( (_2 - _1) / _2 * 100, i, i) << '\n';
    //]

    BOOST_CHECK_EQUAL(0, CalculatorArity::call( lit(100) * 200, i, i));
    BOOST_CHECK_EQUAL(1, CalculatorArity::call( (_1 - _1) / _1 * 100, i, i));
    BOOST_CHECK_EQUAL(2, CalculatorArity::call( (_2 - _1) / _2 * 100, i, i));

    BOOST_CHECK_EQUAL(0, CalcArity2::call( lit(100) * 200, i, i));
    BOOST_CHECK_EQUAL(1, CalcArity2::call( (_1 - _1) / _1 * 100, i, i));
    BOOST_CHECK_EQUAL(2, CalcArity2::call( (_2 - _1) / _2 * 100, i, i));

    using boost::fusion::cons;
    using boost::fusion::nil;
    cons<int, cons<char, cons<std::string> > > args(ArgsAsList::call( _1(1, 'a', std::string("b")), i, i ));
    BOOST_CHECK_EQUAL(args.car, 1);
    BOOST_CHECK_EQUAL(args.cdr.car, 'a');
    BOOST_CHECK_EQUAL(args.cdr.cdr.car, std::string("b"));

    cons<int, cons<char, cons<std::string> > > lst(FoldTreeToList::call( (_1 = 1, 'a', std::string("b")), i, i ));
    BOOST_CHECK_EQUAL(lst.car, 1);
    BOOST_CHECK_EQUAL(lst.cdr.car, 'a');
    BOOST_CHECK_EQUAL(lst.cdr.cdr.car, std::string("b"));

    plus<
        terminal<double>::type
      , terminal<double>::type
    >::type p = Promote::call( lit(1.f) + 2.f, i, i );

    //[ LazyMakePairTest
    int j = 0; // not used, dummy state and visitor parameter

    std::pair<int, double> p2 = MakePair::call( make_pair_(1, 3.14), j, j );
    
    std::cout << p2.first << std::endl;
    std::cout << p2.second << std::endl;
    //]

    BOOST_CHECK_EQUAL(p2.first, 1);
    BOOST_CHECK_EQUAL(p2.second, 3.14);

    NegateInt::call(lit(1), i, i);
#ifndef BOOST_MSVC
    SquareAndPromoteInt::call(lit(1), i, i);
#endif
}

using namespace boost::unit_test;
///////////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
//
test_suite* init_unit_test_suite( int argc, char* argv[] )
{
    test_suite *test = BOOST_TEST_SUITE("test examples from the documentation");

    test->add(BOOST_TEST_CASE(&test_examples));

    return test;
}
