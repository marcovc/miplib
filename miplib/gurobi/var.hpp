#pragma once

#include <miplib/gurobi/solver.hpp>
#include <miplib/var.hpp>

namespace miplib {

struct GurobiVar : detail::IVar
{
  GurobiVar(Solver const& solver, GRBVar const& v, Var::Type type, double lb, double ub);
  virtual ~GurobiVar() {};

  void update_solver_if_pending() const;

  double value() const;

  Var::Type type() const;

  std::optional<std::string> name() const;
  void set_name(std::string const& new_name);

  Solver const& solver() const { return m_solver; }

  double lb() const;
  double ub() const;

  void set_lb(double new_lb);
  void set_ub(double new_ub);

  // start_index is the index of the warm start (i.e. to allow multiple warm start solutions).
  void set_start_value(std::size_t start_index, double v);
  void set_hint(double v);

  Solver m_solver;
  GRBVar m_var;
  // Immutable after creation and mirrored bounds, so reads never need a GRBupdatemodel flush.
  Var::Type m_cached_type;
  mutable double m_cached_lb;
  mutable double m_cached_ub;
};

}  // namespace miplib
