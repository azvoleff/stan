#ifndef __STAN__GM__PARSER__EXPRESSION_GRAMMAR_DEF__HPP__
#define __STAN__GM__PARSER__EXPRESSION_GRAMMAR_DEF__HPP__

#include <cstddef>
#include <iomanip>
#include <iostream>
#include <istream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <stdexcept>

#include <boost/spirit/include/qi.hpp>
// FIXME: get rid of unused include
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_function.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/include/phoenix_object.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/std_pair.hpp>
#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/qi_numeric.hpp>
#include <boost/spirit/include/classic_position_iterator.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_function.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/include/phoenix_object.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/spirit/include/support_multi_pass.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/recursive_variant.hpp>

#include <stan/gm/ast.hpp>
#include <stan/gm/grammars/whitespace_grammar.hpp>
#include <stan/gm/grammars/expression_grammar.hpp>

BOOST_FUSION_ADAPT_STRUCT(stan::gm::index_op,
                          (stan::gm::expression, expr_)
                          (std::vector<std::vector<stan::gm::expression> >, 
                           dimss_) )

BOOST_FUSION_ADAPT_STRUCT(stan::gm::fun,
                          (std::string, name_)
                          (std::vector<stan::gm::expression>, args_) )

BOOST_FUSION_ADAPT_STRUCT(stan::gm::int_literal,
                          (int,val_)
                          (stan::gm::expr_type,type_))

BOOST_FUSION_ADAPT_STRUCT(stan::gm::double_literal,
                          (double,val_)
                          (stan::gm::expr_type,type_) )

BOOST_FUSION_ADAPT_STRUCT(stan::gm::array_literal,
                          (std::vector<stan::gm::expression>,args_))
//                          (stan::gm::expr_type,type_) )

namespace stan { 

  namespace gm {


    struct set_array_type {
      template <typename T1, typename T2>
      struct result { typedef array_literal type; };

      array_literal operator()(array_literal& array_lit,
                               std::ostream& error_msgs) const {
        if (array_lit.args_.size() == 0) {
          array_lit.type_ = expr_type(DOUBLE_T,1U);
          // print warning about default to double
          return array_lit;
        }
        size_t elt_size = array_lit.args_[0].expression_type().num_dims_;
        base_expr_type base_type = array_lit.args_[0].expression_type().base_type_;
        for (size_t i = 1; i < array_lit.args_.size(); ++i) {
          if (elt_size != array_lit.args_[i].expression_type().num_dims_
              || base_type != array_lit.args_[i].expression_type().base_type_) {
            array_lit.type_ = expr_type(); // ill-formed
            // error message here about mismatch
          }
        }
        array_lit.type_ = expr_type(base_type, 1U + elt_size);
        return array_lit;
      }
    };
    boost::phoenix::function<set_array_type> set_array_type_f;

    struct set_fun_type {
      template <typename T1, typename T2>
      struct result { typedef fun type; };

      fun operator()(fun& fun,
                     std::ostream& error_msgs) const {
        std::vector<expr_type> arg_types;
        for (size_t i = 0; i < fun.args_.size(); ++i)
          arg_types.push_back(fun.args_[i].expression_type());
        fun.type_ = function_signatures::instance().get_result_type(fun.name_,
                                                                    arg_types,
                                                                    error_msgs);
        return fun;
      }
    };
    boost::phoenix::function<set_fun_type> set_fun_type_f;


    struct binary_op_expr {
      template <typename T1, typename T2, typename T3, typename T4, typename T5>
      struct result { typedef expression type; };

      expression operator()(expression& expr1,
                            const expression& expr2,
                            const std::string& op,
                            const std::string& fun_name,
                            std::ostream& error_msgs) const {
        if (!expr1.expression_type().is_primitive()
            || !expr2.expression_type().is_primitive()) {
          error_msgs << "binary infix operator "
                     << op 
                     << " with functional interpretation "
                     << fun_name
                     << " requires arguments or primitive type (int or real)"
                     << ", found left type=" << expr1.expression_type()
                     << ", right arg type=" << expr2.expression_type()
                     << "; ";
        }
        std::vector<expression> args;
        args.push_back(expr1);
        args.push_back(expr2);
        set_fun_type sft;
        fun f(fun_name,args);
        sft(f,error_msgs);
        return expression(f);
      }
    };
    boost::phoenix::function<binary_op_expr> binary_op_f;



