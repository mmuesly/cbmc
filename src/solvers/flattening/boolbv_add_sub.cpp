/*******************************************************************\

Module:

Author: Daniel Kroening, kroening@kroening.com

\*******************************************************************/

#include "boolbv.h"

#include <util/invariant.h>
#include <util/std_types.h>

#include <solvers/floatbv/float_utils.h>

bvt boolbvt::convert_add_sub(const exprt &expr)
{
  const typet &type=ns.follow(expr.type());

  if(type.id()!=ID_unsignedbv &&
     type.id()!=ID_signedbv &&
     type.id()!=ID_fixedbv &&
     type.id()!=ID_floatbv &&
     type.id()!=ID_range &&
     type.id()!=ID_complex &&
     type.id()!=ID_vector)
    return conversion_failed(expr);

  std::size_t width=boolbv_width(type);

  if(width==0)
    return conversion_failed(expr);

  const exprt::operandst &operands=expr.operands();

  if(operands.empty())
    throw "operator "+expr.id_string()+" takes at least one operand";

  const exprt &op0=expr.op0();
  DATA_INVARIANT(
    op0.type() == type, "add/sub with mixed types:\n" + expr.pretty());

  bvt bv=convert_bv(op0);

  if(bv.size()!=width)
    throw "convert_add_sub: unexpected operand 0 width";

  bool subtract=(expr.id()==ID_minus ||
                 expr.id()=="no-overflow-minus");

  bool no_overflow=(expr.id()=="no-overflow-plus" ||
                    expr.id()=="no-overflow-minus");

  typet arithmetic_type=
    (type.id()==ID_vector || type.id()==ID_complex)?
      ns.follow(type.subtype()):type;

  bv_utilst::representationt rep=
    (arithmetic_type.id()==ID_signedbv ||
     arithmetic_type.id()==ID_fixedbv)?bv_utilst::representationt::SIGNED:
                                       bv_utilst::representationt::UNSIGNED;

  for(exprt::operandst::const_iterator
      it=operands.begin()+1;
      it!=operands.end(); it++)
  {
    DATA_INVARIANT(
      it->type() == type, "add/sub with mixed types:\n" + expr.pretty());

    const bvt &op=convert_bv(*it);

    if(op.size()!=width)
      throw "convert_add_sub: unexpected operand width";

    if(type.id()==ID_vector || type.id()==ID_complex)
    {
      const typet &subtype=ns.follow(type.subtype());

      std::size_t sub_width=boolbv_width(subtype);

      if(sub_width==0 || width%sub_width!=0)
        throw "convert_add_sub: unexpected vector operand width";

      std::size_t size=width/sub_width;
      bv.resize(width);

      for(std::size_t i=0; i<size; i++)
      {
        bvt tmp_op;
        tmp_op.resize(sub_width);

        for(std::size_t j=0; j<tmp_op.size(); j++)
        {
          assert(i*sub_width+j<op.size());
          tmp_op[j]=op[i*sub_width+j];
        }

        bvt tmp_result;
        tmp_result.resize(sub_width);

        for(std::size_t j=0; j<tmp_result.size(); j++)
        {
          assert(i*sub_width+j<bv.size());
          tmp_result[j]=bv[i*sub_width+j];
        }

        if(type.subtype().id()==ID_floatbv)
        {
          // needs to change due to rounding mode
          float_utilst float_utils(prop, to_floatbv_type(subtype));
          tmp_result=float_utils.add_sub(tmp_result, tmp_op, subtract);
        }
        else
          tmp_result=bv_utils.add_sub(tmp_result, tmp_op, subtract);

        assert(tmp_result.size()==sub_width);

        for(std::size_t j=0; j<tmp_result.size(); j++)
        {
          assert(i*sub_width+j<bv.size());
          bv[i*sub_width+j]=tmp_result[j];
        }
      }
    }
    else if(type.id()==ID_floatbv)
    {
      // needs to change due to rounding mode
      float_utilst float_utils(prop, to_floatbv_type(arithmetic_type));
      bv=float_utils.add_sub(bv, op, subtract);
    }
    else if(no_overflow)
      bv=bv_utils.add_sub_no_overflow(bv, op, subtract, rep);
    else
      bv=bv_utils.add_sub(bv, op, subtract);
  }

  return bv;
}
