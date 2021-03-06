#ifndef STAN_MCMC_HMC_HAMILTONIANS_BASE_HAMILTONIAN_HPP
#define STAN_MCMC_HMC_HAMILTONIANS_BASE_HAMILTONIAN_HPP

#include <stan/interface_callbacks/writer/base_writer.hpp>
#include <stan/math/prim/mat/fun/Eigen.hpp>
#include <stan/model/util.hpp>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

namespace stan {
  namespace mcmc {

    template <class Model, class Point, class BaseRNG>
    class base_hamiltonian {
    public:
      explicit base_hamiltonian(const Model& model)
        : model_(model) {}

      ~base_hamiltonian() {}

      typedef Point PointType;

      virtual double T(Point& z) = 0;

      double V(Point& z) {
        return z.V;
      }

      virtual double tau(Point& z) = 0;

      virtual double phi(Point& z) = 0;

      double H(Point& z) {
        return T(z) + V(z);
      }

      // The time derivative of the virial, G = \sum_{d = 1}^{D} q^{d} p_{d}.
      virtual double dG_dt(
        Point& z,
        interface_callbacks::writer::base_writer& info_writer,
        interface_callbacks::writer::base_writer& error_writer) = 0;

      // tau = 0.5 p_{i} p_{j} Lambda^{ij} (q)
      virtual Eigen::VectorXd dtau_dq(
        Point& z,
        interface_callbacks::writer::base_writer& info_writer,
        interface_callbacks::writer::base_writer& error_writer) = 0;

      virtual Eigen::VectorXd dtau_dp(Point& z) = 0;

      // phi = 0.5 * log | Lambda (q) | + V(q)
      virtual Eigen::VectorXd dphi_dq(
        Point& z,
        interface_callbacks::writer::base_writer& info_writer,
        interface_callbacks::writer::base_writer& error_writer) = 0;

      virtual void sample_p(Point& z, BaseRNG& rng) = 0;

      void init(Point& z,
                interface_callbacks::writer::base_writer& info_writer,
                interface_callbacks::writer::base_writer& error_writer) {
        this->update_potential_gradient(z, info_writer, error_writer);
      }

      void update_potential(
        Point& z,
        interface_callbacks::writer::base_writer& info_writer,
        interface_callbacks::writer::base_writer& error_writer) {
        try {
          z.V = -stan::model::log_prob_propto<true>(model_, z.q);
        } catch (const std::exception& e) {
          this->write_error_msg_(e, error_writer);
          z.V = std::numeric_limits<double>::infinity();
        }
      }

      void update_potential_gradient(
        Point& z,
        interface_callbacks::writer::base_writer& info_writer,
        interface_callbacks::writer::base_writer& error_writer) {
        try {
          stan::model::gradient(model_, z.q, z.V, z.g, info_writer);
          z.V = -z.V;
        } catch (const std::exception& e) {
          this->write_error_msg_(e, error_writer);
          z.V = std::numeric_limits<double>::infinity();
        }
        z.g = -z.g;
      }

      void update_metric(
        Point& z,
        interface_callbacks::writer::base_writer& info_writer,
        interface_callbacks::writer::base_writer& error_writer) {}

      void update_metric_gradient(
        Point& z,
        interface_callbacks::writer::base_writer& info_writer,
        interface_callbacks::writer::base_writer& error_writer) {}

      void update_gradients(
        Point& z,
        interface_callbacks::writer::base_writer& info_writer,
        interface_callbacks::writer::base_writer& error_writer) {
        update_potential_gradient(z, info_writer, error_writer);
      }

    protected:
      const Model& model_;

      void write_error_msg_(const std::exception& e,
                            interface_callbacks::writer::base_writer& writer) {
        writer("Informational Message: The current Metropolis proposal "
               "is about to be rejected because of the following issue:");
        writer(e.what());
        writer("If this warning occurs sporadically, such as for highly "
               "constrained variable types like covariance matrices, then "
               "the sampler is fine,");
        writer("but if this warning occurs often then your model may be "
               "either severely ill-conditioned or misspecified.");
        writer();
      }
    };

  }  // mcmc
}  // stan

#endif