    struct addition_expr {
      template <typename T1, typename T2, typename T3>
      struct result { typedef expression type; };

      expression operator()(expression& expr1,
                            const expression& expr2,
                            std::ostream& error_msgs) const {
        if (expr1.expression_type().is_primitive()
            && expr2.expression_type().is_primitive()) {
          return expr1 += expr2;
        }
        std::vector<expression> args;
        args.push_back(expr1);
        args.push_back(expr2);
        set_fun_type sft;
        fun f("add",args);
        sft(f,error_msgs);
        return expression(f);
        return expr1 += expr2;
      }
    };
    boost::phoenix::function<addition_expr> addition;


    struct subtraction_expr {
      template <typename T1, typename T2, typename T3>
      struct result { typedef expression type; };

      expression operator()(expression& expr1,
                            const expression& expr2,
                            std::ostream& error_msgs) const {
        if (expr1.expression_type().is_primitive()
            && expr2.expression_type().is_primitive()) {
          return expr1 -= expr2;
        }
        std::vector<expression> args;
        args.push_back(expr1);
        args.push_back(expr2);
        set_fun_type sft;
        fun f("subtract",args);
        sft(f,error_msgs);
        return expression(f);
      }
    };
    boost::phoenix::function<subtraction_expr> subtraction;

    struct multiplication_expr {
      template <typename T1, typename T2, typename T3>
      struct result { typedef expression type; };

      expression operator()(expression& expr1,
                            const expression& expr2,
                            std::ostream& error_msgs) const {

        if (expr1.expression_type().is_primitive()
            && expr2.expression_type().is_primitive()) {
          return expr1 *= expr2;
        }
        std::vector<expression> args;
        args.push_back(expr1);
        args.push_back(expr2);
        set_fun_type sft;
        fun f("multiply",args);
        sft(f,error_msgs);
        return expression(f);
      }
    };
    boost::phoenix::function<multiplication_expr> multiplication;

    void generate_expression(const expression& e, std::ostream& o);

    struct division_expr {
      template <typename T1, typename T2, typename T3>
      struct result { typedef expression type; };

      expression operator()(expression& expr1,
                            const expression& expr2,
                            std::ostream& error_msgs) const {
        if (expr1.expression_type().is_primitive_int() 
            && expr2.expression_type().is_primitive_int()) {
          // getting here, but not printing?  only print error if problems?
          error_msgs << "Warning: integer division implicitly rounds to integer."
                     << " Found int division: ";
          generate_expression(expr1.expr_,error_msgs);
          error_msgs << " / ";
          generate_expression(expr2.expr_,error_msgs);
          error_msgs << std::endl
                     << " Positive values rounded down, negative values rounded up or down"
                     << " in platform-dependent way."
                     << std::endl;
        }
            
        if (expr1.expression_type().is_primitive()
            && expr2.expression_type().is_primitive()) {
          return expr1 /= expr2;
        }
        std::vector<expression> args;
        args.push_back(expr1);
        args.push_back(expr2);
        set_fun_type sft;
        if ((expr1.expression_type().type() == MATRIX_T
             || expr1.expression_type().type() == ROW_VECTOR_T)
            && expr2.expression_type().type() == MATRIX_T) {
          fun f("mdivide_right",args);
          sft(f,error_msgs);
          return expression(f);
        }
        
        fun f("divide",args);
        sft(f,error_msgs);
        return expression(f);
      }
    };
    boost::phoenix::function<division_expr> division;

    struct left_division_expr {
      template <typename T1, typename T2, typename T3>
      struct result { typedef expression type; };

      expression operator()(expression& expr1,
                            const expression& expr2,
                            std::ostream& error_msgs) const {
        if (expr1.expression_type().is_primitive()
            && expr2.expression_type().is_primitive()) {
          return expr1 /= expr2;
        }
        std::vector<expression> args;
        args.push_back(expr1);
        args.push_back(expr2);
        set_fun_type sft;
        if (expr1.expression_type().type() == MATRIX_T
            && (expr2.expression_type().type() == VECTOR_T
                || expr2.expression_type().type() == MATRIX_T)) {
          fun f("mdivide_left",args);
          sft(f,error_msgs);
          return expression(f);
        }
        fun f("divide_left",args);
        sft(f,error_msgs);
        return expression(f);
      }
    };
    boost::phoenix::function<left_division_expr> left_division;

