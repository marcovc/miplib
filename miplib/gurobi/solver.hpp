#pragma once

#include <miplib/solver.hpp>

#include <gurobi_c++.h>

namespace miplib {

namespace detail {

struct GurobiCurrentStateHandle;
using GurobiStopper = std::function<bool()>;

struct GurobiCurrentStateHandle : GRBCallback, ICurrentStateHandle
{
  GurobiCurrentStateHandle();
  void add_constr_handler(LazyConstrHandler const& constr_hdlr, bool integral_only);
  void add_stopper(GurobiStopper const& stopper);

  double value(IVar const& var) const;
  void add_lazy(Constr const& constr);
  void callback();
  bool is_active() const { return m_active; }
  std::optional<double> runtime() const;  // optional since not every callback invocation allows to query this property
  std::optional<double> gap() const; // optional since not every callback invocation allows to query this property

  // constraint handlers that can run on integral or non-integral nodes
  std::vector<LazyConstrHandler> m_constr_hdlrs;
  // constraint handlers that can run on integral nodes exclusively
  std::vector<LazyConstrHandler> m_integral_only_constr_hdlrs;
  // stoppers
  std::vector<GurobiStopper> m_stoppers;
  bool m_active;
};
}

struct GurobiSolver : detail::ISolver
{
  GurobiSolver(bool verbose = true);

  std::shared_ptr<detail::IVar> create_var(
    Solver const& solver,
    Var::Type const& type,
    std::optional<double> const& lb,
    std::optional<double> const& ub,
    std::optional<std::string> const& name
  );

  std::shared_ptr<detail::IConstr> create_constr(
    Constr::Type const& type, Expr const& e, std::optional<std::string> const& name
  );

  std::shared_ptr<detail::IIndicatorConstr> create_indicator_constr(
    Constr const& implicant,
    Constr const& implicand,
    std::optional<std::string> const& name);

  void set_objective(Solver::Sense const& sense, Expr const& e);
  double get_objective_value() const;
  Solver::Sense get_objective_sense() const;

  void add(Constr const& constr);
  void add(IndicatorConstr const& constr);

  void remove(Constr const& constr);

  void add_lazy_constr_handler(LazyConstrHandler const&, bool at_integral_only);

  std::pair<Solver::Result, bool> solve();

  void set_non_convex_policy(Solver::NonConvexPolicy policy);
  void set_int_feasibility_tolerance(double value);
  void set_feasibility_tolerance(double value);
  void set_epsilon(double value);
  void set_nr_threads(std::size_t);
  
  double get_int_feasibility_tolerance() const;
  double get_feasibility_tolerance() const;
  double get_epsilon() const;

  void set_pending_update() const;
  void update_if_pending() const;

  bool supports_quadratic_constraints() const { return true; }
  bool supports_quadratic_objective() const { return true; }

  bool supports_indicator_constraint(IndicatorConstr const& i) const;

  double infinity() const;

  void set_time_limit(double secs);

  void set_gap_time_limit(double secs, double max_rel_gap);
  void set_max_nr_solutions(std::size_t);
  void set_stopper(std::function<bool()> const& stopper);

  void dump(std::string const& filename) const;

  bool is_in_callback() const;
  
  void add_warm_start(PartialSolution const& partial_solution);

  void set_reoptimizing(bool);
  void setup_reoptimization();

  static std::string backend_info();

  static bool is_available();

  void compute_iis();

  GRBEnv env;
  mutable GRBModel model;
  mutable bool pending_update;
  mutable bool model_has_changed_since_last_solve;
  std::unique_ptr<detail::GurobiCurrentStateHandle> p_callback;
};

}  // namespace miplib
