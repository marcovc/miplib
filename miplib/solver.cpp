#include "solver.hpp"

#ifdef WITH_GUROBI
#  include "gurobi/solver.hpp"
#endif

#ifdef WITH_SCIP
#  include "scip/solver.hpp"
#endif

#ifdef WITH_LPSOLVE
#  include "lpsolve/solver.hpp"
#endif

namespace miplib {

Solver::Solver(Solver::BackendRequest backend_request, bool verbose): m_backend(Solver::Backend::Gurobi), m_constraint_autoscale(false)
{
  switch (backend_request)
  {
    case  Solver::BackendRequest::Gurobi:
      #ifdef WITH_GUROBI
      p_impl = std::make_shared<GurobiSolver>(verbose);
      m_backend = Solver::Backend::Gurobi; 
      return;
      #else
      throw std::logic_error("Request for Gurobi backend but it was not compiled.");
      #endif
    case Solver::BackendRequest::Scip:
      #ifdef WITH_SCIP
      p_impl = std::make_shared<ScipSolver>(verbose);
      m_backend = Solver::Backend::Scip; 
      return;
      #else
      throw std::logic_error("Request for SCIP backend but it was not compiled.");
      #endif
    case Solver::BackendRequest::Lpsolve:
      #ifdef WITH_LPSOLVE
      p_impl = std::make_shared<LpsolveSolver>(verbose);
      m_backend = Solver::Backend::Lpsolve;       
      return;
      #else
      throw std::logic_error("Request for Lpsolve backend but it was not compiled.");
      #endif
    case Solver::BackendRequest::BestAtCompileTime:
      #if defined(WITH_GUROBI)
      p_impl = std::make_shared<GurobiSolver>(verbose);
      m_backend = Solver::Backend::Gurobi; 
      return;
      #elif defined(WITH_SCIP)
      p_impl = std::make_shared<ScipSolver>(verbose);
      m_backend = Solver::Backend::Scip; 
      return;
      #elif defined(WITH_LPSOLVE)
      p_impl = std::make_shared<LpsolveSolver>(verbose);
      m_backend = Solver::Backend::Lpsolve; 
      return;
      #else
      throw std::logic_error("No MIP backends were compiled.");
      #endif
    case Solver::BackendRequest::BestAtRunTime:
      #if defined(WITH_GUROBI)
      if (backend_is_available(Solver::Backend::Gurobi))
      {
        p_impl = std::make_shared<GurobiSolver>(verbose);
        m_backend = Solver::Backend::Gurobi; 
        return;  
      }
      #endif
      #if defined(WITH_SCIP)
      if (backend_is_available(Solver::Backend::Scip))
      {
        p_impl = std::make_shared<ScipSolver>(verbose);
        m_backend = Solver::Backend::Scip; 
        return;  
      }
      #endif
      #if defined(WITH_LPSOLVE)
      if (backend_is_available(Solver::Backend::Lpsolve))
      {
        p_impl = std::make_shared<LpsolveSolver>(verbose);
        m_backend = Solver::Backend::Lpsolve; 
        return;  
      }
      #endif
      throw std::logic_error("No MIP backends are available.");
  }
}

void Solver::set_objective(Sense const& sense, Expr const& e)
{
  p_impl->set_objective(sense, e);
}

double Solver::get_objective_value() const
{
  return p_impl->get_objective_value();
}

Solver::Sense Solver::get_objective_sense() const
{
  return p_impl->get_objective_sense();
}

void Solver::add(Constr const& constr, bool scale)
{
  if (constr.must_be_violated())
    throw std::logic_error("Attempt to create a constraint that is trivially unsat.");

  if (scale or m_constraint_autoscale)
    p_impl->add(constr.scale());
  else
    p_impl->add(constr);
}

void Solver::add(IndicatorConstr const& constr, bool scale)
{
  if (
    p_impl->m_indicator_constraint_policy == 
      Solver::IndicatorConstraintPolicy::Reformulate or
    (
      !supports_indicator_constraint(constr) and
      p_impl->m_indicator_constraint_policy == 
      Solver::IndicatorConstraintPolicy::ReformulateIfUnsupported
    ) or
    scale
  )
  {
    for (auto const& c: constr.reformulation())
      add(c, scale);
  }
  else
    p_impl->add(constr);
}

void Solver::remove(Constr const& constr)
{
  p_impl->remove(constr);
}

void Solver::add_lazy_constr_handler(LazyConstrHandler const& constr_handler, bool at_integral_only)
{
  p_impl->add_lazy_constr_handler(constr_handler, at_integral_only);
}

std::pair<Solver::Result, bool> Solver::solve()
{
  return p_impl->solve();
}

std::pair<Solver::Result, bool> Solver::maximize(Expr const& e)
{
  set_objective(Sense::Maximize, e);
  return solve();
}

std::pair<Solver::Result, bool> Solver::minimize(Expr const& e)
{
  set_objective(Sense::Minimize, e);
  return solve();
}

void Solver::set_non_convex_policy(NonConvexPolicy policy)
{
  p_impl->set_non_convex_policy(policy);
}

void Solver::set_indicator_constraint_policy(IndicatorConstraintPolicy policy)
{
  p_impl->set_indicator_constraint_policy(policy);
}

void Solver::set_constraint_autoscale(bool autoscale)
{
  m_constraint_autoscale = autoscale;
}

void Solver::set_feasibility_tolerance(double value)
{
  p_impl->set_feasibility_tolerance(value);
}

void Solver::set_int_feasibility_tolerance(double value)
{
  p_impl->set_int_feasibility_tolerance(value);
}

void Solver::set_epsilon(double value)
{
  p_impl->set_epsilon(value);
}

void Solver::set_nr_threads(std::size_t nr_threads)
{
  p_impl->set_nr_threads(nr_threads);
}

double Solver::get_int_feasibility_tolerance() const
{
  return p_impl->get_int_feasibility_tolerance();
}

double Solver::get_feasibility_tolerance() const
{
  return p_impl->get_feasibility_tolerance();
}

double Solver::get_epsilon() const
{
  return p_impl->get_epsilon();
}

bool Solver::supports_indicator_constraint(IndicatorConstr const& constr) const
{
  return p_impl->supports_indicator_constraint(constr); 
}

bool Solver::supports_quadratic_constraints() const
{
  return p_impl->supports_quadratic_constraints();
}

bool Solver::supports_quadratic_objective() const
{
  return p_impl->supports_quadratic_objective();
}

double Solver::infinity() const
{
  return p_impl->infinity();
}

void Solver::set_time_limit(double secs)
{
  p_impl->set_time_limit(secs);
}

// Stop if after secs, the relative gap is less or equal than max_rel_gap
void Solver::set_gap_time_limit(double secs, double max_rel_gap)
{
  p_impl->set_gap_time_limit(secs, max_rel_gap);
}

void Solver::set_max_nr_solutions(std::size_t max_nr_solutions)
{
  p_impl->set_max_nr_solutions(max_nr_solutions);
}

void Solver::set_stopper(std::function<bool()> const& stopper)
{
  p_impl->set_stopper(stopper);
}

bool Solver::backend_is_compiled(Backend const& backend)
{
  switch (backend)
  {
    case  Solver::Backend::Gurobi:
      #ifdef WITH_GUROBI
      return true;
      #else
      return false;
      #endif
    case  Solver::Backend::Scip:
      #ifdef WITH_SCIP
      return true;
      #else
      return false;
      #endif
    case  Solver::Backend::Lpsolve:
      #ifdef WITH_LPSOLVE
      return true;
      #else
      return false;
      #endif
    default:
      return false;
  }
}

bool Solver::backend_is_available(Backend const& backend)
{
  if (!backend_is_compiled(backend))
    return false;
  switch (backend)
  {
    case  Solver::Backend::Gurobi:
      #ifdef WITH_GUROBI
      return GurobiSolver::is_available();
      #else
      return false;
      #endif
    case  Solver::Backend::Scip:
      #ifdef WITH_SCIP
      return ScipSolver::is_available();
      #else
      return false;
      #endif
    case  Solver::Backend::Lpsolve:
      #ifdef WITH_LPSOLVE
      return LpsolveSolver::is_available();
      #else
      return false;
      #endif
    default:
      return false;
  }
}

bool Solver::backend_is_available(BackendRequest const& backend_request)
{
  if (backend_request == BackendRequest::Gurobi)
    return backend_is_available(Backend::Gurobi);
  else  
  if (backend_request == BackendRequest::Scip)
    return backend_is_available(Backend::Scip);
  else
  if (backend_request == BackendRequest::Lpsolve)
    return backend_is_available(Backend::Lpsolve);
  return backend_is_available(Backend::Gurobi) or backend_is_available(Backend::Scip) or backend_is_available(Backend::Lpsolve); 
}

void Solver::dump(std::string const& filename) const
{
  p_impl->dump(filename);
}

void Solver::add_warm_start(PartialSolution const& partial_solution)
{
  p_impl->add_warm_start(partial_solution);
}

void Solver::set_reoptimizing(bool value)
{
  p_impl->set_reoptimizing(value);
}

void Solver::setup_reoptimization()
{
  p_impl->setup_reoptimization();
}

void Solver::compute_iis()
{
  p_impl->compute_iis();
}

std::map<Solver::Backend, std::string> Solver::backend_info()
{
  std::map<Backend, std::string> r;
  #ifdef WITH_GUROBI
  if (backend_is_compiled(Solver::Backend::Gurobi))
    r[Solver::Backend::Gurobi] = GurobiSolver::backend_info();
  #endif
  #ifdef WITH_SCIP
  if (backend_is_compiled(Solver::Backend::Scip))
    r[Solver::Backend::Scip] = ScipSolver::backend_info();
  #endif
  #ifdef WITH_LPSOLVE
  if (backend_is_compiled(Solver::Backend::Lpsolve))
    r[Solver::Backend::Lpsolve] = LpsolveSolver::backend_info();
  #endif
  return r;
}

namespace detail {
void ISolver::set_indicator_constraint_policy(Solver::IndicatorConstraintPolicy policy)
{
  m_indicator_constraint_policy = policy;
}

void ISolver::compute_iis()
{
  throw std::logic_error("Backend does not support computing IIs, or mapping not added yet.");
}

}

std::ostream& operator<<(std::ostream& os, Solver::BackendRequest const& solver_backend_request)
{
  switch (solver_backend_request)
  {
    case Solver::BackendRequest::Scip:
      os << "Scip";
      break;
    case Solver::BackendRequest::Gurobi:
      os << "Gurobi";
      break;
    case Solver::BackendRequest::Lpsolve:
      os << "Lpsolve";
      break;
    case Solver::BackendRequest::BestAtCompileTime:
      os << "BestAtCompileTime";
      break;
    case Solver::BackendRequest::BestAtRunTime:
      os << "BestAtRunTime";
      break;
  }
  return os;
}

}  // namespace miplib