    struct elt_multiplication_expr {
      template <typename T1, typename T2, typename T3>
      struct result { typedef expression type; };

      expression operator()(expression& expr1,
                            const expression& expr2,
                            std::ostream& error_msgs) const {

        if (expr1.expression_type().is_primitive()
            && expr2.expression_type().is_primitive()) {
          return expr1 *= expr2;
        }
        std::vector<expression> args;
        args.push_back(expr1);
        args.push_back(expr2);
        set_fun_type sft;
        fun f("elt_multiply",args);
        sft(f,error_msgs);
        return expression(f);
        return expr1 += expr2;
      }
    };
    boost::phoenix::function<elt_multiplication_expr> elt_multiplication;

    struct elt_division_expr {
      template <typename T1, typename T2, typename T3>
      struct result { typedef expression type; };

      expression operator()(expression& expr1,
                            const expression& expr2,
                            std::ostream& error_msgs) const {

        if (expr1.expression_type().is_primitive()
            && expr2.expression_type().is_primitive()) {
          return expr1 /= expr2;
        }
        std::vector<expression> args;
        args.push_back(expr1);
        args.push_back(expr2);
        set_fun_type sft;
        fun f("elt_divide",args);
        sft(f,error_msgs);
        return expression(f);
        return expr1 += expr2;
      }
    };
    boost::phoenix::function<elt_division_expr> elt_division;

    // Cut-and-Paste from Spirit examples, including comment:  We
    // should be using expression::operator-. There's a bug in phoenix
    // type deduction mechanism that prevents us from doing
    // so. Phoenix will be switching to BOOST_TYPEOF. In the meantime,
    // we will use a phoenix::function below:
    struct negate_expr {
      template <typename T1, typename T2>
      struct result { typedef expression type; };

      expression operator()(const expression& expr,
                            std::ostream& error_msgs) const {
        if (expr.expression_type().is_primitive()) {
          return expression(unary_op('-', expr));
        }
        std::vector<expression> args;
        args.push_back(expr);
        set_fun_type sft;
        fun f("minus",args);
        sft(f,error_msgs);
        return expression(f);
      }
    };
    boost::phoenix::function<negate_expr> negate_expr_f;

    struct logical_negate_expr {
      template <typename T1, typename T2>
      struct result { typedef expression type; };

      expression operator()(const expression& expr,
                            std::ostream& error_msgs) const {
        if (!expr.expression_type().is_primitive()) {
          error_msgs << "logical negation operator ! only applies to int or real types; ";
          return expression();
        }
        std::vector<expression> args;
        args.push_back(expr);
        set_fun_type sft;
        fun f("logical_negation",args);
        sft(f,error_msgs);
        return expression(f);
      }
    };
    boost::phoenix::function<logical_negate_expr> logical_negate_expr_f;

    struct transpose_expr {
      template <typename T1, typename T2>
      struct result { typedef expression type; };

      expression operator()(const expression& expr,
                            std::ostream& error_msgs) const {
        if (expr.expression_type().is_primitive()) {
          return expr; // transpose of basic is self -- works?
        }
        std::vector<expression> args;
        args.push_back(expr);
        set_fun_type sft;
        fun f("transpose",args);
        sft(f,error_msgs);
        return expression(f);
      }
    };
    boost::phoenix::function<transpose_expr> transpose_f;

    struct add_expression_dimss {
      template <typename T1, typename T2, typename T3, typename T4>
      struct result { typedef T1 type; };
      expression operator()(expression& expression,
                            std::vector<std::vector<stan::gm::expression> >& dimss,
                            bool& pass,
                            std::ostream& error_msgs) const {
        index_op iop(expression,dimss);
        iop.infer_type();
        if (iop.type_.is_ill_formed()) {
          error_msgs << "indexes inappropriate for expression." << std::endl;
          pass = false;
        } else {
          pass = true;
        }
        return iop;
      }
    };
    boost::phoenix::function<add_expression_dimss> add_expression_dimss_f;

