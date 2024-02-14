import umbridge
import numpy as np
import cuqi
from scipy.io import loadmat

# %% CUQI imports
from cuqi.model import LinearModel
from cuqi.geometry import Image2D
from cuqi.array import CUQIarray
from cuqi.distribution import Gaussian, GMRF, LMRF, CMRF
from cuqi.problem import BayesianProblem


class CT_UM(umbridge.Model):
    """Base benchmark for all 2D CT problems"""

    dim = 256**2  # Dimension of the 2D signal:  256 x 256
    default_delta = 0.01  # Default value for delta for all but LMRF

    def __init__(self, name):
        """Initialize the model

        parameters
        ----------
        name : str
            Name of the model

        """
        # Load data and exact phantom from file.
        # These were originally generated by the data_script.py script.
        data = np.load("data_ct.npz")

        # Specification of image grid
        N = 256
        nv = 30

        # Load the matrix for the forward model and scale by pixel width
        Amat = loadmat('A' + str(N) + '_' + str(nv) + '.mat')["A"]

        lower = -1.0
        upper = 1.0
        width = upper - lower
        dx = width/N

        Amat = dx*Amat

        # Create the CUQIpy linear model
        A = LinearModel(Amat)

        # Create the visual_only Image2D geometries of image and sinogram spaces
        dg = Image2D(( N,N), visual_only=True)
        rg = Image2D((nv,N), visual_only=True)

        # Equip linear operator with geometries
        A.domain_geometry = dg
        A.range_geometry = rg

        # Create the CUQI data structure from vectorized image and geometry
        imC = CUQIarray(data["exact"], geometry=dg)

        # Specify placeholder x distribution
        x = Gaussian(mean=np.zeros(A.domain_dim), 
                           cov=0.01,
                           geometry=A.domain_geometry)
        
        # Choose noise std and create data distribution
        s = 0.01
        y = Gaussian(A@x, s**2)

        # Create CUQIarray with loaded noisy sinogram data.
        y_data = CUQIarray(data["y_data"], geometry=rg)

        # Set up the Bayesian problem with random variable and observed data.
        BP = BayesianProblem(y, x).set_data(y=y_data)

        # Store likelihood and prior separately
        self.likelihood = BP.likelihood
        self.prior = BP.prior

        super().__init__(name)

    def get_input_sizes(self, config):
        return [self.dim]

    def get_output_sizes(self, config):
        return [1]

    def __call__(self, parameters, config):
        posterior = self._configure_posterior(config)
        output = posterior.logpdf(np.asarray(parameters[0]))
        return [[output[0]]]

    def gradient(self, out_wrt, in_wrt, parameters, sens, config):
        posterior = self._configure_posterior(config)
        output = posterior.gradient(np.asarray(parameters[0])) * sens
        return output.tolist()

    def supports_evaluate(self):
        return True

    def supports_gradient(self):
        return True

    def _configure_posterior(self, config) -> cuqi.distribution.Posterior:
        """ Configure the posterior distribution by conditioning on the delta parameter. """
        if (not "delta" in config):
            config["delta"] = self.default_delta
        prior = self.prior(delta=config["delta"])
        posterior = cuqi.distribution.Posterior(self.likelihood, prior)
        return posterior


class CT_Gaussian(CT_UM):
    """CT with Gaussian prior"""

    def __init__(self):
        super().__init__(self.__class__.__name__)
        self.prior = Gaussian(np.zeros(self.dim), 
                              lambda delta: delta, 
                              geometry=self.likelihood.geometry,
                              name="x")

class CT_GMRF(CT_UM):
    """CT with 2D Gaussian Markov Random Field (GMRF) prior"""

    def __init__(self):
        super().__init__(self.__class__.__name__)
        self.prior = GMRF(np.zeros(self.dim), 
                          lambda delta: 1 / delta,
                          physical_dim=2,
                          geometry=self.likelihood.geometry,
                          name="x")


class CT_LMRF(CT_UM):
    """CT with 2D Laplace Markov Random Field (LMRF) prior"""

    default_delta = 0.1  # Default value for delta for LMRF

    def __init__(self):
        super().__init__(self.__class__.__name__)
        self.prior = LMRF(np.zeros(self.dim), 
                                  lambda delta: delta, 
                                  geometry=self.likelihood.geometry,
                                  name="x")

    def supports_gradient(self):
        return False


class CT_CMRF(CT_UM):
    """CT with 2D Cauchy Markov Random Field (CMRF) prior"""

    def __init__(self):
        super().__init__(self.__class__.__name__)
        self.prior = CMRF(np.zeros(self.dim), 
                                 lambda delta: delta, 
                                 geometry=self.likelihood.geometry,
                                 name="x")


class CT_ExactSolution(umbridge.Model):
    """Exact solution for the CT problem"""

    def __init__(self):
        """Initialize the model. Load the exact solution from file."""
        data = np.load("data_ct.npz")
        self.exactSolution = data["exact"]
        super().__init__(self.__class__.__name__)

    def get_input_sizes(self, config):
        return [0]

    def get_output_sizes(self, config):
        return [len(self.exactSolution)]

    def __call__(self, parameters, config):
        return [self.exactSolution.tolist()]

    def supports_evaluate(self):
        return True


model_Gaussian = CT_Gaussian()
model_GMRF = CT_GMRF()
model_LMRF = CT_LMRF()
model_CMRF = CT_CMRF()
model_exactSolution = CT_ExactSolution()

umbridge.serve_models(
    [model_Gaussian, model_GMRF, model_LMRF, model_CMRF, model_exactSolution], 4243
)