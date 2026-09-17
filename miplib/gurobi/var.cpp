
#include "var.hpp"
#include "exception.hpp"

namespace miplib {

GurobiVar::GurobiVar(
  Solver const& solver, GRBVar const& v, Var::Type type, double lb, double ub
) :
  m_solver(solver),
  m_var(v),
  m_cached_type(type),
  m_cached_lb(lb),
  m_cached_ub(ub)
{
  static_cast<GurobiSolver const&>(*m_solver.p_impl).set_pending_update();
}

void GurobiVar::update_solver_if_pending() const
{
  auto const& gurobi_solver = static_cast<GurobiSolver const&>(*m_solver.p_impl);
  if (!gurobi_solver.is_in_callback())
    gurobi_solver.update_if_pending();
}

double GurobiVar::value() const
{
  auto const& gurobi_solver = static_cast<GurobiSolver const&>(*m_solver.p_impl);

  if (gurobi_solver.is_in_callback())
    return gurobi_solver.p_callback->value(*this);

  update_solver_if_pending();
  return call_with_exception_logging([&]{return m_var.get(GRB_DoubleAttr_X);});
}

Var::Type GurobiVar::type() const
{
  // Cached — variable type is immutable after creation.
  return m_cached_type;
}

std::optional<std::string> GurobiVar::name() const
{
  update_solver_if_pending();
  auto n = m_var.get(GRB_StringAttr_VarName);
  if (n.empty())
    return std::nullopt;
  return n;
}

void GurobiVar::set_name(std::string const& new_name)
{
  auto const& gurobi_solver = static_cast<GurobiSolver const&>(*m_solver.p_impl);
  
  if (gurobi_solver.is_in_callback())
    throw std::logic_error("Operation not allowed within callback.");

  m_var.set(GRB_StringAttr_VarName, new_name);
  gurobi_solver.set_pending_update();  
}

double GurobiVar::lb() const
{
  // Cached — kept in sync by set_lb, so no model flush is needed to read it.
  return m_cached_lb;
}

double GurobiVar::ub() const
{
  // Cached — kept in sync by set_ub, so no model flush is needed to read it.
  return m_cached_ub;
}

void GurobiVar::set_lb(double new_lb)
{
  auto const& gurobi_solver = static_cast<GurobiSolver const&>(*m_solver.p_impl);
  
  if (gurobi_solver.is_in_callback())
    throw std::logic_error("Operation not allowed within callback.");

  m_var.set(GRB_DoubleAttr_LB, new_lb);
  m_cached_lb = new_lb;
  gurobi_solver.set_pending_update();  
}

void GurobiVar::set_ub(double new_ub)
{
  auto const& gurobi_solver = static_cast<GurobiSolver const&>(*m_solver.p_impl);
  
  if (gurobi_solver.is_in_callback())
    throw std::logic_error("Operation not allowed within callback.");

  m_var.set(GRB_DoubleAttr_UB, new_ub);
  m_cached_ub = new_ub;
  gurobi_solver.set_pending_update();  
}

void GurobiVar::set_start_value(std::size_t start_index, double v)
{
  auto const& gurobi_solver = static_cast<GurobiSolver const&>(*m_solver.p_impl);
  
  if (gurobi_solver.is_in_callback())
    throw std::logic_error("Operation not allowed within callback.");

  gurobi_solver.model.set(GRB_IntParam_StartNumber, (int)start_index);
  m_var.set(GRB_DoubleAttr_Start, v);
  gurobi_solver.set_pending_update();
}

void GurobiVar::set_hint(double v)
{
  auto const& gurobi_solver = static_cast<GurobiSolver const&>(*m_solver.p_impl);
  
  if (gurobi_solver.is_in_callback())
    throw std::logic_error("Operation not allowed within callback.");

  m_var.set(GRB_DoubleAttr_VarHintVal, v);
  gurobi_solver.set_pending_update();
}

}  // namespace miplib