    struct set_var_type {
      template <typename T1, typename T2, typename T3, typename T4>
      struct result { typedef variable type; };
      variable operator()(variable& var_expr, 
                          variable_map& vm,
                          std::ostream& error_msgs,
                          bool& pass) const {
        std::string name = var_expr.name_;
        if (!vm.exists(name)) {
          pass = false;
          error_msgs << "variable \"" << name << '"' << " does not exist." 
                     << std::endl;
          return var_expr;
        }
        pass = true;
        var_expr.set_type(vm.get_base_type(name),vm.get_num_dims(name));
        return var_expr;
      }
    };
    boost::phoenix::function<set_var_type> set_var_type_f;

    struct validate_int_expr3 {
      template <typename T1, typename T2>
      struct result { typedef bool type; };

      bool operator()(const expression& expr,
                      std::stringstream& error_msgs) const {
        if (!expr.expression_type().is_primitive_int()) {
          error_msgs << "expression denoting integer required; found type=" 
                     << expr.expression_type() << std::endl;
          return false;
        }
        return true;
      }
    };
    boost::phoenix::function<validate_int_expr3> validate_int_expr3_f;


    struct validate_expr_type {
      template <typename T1, typename T2>
      struct result { typedef bool type; };

      bool operator()(const expression& expr,
                      std::ostream& error_msgs) const {
        if (expr.expression_type().is_ill_formed()) {
          error_msgs << "expression is ill formed" << std::endl;
          return false;
        }
        return true;
      }
    };
    boost::phoenix::function<validate_expr_type> validate_expr_type_f;

    



    template <typename Iterator>
    expression_grammar<Iterator>::expression_grammar(variable_map& var_map,
                                                     std::stringstream& error_msgs,
                                                     bool allow_lte)
      : expression_grammar::base_type(allow_lte ? expression_r : expression07_r),
        var_map_(var_map),
        error_msgs_(error_msgs) 
    {
      using boost::spirit::qi::_1;
      using boost::spirit::qi::char_;
      using boost::spirit::qi::double_;
      using boost::spirit::qi::eps;
      using boost::spirit::qi::int_;
      using boost::spirit::qi::lexeme;
      using boost::spirit::qi::lit;
      using boost::spirit::qi::_pass;
      using boost::spirit::qi::_val;



      expression_r.name("expression");
      expression_r
        %= expression15_r;


      expression15_r.name("expression, precedence 15, binary ||");
      expression15_r 
        = expression14_r [_val = _1]
        > *( lit("||") 
             > expression15_r  [_val = binary_op_f(_val,_1,"||","logical_or",
                                                   boost::phoenix::ref(error_msgs))] 
             );

      expression14_r.name("expression, precedence 14, binary &&");
      expression14_r 
        = expression10_r [_val = _1]
        > *( lit("&&") 
             > expression14_r  [_val = binary_op_f(_val,_1,"&&","logical_and",
                                                   boost::phoenix::ref(error_msgs))] 
             );

      expression10_r.name("expression, precedence 10, binary ==, !=");
      expression10_r 
        = expression09_r [_val = _1]
        > *( ( lit("==") 
               > expression10_r  [_val = binary_op_f(_val,_1,"==","logical_eq",
                                                       boost::phoenix::ref(error_msgs))] )
              |
              ( lit("!=") 
                > expression10_r  [_val = binary_op_f(_val,_1,"!=","logical_neq",
                                                      boost::phoenix::ref(error_msgs))] ) 
              );

      expression09_r.name("expression, precedence 9, binary <, <=, >, >=");
      expression09_r 
        = expression07_r [_val = _1]
        > *( ( lit("<=")
               > expression09_r  [_val = binary_op_f(_val,_1,"<","logical_lt",
                                                      boost::phoenix::ref(error_msgs))] )
              |
              ( lit("<") 
                > expression09_r  [_val = binary_op_f(_val,_1,"<=","logical_lte",
                                                      boost::phoenix::ref(error_msgs))] ) 
              |
              ( lit(">=") 
                > expression09_r  [_val = binary_op_f(_val,_1,">","logical_gt",
                                                      boost::phoenix::ref(error_msgs))] ) 
              |
              ( lit(">") 
                > expression09_r  [_val = binary_op_f(_val,_1,">=","logical_gte",
                                                      boost::phoenix::ref(error_msgs))] ) 
              );
      
      expression07_r.name("expression, precedence 7, binary +, -");
      expression07_r 
        =  term_r                     
            [_val = _1]
        > *( ( lit('+')
                > expression07_r       
                [_val = addition(_val,_1,boost::phoenix::ref(error_msgs))] )
              |  
              ( lit('-') 
                > expression07_r   
                [_val = subtraction(_val,_1,boost::phoenix::ref(error_msgs))] )
              )
        > eps[_pass = validate_expr_type_f(_val,boost::phoenix::ref(error_msgs_))]
        ;

      term_r.name("term");
      term_r 
        = ( negated_factor_r                       [_val = _1]
             >> *( (lit('*') > negated_factor_r     
                               [_val = multiplication(_val,_1,boost::phoenix::ref(error_msgs_))])
                   | (lit('/') > negated_factor_r   
                      [_val = division(_val,_1,boost::phoenix::ref(error_msgs_))])
                   | (lit('\\') > negated_factor_r   [_val = left_division(_val,_1,boost::phoenix::ref(error_msgs_))])
                   | (lit(".*") > negated_factor_r   
                      [_val = elt_multiplication(_val,_1,boost::phoenix::ref(error_msgs_))])
                   | (lit("./") > negated_factor_r   
                      [_val = elt_division(_val,_1,boost::phoenix::ref(error_msgs_))])
                   )
             )
        ;

      negated_factor_r 
        = lit('-') >> negated_factor_r 
                      [_val = negate_expr_f(_1,boost::phoenix::ref(error_msgs_))]
        | lit('!') >> negated_factor_r 
                      [_val = logical_negate_expr_f(_1,boost::phoenix::ref(error_msgs_))]
        | lit('+') >> negated_factor_r  [_val = _1]
        | indexed_factor_r [_val = _1];


      indexed_factor_r.name("(optionally) indexed factor [sub]");
      indexed_factor_r 
        = factor_r [_val = _1]
        > * (  
               (+dims_r) 
               [_val = add_expression_dimss_f(_val, _1, _pass,
                                            boost::phoenix::ref(error_msgs_))]
               | 
               lit("'") 
               [_val = transpose_f(_val, boost::phoenix::ref(error_msgs_))] 
               )
        ;

      factor_r.name("factor");
      factor_r
        =  int_literal_r     [_val = _1]
        | double_literal_r    [_val = _1]
        | array_literal_r     [_val = set_array_type_f(_1,boost::phoenix::ref(error_msgs_))]
        | fun_r               [_val = set_fun_type_f(_1,boost::phoenix::ref(error_msgs_))]
        | variable_r          
        [_val = set_var_type_f(_1,boost::phoenix::ref(var_map_),
                               boost::phoenix::ref(error_msgs_),
                               _pass)]
        | ( lit('(') 
            > expression_r    [_val = _1]
            > lit(')') )
        ;
        
      int_literal_r.name("integer literal");
      int_literal_r
        %= int_ 
        >> !( lit('.')
              | lit('e')
              | lit('E') );

      double_literal_r.name("real literal");
      double_literal_r
        %= double_;

      // a[1 2 3]

      array_literal_r.name("array literal");
      array_literal_r
        %= lit("a__[")
        > *expression_r
        > lit("]")
        ;

      fun_r.name("function and argument expressions");
      fun_r 
        %= identifier_r // no test yet on valid naming
        >> args_r; 

      identifier_r.name("identifier (expression grammar)");
      identifier_r
        %= lexeme[char_("a-zA-Z") 
                  >> *char_("a-zA-Z0-9_.")];

      args_r.name("function argument expressions");
      args_r 
        %= (lit('(') >> lit(')'))
        | ( lit('(')
            >> (expression_r % ',')
            > lit(')') )
        ;
      
      dims_r.name("array dimensions");
      dims_r 
        %= lit('[') 
        > (expression_r 
           [_pass = validate_int_expr3_f(_1,boost::phoenix::ref(error_msgs_))]
           % ',')
        > lit(']')
        ;
 
      variable_r.name("variable expression");
      variable_r
        %= identifier_r;


    }
  }
}

#endif
